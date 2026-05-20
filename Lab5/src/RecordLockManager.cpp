#include "RecordLockManager.h"

namespace lab5 {

void RecordLockManager::Acquire(int id, LockMode mode) {
    std::unique_lock<std::mutex> lock(mutex_);
    LockState& state = states_[id];

    if (mode == LockMode::Read) {
        cv_.wait(lock, [&state]() {
            return !state.writer_active && state.pending_writers == 0;
        });
        ++state.readers;
        return;
    }

    ++state.pending_writers;
    cv_.wait(lock, [&state]() {
        return !state.writer_active && state.readers == 0;
    });
    --state.pending_writers;
    state.writer_active = true;
}

void RecordLockManager::Release(int id, LockMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = states_.find(id);
    if (it == states_.end()) {
        return;
    }

    LockState& state = it->second;
    if (mode == LockMode::Read) {
        if (state.readers > 0) {
            --state.readers;
        }
    } else {
        state.writer_active = false;
    }

    if (state.readers == 0 && !state.writer_active && state.pending_writers == 0) {
        states_.erase(it);
    }

    cv_.notify_all();
}

}  // namespace lab5
