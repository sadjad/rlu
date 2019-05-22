/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef RLU_HH
#define RLU_HH

#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

#define SPECIAL_CONSTANT ((void*)0x1020304050607080)
#define WRITE_LOG_SIZE 1'000'000  // 1 MB

#define OBJ_HEADER(obj)                                          \
  reinterpret_cast<ObjectHeader*>(reinterpret_cast<char*>(obj) - \
                                  sizeof(ObjectHeader))

#define WS_HEADER(obj)                                                  \
  reinterpret_cast<WriteLogEntryHeader*>(reinterpret_cast<char*>(obj) - \
                                         sizeof(WriteLogEntryHeader))

#define GET_COPY(obj) OBJ_HEADER(obj)->copy
#define IS_UNLOCKED(obj) (OBJ_HEADER(obj)->copy == nullptr)
#define IS_COPY(obj) (obj == SPECIAL_CONSTANT)

#define GET_ACTUAL(obj) (IS_COPY(obj) ? (WS_HEADER(obj)->actual) : obj)

namespace rlu {

using Pointer = void*;

struct ObjectHeader {
  Pointer copy{nullptr};
};

struct WriteLogEntryHeader {
  uint64_t thread_id{0};
  uint64_t object_size{0};
  Pointer actual{nullptr};

  // must be the last one
  Pointer copy{SPECIAL_CONSTANT};
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
    void write_back();
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

  void reader_lock();
  void reader_unlock();

  Pointer dereference(Pointer obj);
  Pointer try_lock(Pointer obj, const size_t size);

  bool compare_objects(Pointer obj1, Pointer obj2);
  void assign(Pointer& handle, Pointer obj);
  void commit_write_log();
  void unlock_write_log();
  void swap_write_logs();
  void synchronize();
  void abort();
};

}  // namespace context

namespace mem {

Pointer alloc(const size_t len);
void free(Pointer ptr);

}  // namespace mem

}  // namespace rlu

#endif /* RLU_HH */
