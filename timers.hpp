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
  bool reported = false;
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
  inline void report([[maybe_unused]] bool clear = true) {
#if ENABLE_TRACE_TIMER == 1
    if (clear) {
      reported = true;
    }
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

  uint64_t get([[maybe_unused]] bool clear = true) {
#if ENABLE_TRACE_TIMER == 1
    if (clear) {
      reported = true;
    }
    return elapsed_time;
#endif
    return 0;
  }

  ~timer() {
#if ENABLE_TRACE_TIMER == 1
    if (!reported) {
      report();
    }
#endif
  }
};