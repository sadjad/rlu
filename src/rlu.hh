/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef RLU_HH
#define RLU_HH

#include <array>
#include <atomic>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace rlu {

constexpr intptr_t SPECIAL_CONSTANT = 0x1020304050607080ull;
constexpr size_t WRITE_LOG_SIZE = 1024 * 1024;  // 1 MB
constexpr size_t MAX_THREADS = 256;

using Pointer = void*;

struct ObjectHeader {
  std::atomic<Pointer> copy{nullptr};
};

struct WriteLogEntryHeader {
  uint64_t thread_id{0};
  uint64_t object_size{0};
  Pointer actual{nullptr};

  // must be the last one
  ObjectHeader copy{reinterpret_cast<void*>(SPECIAL_CONSTANT)};
};

namespace util {

template <class T>
inline ObjectHeader* object_header(T* obj)
{
  return reinterpret_cast<ObjectHeader*>(reinterpret_cast<uint8_t*>(obj) -
                                         sizeof(ObjectHeader));
}

template <class T>
inline WriteLogEntryHeader* writelog_header(T* obj)
{
  return reinterpret_cast<WriteLogEntryHeader*>(
      reinterpret_cast<uint8_t*>(obj) - sizeof(WriteLogEntryHeader));
}

template <class T>
inline T* get_copy(T* obj)
{
  return reinterpret_cast<T*>(
      object_header(obj)->copy.load(std::memory_order_consume));
}

template <class T>
inline bool is_copy(T* obj)
{
  return obj == reinterpret_cast<void*>(SPECIAL_CONSTANT);
}

template <class T>
inline T* get_actual(T* obj)
{
  return reinterpret_cast<T*>(
      is_copy(get_copy(obj)) ? (writelog_header(obj)->actual) : obj);
}

template <class T>
inline bool is_unlocked(T* obj)
{
  return obj == nullptr;
}

}  // namespace util

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
    uint8_t* log{nullptr};

    WriteLog() { log = new uint8_t[WRITE_LOG_SIZE]; }
    ~WriteLog() { delete[] log; }

    WriteLog(const WriteLog&) = delete;
    WriteLog& operator=(const WriteLog&) = delete;

    template <class T>
    T* append_header(const uint64_t thread_id, T* ptr);

    template <class T>
    void append_log(T* obj);
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
  bool try_lock(T*& obj);

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
  handle = util::get_actual(obj);
}

template <class T>
T* Thread::dereference(T* ptr)
{
  auto ptr_copy = util::get_copy(ptr);

  if (util::is_unlocked(ptr_copy)) return ptr;  // it's free
  if (util::is_copy(ptr_copy)) return ptr;      // it's already a copy

  const auto other_id = util::writelog_header(ptr_copy)->thread_id;

  if (other_id == thread_id_) return ptr_copy;  // locked by us

  if (global_ctx_.threads[other_id]->write_clock_ <= local_clock_) {
    return ptr_copy; /* let's steal this copy */
  }
  else {
    return ptr; /* no stealing */
  }
}

template <class T>
bool Thread::try_lock(T*& original_ptr)
{
  is_writer_ = true;

  T* ptr = original_ptr;
  ptr = util::get_actual(ptr);          // read the actual object
  auto ptr_copy = util::get_copy(ptr);  // read the copy

  if (!util::is_unlocked(ptr_copy)) {
    const auto wl_header = util::writelog_header(ptr_copy);
    if (wl_header->thread_id == thread_id_) {
      original_ptr = ptr_copy;  // it's locked by us, let's send our copy
      return true;
    }

    this->abort();
    return false;
  }

  ptr_copy = write_log_.append_header(thread_id_, ptr);
  void* expt = nullptr;

  if (!util::object_header(ptr)->copy.compare_exchange_weak(
          expt, ptr_copy, std::memory_order_release)) {
    this->abort();
    return false;
  }

  write_log_.append_log(ptr);
  original_ptr = ptr_copy;

  return true;
}

template <class T>
T* Thread::WriteLog::append_header(const uint64_t thread_id, T* ptr)
{
  if (pos + sizeof(WriteLogEntryHeader) >= WRITE_LOG_SIZE) {
    throw std::runtime_error("write log full");
  }

  auto wl_header = new (log + pos) WriteLogEntryHeader;
  wl_header->thread_id = thread_id;
  wl_header->actual = ptr;
  wl_header->object_size = sizeof(T);

  pos += sizeof(WriteLogEntryHeader);

  return reinterpret_cast<T*>(log + pos);
}

template <class T>
void Thread::WriteLog::append_log(T* obj)
{
  if (pos + sizeof(T) >= WRITE_LOG_SIZE) {
    throw std::runtime_error("write log full");
  }

  *reinterpret_cast<T*>(log + pos) = *obj;
  pos += sizeof(T);
}

}  // namespace context

namespace mem {

template <class T, typename... Args>
T* alloc(Args&&... args)
{
  auto ptr =
      reinterpret_cast<uint8_t*>(malloc(sizeof(ObjectHeader) + sizeof(T)));

  if (ptr != nullptr) {
    new (ptr) ObjectHeader;
    return new (ptr + sizeof(ObjectHeader)) T(std::forward<Args>(args)...);
  }

  return reinterpret_cast<T*>(ptr);
}

template <class T>
void free(T* ptr)
{
  if (ptr == nullptr) return;

  ptr->~T();
  util::object_header(ptr)->~ObjectHeader();
  free(util::object_header(ptr));
}

}  // namespace mem

}  // namespace rlu

#endif /* RLU_HH */
