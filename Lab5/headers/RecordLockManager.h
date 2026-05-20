#pragma once

#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace lab5 {

enum class LockMode {
    Read,
    Write
};

class RecordLockManager {
public:
    void Acquire(int id, LockMode mode);
    void Release(int id, LockMode mode);

private:
    struct LockState {
        int readers = 0;
        int pending_writers = 0;
        bool writer_active = false;
    };

    std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<int, LockState> states_;
};

}  // namespace lab5
