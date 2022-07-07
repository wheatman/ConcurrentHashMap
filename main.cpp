
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

template <class key_t = uint32_t, bool batched = true>
void test_hashmap_unordered_insert_batches(std::vector<key_t> elements,
                                           key_t correct_sum,
                                           uint64_t batch_size,
                                           int blow_up_factor) {

  if (batch_size > elements.size()) {
    batch_size = elements.size();
  }
  ConcurrentHashSet<key_t> hashset(getWorkers(), blow_up_factor);
  timer insert_timer("insert");
  insert_timer.start();
  for (uint64_t i = 0; i < elements.size(); i += batch_size) {
    uint64_t end = std::min(i + batch_size, elements.size());
    if constexpr (batched) {
      hashset.insert_batch(elements.data() + i, end - i);
    } else {
      parallel_for(uint64_t j = i; j < end; j++) {
        hashset.insert(elements[j]);
      }
    }
  }
  insert_timer.stop();
  timer sum_timer("sum_timer_with_locks");
  key_t sum = 0;
  sum_timer.start();
  sum = hashset.sum();
  sum_timer.stop();

  std::cout << elements.size() << "," << batch_size << "," << blow_up_factor
            << "," << elements.size() / static_cast<double>(insert_timer.get())
            << "," << elements.size() / static_cast<double>(sum_timer.get())
            << "," << PARALLEL << "," << batched << "," << sum << std::endl;
  if (sum != correct_sum) {
    std::cout << "got wrong sum" << std::endl;
    std::cout << "got sum " << sum << " correct sum was " << correct_sum
              << std::endl;
  }
}

int main(int32_t argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "call with ./run <max_num_elements> <min_batch_size> "
                 "<blow_up_factor>"
              << std::endl;
  }
  std::seed_seq seed{0};
  uint64_t num_elements = std::strtol(argv[1], nullptr, 10);
  uint64_t batch_size = std::strtol(argv[2], nullptr, 10);
  uint64_t blow_up_factor = std::strtol(argv[3], nullptr, 10);

  std::cout << "num_elements, batch_size, blow_up_factor, insert_throughput, "
               "sum_throughput, parallel?, batched?, sum"
            << std::endl;

  using key_t = uint64_t;

  for (uint64_t i = 1000; i <= num_elements; i *= 10) {
    std::vector<key_t> data =
        create_random_data<key_t>(i, std::numeric_limits<key_t>::max(), seed);
    std::unordered_set<key_t> correct;
    for (const auto d : data) {
      correct.insert(d);
    }
    key_t correct_sum = 0;
    for (const auto d : correct) {
      correct_sum += d;
    }

    for (uint64_t j = batch_size; j < i; j *= 10) {
      test_hashmap_unordered_insert_batches<key_t, true>(data, correct_sum, j,
                                                         blow_up_factor);
      // test_hashmap_unordered_insert_batches<key_t, false>(data, correct_sum,
      // j,
      //                                                     blow_up_factor);
    }
  }

  return 0;
}