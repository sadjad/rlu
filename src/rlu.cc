#include "rlu.hh"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace rlu;
using namespace rlu::context;

Thread::Thread(const size_t thread_id, Global& global_context)
    : thread_id_(thread_id), global_ctx_(global_context)
{
}

Thread::~Thread() {}

void Thread::reader_lock()
{
  is_writer_ = false;
  run_count_++;

  local_clock_ = global_ctx_.clock.load();
}

void Thread::reader_unlock()
{
  run_count_++;

  if (is_writer_) {
    commit_write_log();
  }
}

bool Thread::compare_objects(Pointer obj1, Pointer obj2)
{
  return util::get_actual(obj1) == util::get_actual(obj2);
}

void Thread::writeback_write_log()
{
  uint8_t* dataPtr = write_log_.log;
  const uint8_t* end = dataPtr + write_log_.pos;

  while (dataPtr < end) {
    auto header = reinterpret_cast<WriteLogEntryHeader*>(dataPtr);
    dataPtr += sizeof(WriteLogEntryHeader);

    memcpy(header->actual, dataPtr, header->object_size);
    util::object_header(header->actual)
        ->copy.store(nullptr);  // Unlock the object
    header->~WriteLogEntryHeader();

    dataPtr += header->object_size;
  }
}

void Thread::unlock_write_log()
{
  uint8_t* dataPtr = write_log_.log;
  uint8_t* end = dataPtr + write_log_.pos;

  while (dataPtr < end) {
    auto header = reinterpret_cast<WriteLogEntryHeader*>(dataPtr);
    dataPtr += sizeof(WriteLogEntryHeader) + header->object_size;

    util::object_header(header->actual)
        ->copy.store(nullptr);  // Unlock the object
  }
}

void Thread::commit_write_log()
{
  write_clock_ = global_ctx_.clock.load() + 1;
  global_ctx_.clock.fetch_add(1);

  synchronize();
  writeback_write_log();

  write_clock_ = numeric_limits<uint64_t>::max();
  swap_write_logs();
}

void Thread::swap_write_logs()
{
  auto ptr_copy = write_log_.log;
  write_log_.log = write_log_quiesce_.log;
  write_log_quiesce_.log = ptr_copy;
  write_log_quiesce_.pos = write_log_.pos;
  write_log_.pos = 0;
}

void Thread::synchronize()
{
  uint64_t sync_counts[MAX_THREADS];

  for (const auto& thread : global_ctx_.threads) {
    sync_counts[thread->thread_id_] = thread->run_count_;
  }

  for (const auto& thread : global_ctx_.threads) {
    if (thread->thread_id_ == thread_id_) continue;

    while (sync_counts[thread->thread_id_] % 2 != 0) {
      if (sync_counts[thread->thread_id_] != thread->run_count_) break;
      if (write_clock_ <= thread->local_clock_) break;
    }
  }
}

void Thread::abort()
{
  run_count_++;

  if (is_writer_) {
    unlock_write_log();
    write_log_.pos = 0;
  }
}
