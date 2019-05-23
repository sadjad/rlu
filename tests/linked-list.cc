#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "list.hh"
#include "rlu.hh"

using namespace std;

constexpr size_t NUM_THREADS = 5;

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
        [&global_ctx, &list](const size_t thread_id, const bool reader) {
          auto& thread_ctx = *global_ctx.threads[thread_id];
          ostringstream oss;

          if (thread_id == 2) {
            this_thread::sleep_for(1s);
          }

          if (reader) {
            thread_ctx.reader_lock();

            auto current = list.head();

            oss << "[read:" << thread_id << "]";
            while (current != nullptr) {
              current = thread_ctx.dereference(current);
              oss << " " << current->value;
              current = current->next;
            }
            oss << endl;

            cerr << oss.str();

            thread_ctx.reader_unlock();
          }
          else /* it's a writer */ {
            list.add(thread_ctx, 10);
          }
        },
        i, (i != 4));
  }

  for (size_t i = 0; i < NUM_THREADS; i++) {
    threads[i].join();
  }

  return EXIT_SUCCESS;
}
