/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef RLU_HH
#define RLU_HH

#include <atomic>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

namespace rlu {
namespace metadata {

using Pointer = void*;

class Thread;

class Global {
public:
  std::atomic<uint64_t> clock{0};
  std::vector<std::unique_ptr<Thread>> threads{};

  Global();
  ~Global();
};

class Thread {
private:
  struct WriteLogEntry {
    std::thread::id thread_id{std::this_thread::get_id()};

    size_t object_size{0};
    void* pointer{nullptr};
    void* copy{nullptr};
  };

  std::shared_ptr<Global> global_context_;

  uint64_t local_clock_{0};
  uint64_t write_clock_{std::numeric_limits<uint64_t>::max()};

  std::vector<WriteLogEntry> write_log_{};

public:
  Thread(const std::shared_ptr<Global>& global_context);
  ~Thread();

  void reader_lock();
  void reader_unlock();

  Pointer dereference(Pointer obj);
  Pointer try_lock(Pointer obj);

  bool compare_objects(Pointer obj1, Pointer obj_2);
  void assign(Pointer handle, Pointer obj);
  void commit_write_log();
  void synchronize();
  void swap_write_logs();
  void abort(Pointer obj);
};

class Object {
private:
  void* copy_{nullptr};
};

Pointer alloc(const size_t len);
void free(Pointer ptr);

}  // namespace metadata
}  // namespace rlu

#endif /* RLU_HH */
