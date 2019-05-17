/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include <atomic>
#include <limits>
#include <thread>
#include <vector>

namespace rlu {
namespace metadata {

class Global {
    std::atomic<uint64_t> clock{0};
};

class Thread {
    struct WriteLogEntry {
        std::thread::id thread_id{std::this_thread::get_id()};
        void* pointer{nullptr};
        size_t object_size{0};
        void* copy{nullptr};
    };

    uint64_t local_clock{0};
    uint64_t write_clock{std::numeric_limits<uint64_t>::max()};

    std::vector<WriteLogEntry> write_log{};
};

class Object {
    void* copy{nullptr};
};

}  // namespace metadata
}  // namespace rlu
