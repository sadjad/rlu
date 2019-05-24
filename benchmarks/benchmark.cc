#include "benchmark.hh"

#include <future>
#include <iomanip>
#include <iostream>
#include <thread>

#include "list.hh"
#include "rlu.hh"

using namespace std;
using namespace std::chrono;

int32_t randint(const int32_t min, const int32_t max)
{
  static thread_local random_device dev;
  static thread_local mt19937 rng{dev()};
  uniform_int_distribution<int32_t> distribution{min, max};
  return distribution(rng);
}

bool coinflip(const float p_true = 0.5)
{
  static thread_local random_device dev;
  static thread_local mt19937 rng{dev()};
  bernoulli_distribution distribution{p_true};
  return distribution(rng);
}

void Benchmark::Stats::merge(const Stats &other)
{
  start = min(start, other.start);
  end = max(end, other.end);

  count_add += other.count_add;
  count_erase += other.count_erase;
  count_contains += other.count_contains;
  count_found += other.count_found;
}

void Benchmark::Stats::print()
{
  const auto d = duration_cast<microseconds>(end - start).count();
  const auto total = count_add + count_erase + count_contains;

  const float ops_per_us = (float)total / d;

  auto percentage = [](const int n, const int total) -> double {
    return total ? (100.0 * n / total) : 0.0;
  };

  cout << endl
       << "  Duration: " << fixed << setprecision(3) << (d / 1e6) << "s" << endl
       << "     Total: " << total << endl
       << "       Add: " << count_add << " (" << fixed << setprecision(2)
       << percentage(count_add, total) << "%)" << endl
       << "     Erase: " << count_erase << " (" << fixed << setprecision(2)
       << percentage(count_erase, total) << "%)" << endl
       << "  Contains: " << count_contains << " (" << fixed << setprecision(2)
       << percentage(count_contains, total) << "%)" << endl
       << "     Found: " << count_found << " (" << fixed << setprecision(2)
       << percentage(count_found, count_contains) << "%)" << endl
       << endl
       << "    Ops/us: " << ops_per_us << endl;
}

void Benchmark::run()
{
  vector<future<Stats>> thread_stats;

  /* create the global and thread contexts */
  rlu::context::Global global_ctx;

  for (size_t i = 0; i < config_.n_threads; i++) {
    global_ctx.threads.emplace_back(
        make_unique<rlu::context::Thread>(i, global_ctx));
  }

  /* create the data structure */
  rlu::List<int32_t> list{config_.initial_size, config_.min_value,
                          config_.max_value};

  /* set the start time */
  cout << "Starting the benchmark in 1 second..." << endl;
  const clock::time_point experiment_start = clock::now() + 1s;
  const clock::time_point experiment_end = experiment_start + config_.duration;

  /* starting the threads */
  for (size_t i = 0; i < config_.n_threads; i++) {
    thread_stats.emplace_back(async(
        launch::async,
        [&](const size_t, rlu::context::Thread &thread_ctx) {
          Stats thread_stats;

          this_thread::sleep_until(experiment_start);
          thread_stats.start = clock::now();

          while (clock::now() < experiment_end) {
            const bool is_writer = coinflip(config_.update_ratio);
            const auto randval = randint(config_.min_value, config_.max_value);

            if (!is_writer) {
              thread_stats.count_found += list.contains(thread_ctx, randval);
              thread_stats.count_contains++;
            }
            else {
              const bool is_adder = coinflip();

              if (is_adder) {
                list.add(thread_ctx, randval);
                thread_stats.count_add++;
              }
              else {
                list.erase(thread_ctx, randval);
                thread_stats.count_erase++;
              }
            }
          }

          thread_stats.end = clock::now();
          return thread_stats;
        },
        i, ref(*global_ctx.threads[i])));
  }

  for (auto &waitable : thread_stats) {
    aggregate_.merge(waitable.get());
  }

  aggregate_.print();
}