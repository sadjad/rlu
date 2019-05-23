/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef RLU_HH
#define RLU_HH

#include <array>
#include <atomic>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#define SPECIAL_CONSTANT ((void*)0x1020304050607080)
#define WRITE_LOG_SIZE 1'000'000  // 1 MB

#define OBJ_HEADER(obj)                                             \
  (reinterpret_cast<ObjectHeader*>(reinterpret_cast<uint8_t*>(obj) - \
                                   sizeof(ObjectHeader)))

#define WL_HEADER(obj)                                                     \
  (reinterpret_cast<WriteLogEntryHeader*>(reinterpret_cast<uint8_t*>(obj) - \
                                          sizeof(WriteLogEntryHeader)))

#define GET_COPY(obj) \
  (reinterpret_cast<decltype(obj)>(OBJ_HEADER(obj)->copy.load()))

#define GET_ACTUAL(obj)                                                    \
  (reinterpret_cast<decltype(obj)>(IS_COPY(obj) ? (WL_HEADER(obj)->actual) \
                                                : obj))

#define IS_UNLOCKED(obj) (obj == nullptr)
#define IS_COPY(obj) (obj == SPECIAL_CONSTANT)

namespace rlu {

using Pointer = void*;

struct ObjectHeader {
  std::atomic<Pointer> copy{nullptr};
};

struct WriteLogEntryHeader {
  uint64_t thread_id{0};
  uint64_t object_size{0};
  Pointer actual{nullptr};

  // must be the last one
  ObjectHeader copy{SPECIAL_CONSTANT};
};

namespace context {

class Thread;

class Global {
public:
  std::atomic<uint64_t> clock{0};
  std::vector<std::unique_ptr<Thread>> threads{};

  Global() {}
};

class Thread {
private:
  struct WriteLog {
    size_t pos{0};
    std::array<uint8_t, WRITE_LOG_SIZE> log;

    Pointer append_log(const size_t len, void* buffer);
  };

  const uint64_t thread_id_;
  Global& global_ctx_;

  bool is_writer_{false};
  uint64_t local_clock_{0};
  uint64_t run_count_{0};
  uint64_t write_clock_{std::numeric_limits<uint64_t>::max()};

  WriteLog write_log_{};
  WriteLog write_log_quiesce_{};

public:
  Thread(const size_t thread_id, Global& global_context);
  ~Thread();

  size_t thread_id() const { return thread_id_; }
  uint64_t run_count() const { return run_count_; }
  uint64_t write_clock() const { return write_clock_; }

  void reader_lock();
  void reader_unlock();

  template <class T>
  T* dereference(T* obj);

  template <class T>
  T* try_lock(T* obj, const size_t size);

  template <class T>
  void assign(T*& handle, T* obj);

  bool compare_objects(Pointer obj1, Pointer obj2);
  void commit_write_log();
  void unlock_write_log();
  void writeback_write_log();
  void swap_write_logs();
  void synchronize();
  void abort();
};

template <class T>
void Thread::assign(T*& handle, T* obj)
{
  handle = GET_ACTUAL(obj);
}

template <class T>
T* Thread::dereference(T* ptr)
{
  auto ptr_copy = GET_COPY(ptr);

  if (IS_UNLOCKED(ptr_copy)) return ptr;  // it's free
  if (IS_COPY(ptr_copy)) return ptr;      // it's already a copy

  if (WL_HEADER(ptr_copy)->thread_id == thread_id_)
    return ptr_copy;  // locked by us

  if (global_ctx_.threads[thread_id_]->write_clock_ <= local_clock_) {
    return ptr_copy; /* let's steal this copy */
  }
  else {
    return ptr; /* no stealing */
  }
}

template <class T>
T* Thread::try_lock(T* ptr, const size_t size)
{
  is_writer_ = true;
  ptr = GET_ACTUAL(ptr);          // read the actual object
  auto ptr_copy = GET_COPY(ptr);  // read the copy

  if (!IS_UNLOCKED(ptr_copy)) {
    const WriteLogEntryHeader* wl_header = WL_HEADER(ptr_copy);
    if (wl_header->thread_id == thread_id_) {
      return ptr_copy;  // it's locked by us, let's send our copy
    }

    this->abort();
  }

  WriteLogEntryHeader entry;
  entry.thread_id = thread_id_;
  entry.actual = ptr;
  entry.object_size = sizeof(ptr);

  ptr_copy = reinterpret_cast<T*>(write_log_.append_log(sizeof(entry), &entry));

  void* expected = nullptr;

  if (!OBJ_HEADER(ptr)->copy.compare_exchange_weak(expected, ptr_copy)) {
    throw std::runtime_error("compare-exchange failed");
  }

  write_log_.append_log(size, ptr);
  return ptr_copy;
}

}  // namespace context

namespace mem {

template <class T>
T* alloc()
{
  auto ptr =
      reinterpret_cast<uint8_t*>(malloc(sizeof(ObjectHeader) + sizeof(T)));

  if (ptr != nullptr) {
    new (ptr) ObjectHeader{};
    return new (ptr + sizeof(ObjectHeader)) T{};
  }

  return reinterpret_cast<T*>(ptr);
}

void free(Pointer ptr);

}  // namespace mem

}  // namespace rlu

#endif /* RLU_HH */
