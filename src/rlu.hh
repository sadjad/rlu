/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef RLU_HH
#define RLU_HH

#include <atomic>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

namespace rlu {

using Pointer = void*;

struct ObjectHeader {
  Pointer copy{nullptr};
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
  struct WriteLogEntry {
    size_t object_size{0};
    void* pointer{nullptr};
    void* copy{nullptr};
  };

  const size_t thread_id_;
  Global& global_ctx_;

  bool is_writer_{false};
  uint64_t local_clock_{0};
  uint64_t run_count_{0};
  uint64_t write_clock_{std::numeric_limits<uint64_t>::max()};

  std::vector<WriteLogEntry> write_log_{};

public:
  Thread( const size_t thread_id, Global& global_context );
  ~Thread();

  size_t thread_id() const { return thread_id_; }

  void reader_lock();
  void reader_unlock();

  Pointer dereference( Pointer obj );
  Pointer try_lock( Pointer obj );

  bool compare_objects( Pointer obj1, Pointer obj_2 );
  void assign( Pointer handle, Pointer obj );
  void commit_write_log();
  void synchronize();
  void swap_write_logs();
  void abort();
};

}  // namespace context

namespace mem {

Pointer alloc( const size_t len );
void free( Pointer ptr );

}  // namespace mem

}  // namespace rlu

#endif /* RLU_HH */
