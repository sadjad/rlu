#include <iostream>
#include <memory>
#include <thread>

#include "list.hh"
#include "rlu.hh"

using namespace std;

const size_t NUM_THREADS = 10;

int main( const int, char*[] )
{
  vector<thread> threads;

  rlu::List<int32_t> list;
  rlu::context::Global global_ctx;

  for ( size_t i = 0; i < NUM_THREADS; i++ ) {
    global_ctx.threads.emplace_back(
        make_unique<rlu::context::Thread>( i, global_ctx ) );
  }

  for ( size_t i = 0; i < NUM_THREADS; i++ ) {
    threads.emplace_back(
        [&global_ctx, &list]( const size_t thread_id ) {
          auto& thread_ctx = global_ctx.threads[thread_id];
        },
        i );
  }

  for ( size_t i = 0; i < NUM_THREADS; i++ ) {
    threads[i].join();
  }

  return EXIT_SUCCESS;
}
