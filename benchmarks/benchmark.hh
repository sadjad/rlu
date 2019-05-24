#ifndef BENCHMARK_HH
#define BENCHMARK_HH

#include <chrono>
#include <random>
#include <thread>

class Benchmark {
public:
  using clock = std::chrono::high_resolution_clock;

  struct Config {
    size_t n_threads = 8;
    float update_ratio = 0.02;
    int32_t min_value = -1024;
    int32_t max_value = 1023;
    size_t initial_size = 512;
    std::chrono::seconds duration{2};
  };

  struct Stats {
    clock::time_point start{clock::time_point::max()};
    clock::time_point end{clock::time_point::min()};

    size_t count_add{0};
    size_t count_erase{0};
    size_t count_contains{0};
    size_t count_found{0};

    void merge(const Stats& stats);
    void print();
  };

private:
  const Config config_;
  Stats aggregate_{};

public:
  Benchmark(const Config& config) : config_(config) {}
  void run();
};

#endif /* BENCHMARK_HH */
