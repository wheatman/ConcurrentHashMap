#pragma once

#include <cstdint>
#include <iostream> // cout
#include <string>
#include <sys/time.h>

class timer {
#if ENABLE_TRACE_TIMER == 1
  uint64_t starting_time = 0;
  uint64_t elapsed_time = 0;
  std::string name;
  static constexpr bool csv_printing = true;
#endif

#if CYCLE_TIMER == 1
  static inline uint64_t get_time() { return __rdtsc(); }
#else
  static inline uint64_t get_time() {
    struct timeval st {};
    gettimeofday(&st, nullptr);
    return st.tv_sec * 1000000 + st.tv_usec;
  }
#endif

public:
#if ENABLE_TRACE_TIMER == 1
  timer(std::string n) : name(std::move(n)) {}
#else
  timer([[maybe_unused]] const std::string &n) {}
#endif

  inline void start() {
#if ENABLE_TRACE_TIMER == 1
    starting_time = get_time();
#endif
  }

  inline void stop() {
#if ENABLE_TRACE_TIMER == 1
    uint64_t end_time = get_time();
    elapsed_time += (end_time - starting_time);
#endif
  }
  inline void report() {
#if ENABLE_TRACE_TIMER == 1
    if (csv_printing) {
#if CYCLE_TIMER == 1
      std::cout << name << ", " << elapsed_time << ", cycles" << std::endl;
#else
      std::cout << name << ", " << elapsed_time << ", micro secs" << std::endl;
#endif
    } else {
#if CYCLE_TIMER == 1
      std::cout << name << ": " << elapsed_time << " [cycles]" << std::endl;
#else
      std::cout << name << ": " << elapsed_time << " [micro secs]" << std::endl;
#endif
    }
#endif
  }

  ~timer() {
#if ENABLE_TRACE_TIMER == 1
    report();
#endif
  }
};