#include <iostream>
#include <memory>
#include <random>
#include <thread>

#include "list.hh"
#include "rlu.hh"

using namespace std;

constexpr size_t NUM_THREADS = 128;

int32_t randint()
{
  static random_device dev;
  static mt19937 rng{dev()};
  static uniform_int_distribution<int32_t> distribution{-2048, 2048};

  return distribution(rng);
}

int main(const int, char*[])
{
  vector<thread> threads;

  rlu::List<int32_t> list;
  rlu::context::Global global_ctx;

  for (size_t i = 0; i < NUM_THREADS; i++) {
    global_ctx.threads.emplace_back(
        make_unique<rlu::context::Thread>(i, global_ctx));
  }

  for (size_t i = 0; i < NUM_THREADS; i++) {
    threads.emplace_back(
        [&global_ctx, &list](const size_t thread_id, const bool is_reader) {
          auto& thread_ctx = *global_ctx.threads[thread_id];

          this_thread::sleep_for(chrono::milliseconds{100 * thread_id / 8});

          if (is_reader) {
            for (size_t i = 0; i < 1000; i++) {
              thread_ctx.reader_lock();

              int32_t val = numeric_limits<int32_t>::min();

              for (auto node = list.head(); node; node = node->next) {
                node = thread_ctx.dereference(node);

                if (node->value < val) {
                  throw runtime_error("inconsistent list");
                }

                val = node->value;
              }

              if (val != numeric_limits<int32_t>::max()) {
                throw runtime_error("inconsistent list");
              }

              thread_ctx.reader_unlock();

              if (i % 100 == 0) cerr << 'R';
            }
          }
          else /* it's a writer */ {
            for (int i = 0; i < 1000; i++) {
              list.add(thread_ctx, randint());
              if (i % 100 == 0) cerr << 'W';
            }
          }
        },
        i, i % 32);
  }

  for (size_t i = 0; i < NUM_THREADS; i++) {
    threads[i].join();
  }

  cerr << endl;

  return EXIT_SUCCESS;
}
