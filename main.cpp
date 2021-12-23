
#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <unordered_set>
#include <vector>

#include "hash_table.hpp"
#include "parallel.h"
#include "timers.hpp"

template <class T>
std::vector<T> create_random_data(size_t n, size_t max_val,
                                  std::seed_seq &seed) {

  std::mt19937_64 eng(seed); // a source of random data

  std::uniform_int_distribution<T> dist(0, max_val);
  std::vector<T> v(n);

  generate(begin(v), end(v), bind(dist, eng));
  return v;
}

template <class key_t = uint32_t>
void test_hashmap_unordered_insert_batches(uint64_t num_elements,
                                           uint64_t batch_size,
                                           int blow_up_factor,
                                           std::seed_seq &seed) {
  std::cout << "num_elements = " << num_elements
            << " batch_size = " << batch_size
            << " blow_up_factor = " << blow_up_factor << std::endl;

  if (batch_size > num_elements) {
    batch_size = num_elements;
  }
  std::vector<key_t> data = create_random_data<key_t>(
      num_elements, std::numeric_limits<key_t>::max(), seed);

#if VERIFY == 1
  std::unordered_set<key_t> correct;
  for (const auto d : data) {
    correct.insert(d);
  }
  key_t correct_sum = 0;
  for (const auto d : correct) {
    correct_sum += d;
  }
#endif

  ConcurrentHashSet<key_t> hashset(getWorkers(), blow_up_factor);
  timer insert_timer("insert");
  insert_timer.start();
  for (uint64_t i = 0; i < num_elements; i += batch_size) {
    uint64_t end = std::min(i + batch_size, num_elements);
    parallel_for(uint64_t j = i; j < end; j++) { hashset.insert(data[j]); }
  }
  insert_timer.stop();
  timer sum_timer("sum_timer_with_locks");
  key_t sum = 0;
  sum_timer.start();
  sum = hashset.sum();
  sum_timer.stop();
#if VERIFY == 1
  if (sum != correct_sum) {
    std::cout << "got wrong sum" << std::endl;
    std::cout << "got sum " << sum << " correct sum was " << correct_sum
              << std::endl;
  }
#endif
}

template <class key_t = uint32_t>
void test_hashmap_unordered_insert_batches_2(uint64_t num_elements,
                                             uint64_t batch_size,
                                             int blow_up_factor,
                                             std::seed_seq &seed) {
  std::cout << "num_elements = " << num_elements
            << " batch_size = " << batch_size
            << " blow_up_factor = " << blow_up_factor << std::endl;

  if (batch_size > num_elements) {
    batch_size = num_elements;
  }
  std::vector<key_t> data = create_random_data<key_t>(
      num_elements, std::numeric_limits<key_t>::max(), seed);

#if VERIFY == 1
  std::unordered_set<key_t> correct;
  for (const auto d : data) {
    correct.insert(d);
  }
  key_t correct_sum = 0;
  for (const auto d : correct) {
    correct_sum += d;
  }
#endif

  ConcurrentHashSet<key_t> hashset(getWorkers(), blow_up_factor);
  timer insert_timer("insert");
  insert_timer.start();
  for (uint64_t i = 0; i < num_elements; i += batch_size) {
    uint64_t end = std::min(i + batch_size, num_elements);
    hashset.insert_batch(data.data() + i, end - i);
  }
  insert_timer.stop();
  timer sum_timer("sum_timer_with_locks");
  key_t sum = 0;
  sum_timer.start();
  sum = hashset.sum();
  sum_timer.stop();
#if VERIFY == 1
  if (sum != correct_sum) {
    std::cout << "got wrong sum" << std::endl;
    std::cout << "got sum " << sum << " correct sum was " << correct_sum
              << std::endl;
  }
#endif
}

int main(int32_t argc, char *argv[]) {
  std::seed_seq seed{0};
  uint64_t num_elements = std::strtol(argv[1], nullptr, 10);
  uint64_t batch_size = std::strtol(argv[2], nullptr, 10);
  uint64_t blow_up_factor = std::strtol(argv[3], nullptr, 10);

  for (uint64_t i = 1000; i < num_elements; i *= 10) {
    for (uint64_t j = 100; j < i / 10; j *= 10) {
      test_hashmap_unordered_insert_batches(i, j, blow_up_factor, seed);
      test_hashmap_unordered_insert_batches_2(i, j, blow_up_factor, seed);
    }
  }

  return 0;
}