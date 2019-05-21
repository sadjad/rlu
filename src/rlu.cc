#include "rlu.hh"

#include <cstdlib>
#include <iostream>

using namespace std;
using namespace rlu;
using namespace rlu::context;

Pointer mem::alloc(const size_t len)
{
  auto ptr =
      reinterpret_cast<ObjectHeader*>(malloc(sizeof(ObjectHeader) + len));

  if (ptr != nullptr) {
    ptr->copy = nullptr;
    return (ptr + sizeof(ObjectHeader));
  }

  return ptr;
}

void mem::free(Pointer ptr)
{
  if (ptr == nullptr) {
    return;
  }

  free(OBJ_HEADER(ptr));
}

Thread::Thread(const size_t thread_id, Global& global_context)
    : thread_id_(thread_id), global_ctx_(global_context)
{
  cerr << "Thread(" << thread_id_ << ")" << endl;
}

Thread::~Thread() { cerr << "~Thread(" << thread_id_ << ")" << endl; }

void Thread::reader_lock()
{
  is_writer_ = false;
  run_count_++;

  local_clock_ = global_ctx_.clock.load(memory_order_consume);
}

void Thread::reader_unlock()
{
  run_count_++;

  if (is_writer_) {
    commit_write_log();
  }
}

Pointer Thread::dereference(Pointer ptr)
{
  auto ptr_copy = GET_COPY(ptr);

  if (IS_UNLOCKED(ptr_copy)) {
    return ptr;  // it's free
  }

  if (IS_COPY(ptr_copy)) {
    return ptr;  // it's already a copy
  }

  const WriteLogEntryHeader* ws_header = WS_HEADER(ptr_copy);

  if (ws_header->thread_id == thread_id_) {
    return ptr_copy; // it's locked by us!
  }

  const auto &other_ctx = global_ctx_.threads[thread_id_];
  if (other_ctx->write_clock_ <= local_clock_) {
    return ptr_copy; /* let's steal this copy */
  }
  else {
    return ptr;/* no stealing */
  }
}
