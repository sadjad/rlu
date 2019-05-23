#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "list.hh"
#include "rlu.hh"

using namespace std;

constexpr size_t NUM_THREADS = 128;

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
          ostringstream oss;

          this_thread::sleep_for(chrono::milliseconds{10 * thread_id / 8});

          if (is_reader) {
            thread_ctx.reader_lock();

            oss << "[read:" << thread_id << "]";
            auto current = list.head();

            int32_t val = numeric_limits<int32_t>::min();

            while (current != nullptr) {
              current = thread_ctx.dereference(current);
              oss << " " << current->value;

              if (current->value < val) {
                throw runtime_error("inconsistent list");
              }

              val = current->value;
              current = current->next;
            }

            oss << endl;
            cerr << oss.str();

            if (val != numeric_limits<int32_t>::max()) {
              throw runtime_error("inconsistent list");
            }

            thread_ctx.reader_unlock();
          }
          else /* it's a writer */ {
            list.add(thread_ctx, 8 * thread_id);
            list.add(thread_ctx, 8 * thread_id + 2);
            list.add(thread_ctx, 8 * thread_id + 4);
            list.add(thread_ctx, 8 * thread_id + 6);
          }
        },
        i, i % 2);
  }

  for (size_t i = 0; i < NUM_THREADS; i++) {
    threads[i].join();
  }

  return EXIT_SUCCESS;
}
