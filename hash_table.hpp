#pragma once

#include "parallel.h"
#include "reducer.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <class Key, class T, class Hash = std::hash<Key>>
class ConcurrentHashMap {
private:
  using Map = std::pair<std::unordered_map<Key, T>, std::mutex>;
  struct aligned_map {
    alignas(hardware_destructive_interference_size) Map m;
  };
  std::vector<aligned_map> maps;

  static size_t bucket_hash(Key k) { return Hash{}(k >> (sizeof(Key) * 4)); }

  template <class U> size_t log2_up(U i) {
    size_t a = 0;
    U b = i - 1;
    while (b > 0) {
      b = b >> 1U;
      a++;
    }
    return a;
  }

public:
  ConcurrentHashMap(int blow_up_factor = 10)
      : maps(1UL << log2_up(getWorkers() * blow_up_factor)) {}

  void insert(Key k, T value) {
    size_t bucket = bucket_hash(k) % maps.size();
    maps[bucket].m.second.lock();
    maps[bucket].m.first.insert({k, value});
    maps[bucket].m.second.unlock();
  }

  void remove(Key k) {
    size_t bucket = bucket_hash(k) % maps.size();
    maps[bucket].m.second.lock();
    maps[bucket].m.first.erase(k);
    maps[bucket].m.second.unlock();
  }

  T value(Key k, T null_value) {
    size_t bucket = bucket_hash(k) % maps.size();
    maps[bucket].m.second.lock();
    auto it = maps[bucket].m.first.find(k);
    T value;
    if (it == maps[bucket].m.first.end()) {
      value = null_value;
    } else {
      value = *it;
    }
    maps[bucket].m.second.unlock();
    return value;
  }
  bool contains(Key k) {
    size_t bucket = bucket_hash(k) % maps.size();
    maps[bucket].m.second.lock();
    bool has = maps[bucket].m.first.contains(k);
    maps[bucket].m.second.unlock();
    return has;
  }
};

template <class Key, class Hash = std::hash<Key>> class ConcurrentHashSet {
private:
  using Set = std::pair<std::unordered_set<Key>, std::mutex>;
  struct aligned_set {
    alignas(hardware_destructive_interference_size) Set s;
  };
  std::vector<aligned_set> sets;

  static size_t bucket_hash(Key k) { return Hash{}(k >> (sizeof(Key) * 4)); }

  template <class T> size_t log2_up(T i) {
    size_t a = 0;
    T b = i - 1;
    while (b > 0) {
      b = b >> 1U;
      a++;
    }
    return a;
  }

public:
  ConcurrentHashSet(int num_workers, int blow_up_factor = 10)
      : sets(1UL << log2_up(num_workers * blow_up_factor)) {}

  void insert(Key k) {
    size_t bucket = bucket_hash(k) % sets.size();
    sets[bucket].s.second.lock();
    sets[bucket].s.first.insert(k);
    sets[bucket].s.second.unlock();
  }

  void insert_batch(Key *k, uint64_t num_keys) {
    std::sort(k, k + num_keys,
              [](Key a, Key b) { return bucket_hash(a) > bucket_hash(b); });
    parallel_for(uint64_t i = 0; i < num_keys; i++) { insert(k[i]); }
  }

  void remove(Key k) {
    size_t bucket = bucket_hash(k) % sets.size();
    sets[bucket].s.second.lock();
    sets[bucket].s.first.erase(k);
    sets[bucket].s.second.unlock();
  }

  bool contains(Key k) {
    size_t bucket = bucket_hash(k) % sets.size();
    sets[bucket].s.second.lock();
    bool has = sets[bucket].s.first.contains(k);
    sets[bucket].s.second.unlock();
    return has;
  }
  Key sum() {
    Reducer_sum<Key> s(getWorkers());
    parallel_for(size_t i = 0; i < sets.size(); i++) {
      Key local_sum = 0;
      sets[i].s.second.lock();
      for (const auto &item : sets[i].s.first) {
        local_sum += item;
      }
      sets[i].s.second.unlock();
      s.add(local_sum);
    }
    return s.get();
  }
  Key sum_no_lock() {
    Reducer_sum<Key> s(getWorkers());
    parallel_for(size_t i = 0; i < sets.size(); i++) {
      Key local_sum = 0;
      for (const auto &item : sets[i].s.first) {
        local_sum += item;
      }
      s.add(local_sum);
    }
    return s.get();
  }
};