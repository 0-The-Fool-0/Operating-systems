#include <windows.h>

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

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

int ReadPositiveInt(const std::string& prompt) {
    std::cout << prompt;
    int value = 0;
    if (!(std::cin >> value) || value <= 0) {
        throw std::invalid_argument("Input must be a positive integer.");
    }
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    return value;
}

}  // namespace

int main() {
    try {
        std::string fileName;
        std::cout << "Receiver\n";
        std::cout << "Enter binary file name: ";
        std::getline(std::cin, fileName);
        if (fileName.empty()) {
            throw std::invalid_argument("File name cannot be empty.");
        }

        const int recordsCount = ReadPositiveInt("Enter number of records in file: ");

        if (GetFileAttributesA(fileName.c_str()) != INVALID_FILE_ATTRIBUTES) {
            std::cout << "File already exists. Overwrite? (y/n): ";
            std::string answer;
            std::getline(std::cin, answer);
            if (answer != "y" && answer != "Y") {
                std::cout << "Cancelled.\n";
                return 0;
            }
        }

        ScopedHandle file(CreateFileA(
            fileName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr));
        if (!file.valid()) {
            throw std::runtime_error(queuefile::FormatWindowsError("Cannot create file", GetLastError()));
        }
        queuefile::InitializeQueueFile(file.get(), recordsCount);

        auto names = BuildSyncNames(fileName);
        ScopedHandle mutexHandle(CreateMutexA(nullptr, FALSE, names.mutexName.c_str()));
        ScopedHandle emptySem(CreateSemaphoreA(nullptr, recordsCount, recordsCount, names.emptySemaphoreName.c_str()));
        ScopedHandle fullSem(CreateSemaphoreA(nullptr, 0, recordsCount, names.fullSemaphoreName.c_str()));
        ScopedHandle readySem(CreateSemaphoreA(nullptr, 0, 32767, names.readySemaphoreName.c_str()));
        if (!mutexHandle.valid() || !emptySem.valid() || !fullSem.valid() || !readySem.valid()) {
            throw std::runtime_error(queuefile::FormatWindowsError(
                "Failed to create synchronization objects", GetLastError()));
        }

        const int senderCount = ReadPositiveInt("Enter number of sender processes: ");

        std::vector<PROCESS_INFORMATION> senders;
        senders.reserve(senderCount);
        for (int i = 0; i < senderCount; ++i) {
            STARTUPINFOA si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            std::string cmd = "Sender.exe \"" + fileName + "\"";
            std::vector<char> buffer(cmd.begin(), cmd.end());
            buffer.push_back('\0');
            if (!CreateProcessA(
                    nullptr, buffer.data(), nullptr, nullptr, FALSE,
                    CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
                std::cerr << "Failed to start Sender #" << (i + 1) << ". "
                          << queuefile::FormatWindowsError("CreateProcessA failed", GetLastError()) << "\n";
                continue;
            }
            senders.push_back(pi);
        }

        std::cout << "Waiting for sender readiness...\n";
        for (size_t i = 0; i < senders.size(); ++i) {
            WaitForSingleObject(readySem.get(), INFINITE);
        }
        std::cout << "All started senders are ready.\n";

        for (;;) {
            std::cout << "\nCommands: read / quit\n> ";
            std::string cmd;
            std::getline(std::cin, cmd);

            if (cmd == "quit" || cmd == "q") {
                break;
            }
            if (cmd != "read" && cmd != "r") {
                std::cout << "Unknown command.\n";
                continue;
            }

            WaitForSingleObject(fullSem.get(), INFINITE);
            WaitForSingleObject(mutexHandle.get(), INFINITE);
            try {
                const std::string message = queuefile::PopMessage(file.get());
                std::cout << "Message: " << message << "\n";
            } catch (const std::exception& ex) {
                std::cerr << "Read failed: " << ex.what() << "\n";
            }
            ReleaseMutex(mutexHandle.get());
            ReleaseSemaphore(emptySem.get(), 1, nullptr);
        }

        for (auto& pi : senders) {
            if (pi.hThread != nullptr) {
                CloseHandle(pi.hThread);
            }
            if (pi.hProcess != nullptr) {
                CloseHandle(pi.hProcess);
            }
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Receiver error: " << ex.what() << "\n";
        return 1;
    }
}
