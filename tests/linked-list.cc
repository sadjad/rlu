#include <memory>
#include <thread>

#include "rlu.hh"

using namespace std;

const size_t NUM_THREADS = 10;

int main( const int, char*[] )
{
  vector<thread> threads;
  rlu::context::Global global_context;

  for ( size_t i = 0; i < NUM_THREADS; i++ ) {
    global_context.threads.emplace_back(
        make_unique<rlu::context::Thread>( i, global_context ) );
  }

  for ( size_t i = 0; i < NUM_THREADS; i++ ) {
    threads.emplace_back(
        [&global_context]( const size_t thread_id ) {
          auto& context = global_context.threads[thread_id];
          context->thread_id();
        },
        i );
  }

  for ( size_t i = 0; i < NUM_THREADS; i++ ) {
    threads[i].join();
  }

  return EXIT_SUCCESS;
}
