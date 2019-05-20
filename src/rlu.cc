#include "rlu.hh"

#include <iostream>

using namespace std;
using namespace rlu::context;

Thread::Thread( const size_t thread_id,
                const shared_ptr<Global>& global_context )
    : thread_id_( thread_id ), global_context_( global_context )
{
  cerr << "Thread(" << thread_id_ << ")" << endl;
}

Thread::~Thread() { cerr << "~Thread(" << thread_id_ << ");" << endl; }
