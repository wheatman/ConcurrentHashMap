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

public:
  ConcurrentHashMap(int num_workers, int blow_up_factor = 10)
      : maps(num_workers * blow_up_factor) {}

  void insert(Key k, T value) {
    size_t bucket = Hash{}(k) % maps.size();
    maps[bucket].m.second.lock();
    maps[bucket].m.first.insert({k, value});
    maps[bucket].m.second.unlock();
  }

  void remove(Key k) {
    size_t bucket = Hash{}(k) % maps.size();
    maps[bucket].m.second.lock();
    maps[bucket].m.first.erase(k);
    maps[bucket].m.second.unlock();
  }

  T value(Key k, T null_value) {
    size_t bucket = Hash{}(k) % maps.size();
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
    size_t bucket = Hash{}(k) % maps.size();
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

public:
  ConcurrentHashSet(int num_workers, int blow_up_factor = 10)
      : sets(num_workers * blow_up_factor) {}

  void insert(Key k) {
    size_t bucket = Hash{}(k) % sets.size();
    sets[bucket].s.second.lock();
    sets[bucket].s.first.insert(k);
    sets[bucket].s.second.unlock();
  }

  void insert_batch(Key *k, uint64_t num_keys) {
    std::sort(k, k + num_keys,
              [](Key a, Key b) { return Hash{}(a) > Hash{}(b); });
    parallel_for(uint64_t i = 0; i < num_keys; i++) { insert(k[i]); }
  }

  void remove(Key k) {
    size_t bucket = Hash{}(k) % sets.size();
    sets[bucket].s.second.lock();
    sets[bucket].s.first.erase(k);
    sets[bucket].s.second.unlock();
  }

  bool contains(Key k) {
    size_t bucket = Hash{}(k) % sets.size();
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