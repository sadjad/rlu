#include "rlu.hh"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

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

Pointer Thread::WriteLog::append_log(const size_t len, void* buffer)
{
  if (len + pos >= log.size()) {
    /* log full */
    throw runtime_error("write log full");
  }

  memcpy(log.data() + pos, buffer, len);
  pos += len;

  return log.data() + pos;
}

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
    return ptr_copy;  // it's locked by us!
  }

  const auto& other_ctx = global_ctx_.threads[thread_id_];
  if (other_ctx->write_clock_ <= local_clock_) {
    return ptr_copy; /* let's steal this copy */
  }
  else {
    return ptr; /* no stealing */
  }
}

Pointer Thread::try_lock(Pointer ptr, const size_t size)
{
  is_writer_ = true;
  ptr = GET_ACTUAL(ptr);  // read the actual object

  auto ptr_copy = GET_COPY(ptr);

  if (!IS_UNLOCKED(ptr_copy)) {
    const WriteLogEntryHeader* ws_header = WS_HEADER(ptr_copy);
    if (ws_header->thread_id == thread_id_) {
      return ptr_copy;  // it's locked by us, let's send our copy
    }

    this->abort();
  }

  WriteLogEntryHeader entry;
  entry.thread_id = thread_id_;
  entry.actual = ptr;
  entry.object_size = sizeof(ptr);

  ptr_copy = write_log_.append_log(sizeof(entry), &entry);

  Pointer expected = nullptr;

  if (!OBJ_HEADER(ptr)->copy.compare_exchange_weak(expected, ptr_copy)) {
    throw runtime_error("compare-exchange failed");
  }

  write_log_.append_log(size, ptr);
  return ptr_copy;
}

bool Thread::compare_objects(Pointer obj1, Pointer obj2)
{
  return GET_ACTUAL(obj1) == GET_ACTUAL(obj2);
}

void Thread::assign(Pointer& handle, Pointer obj) { handle = GET_ACTUAL(obj); }

void Thread::commit_write_log()
{
  write_clock_ = global_ctx_.clock.load(memory_order_consume) + 1;
  global_ctx_.clock.fetch_add(1);

  synchronize();
  write_log_.write_back();
  unlock_write_log();

  write_clock_ = numeric_limits<uint64_t>::max();
  swap_write_logs();
}

void Thread::swap_write_logs() { swap(write_log_, write_log_quiesce_); }

void Thread::synchronize()
{
  vector<uint64_t> sync_counts{global_ctx_.threads.size(), 0};

  for (const auto& thread : global_ctx_.threads) {
    sync_counts[thread->thread_id()] = thread->run_count();
  }

  for (const auto& thread : global_ctx_.threads) {
    while (true) {
      if (sync_counts[thread->thread_id()] % 2 == 0) break;
      if (sync_counts[thread->thread_id()] != thread->run_count()) break;
      if (write_clock_ <= thread->write_clock()) break;
    }
  }
}

void Thread::abort()
{
  run_count_++;
  if (is_writer_) {
    unlock_write_log();
  }

  /* XXX retry!? */
}
