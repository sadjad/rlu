#include <iostream>
#include <memory>
#include <thread>

#include "list.hh"
#include "rlu.hh"

using namespace std;

constexpr size_t NUM_THREADS = 1;

int main(const int, char*[])
{
  vector<thread> threads;

  rlu::List<int32_t> list;
  rlu::context::Global global_ctx;

  for (size_t i = 0; i < NUM_THREADS; i++) {
    global_ctx.threads.emplace_back(
        make_unique<rlu::context::Thread>(i, global_ctx));
  }

  auto x = list.head();
  while (x != nullptr) {
    cout << x->value << endl;
    x = x->next;
  }

  return 0;

  for (size_t i = 0; i < NUM_THREADS; i++) {
    threads.emplace_back(
        [&global_ctx, &list](const size_t thread_id, const bool reader) {
          auto& thread_ctx = *global_ctx.threads[thread_id];

          if (reader) {
            thread_ctx.reader_lock();

            cout << list.head() << endl;

            auto current = thread_ctx.dereference(list.head());

            cout << current << endl;

            while (current != nullptr) {
              cerr << "Thread(" << thread_id << ") [read] " << current->value
                   << endl;

              current = thread_ctx.dereference(current->next);
            }

            thread_ctx.reader_unlock();
          }
          else {
          }
        },
        i, true);
  }

  for (size_t i = 0; i < NUM_THREADS; i++) {
    threads[i].join();
  }

  return EXIT_SUCCESS;
}
