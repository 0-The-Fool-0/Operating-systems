#include <windows.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include "QueueCommon.h"
#include "QueueFile.h"

namespace {

class ScopedHandle {
public:
    ScopedHandle() = default;
    explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
    ~ScopedHandle() {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
    }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    ScopedHandle(ScopedHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
                CloseHandle(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    HANDLE get() const { return handle_; }
    bool valid() const { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }

private:
    HANDLE handle_ = nullptr;
};

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            throw std::invalid_argument("Usage: Sender.exe <binary_file_name>");
        }

        const std::string fileName = argv[1];
        ScopedHandle file(CreateFileA(
            fileName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr));
        if (!file.valid()) {
            throw std::runtime_error(queuefile::FormatWindowsError("Cannot open queue file", GetLastError()));
        }

        auto names = BuildSyncNames(fileName);
        ScopedHandle mutexHandle(OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, names.mutexName.c_str()));
        ScopedHandle emptySem(OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, names.emptySemaphoreName.c_str()));
        ScopedHandle fullSem(OpenSemaphoreA(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, names.fullSemaphoreName.c_str()));
        ScopedHandle readySem(OpenSemaphoreA(SEMAPHORE_MODIFY_STATE, FALSE, names.readySemaphoreName.c_str()));
        if (!mutexHandle.valid() || !emptySem.valid() || !fullSem.valid() || !readySem.valid()) {
            throw std::runtime_error(queuefile::FormatWindowsError(
                "Failed to open synchronization objects", GetLastError()));
        }

        if (ReleaseSemaphore(readySem.get(), 1, nullptr) == 0) {
            throw std::runtime_error(queuefile::FormatWindowsError("Failed to send readiness signal", GetLastError()));
        }

        std::cout << "Sender connected to file: " << fileName << "\n";
        for (;;) {
            std::cout << "\nCommands: send / quit\n> ";
            std::string cmd;
            std::getline(std::cin, cmd);

            if (cmd == "quit" || cmd == "q") {
                break;
            }
            if (cmd != "send" && cmd != "s") {
                std::cout << "Unknown command.\n";
                continue;
            }

            std::cout << "Enter message (<20 chars): ";
            std::string message;
            std::getline(std::cin, message);
            if (message.size() >= static_cast<size_t>(kMaxMessageLen)) {
                std::cout << "Message too long. Max is 19 characters.\n";
                continue;
            }

            WaitForSingleObject(emptySem.get(), INFINITE);
            WaitForSingleObject(mutexHandle.get(), INFINITE);
            try {
                queuefile::PushMessage(file.get(), message);
                std::cout << "Message sent.\n";
            } catch (const std::exception& ex) {
                std::cerr << "Write failed: " << ex.what() << "\n";
            }
            ReleaseMutex(mutexHandle.get());
            ReleaseSemaphore(fullSem.get(), 1, nullptr);
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Sender error: " << ex.what() << "\n";
        return 1;
    }
}
