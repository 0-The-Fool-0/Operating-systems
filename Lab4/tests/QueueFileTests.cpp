#include <windows.h>

#include <cstdio>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "QueueFile.h"

namespace {

class ScopedHandle {
public:
    explicit ScopedHandle(HANDLE handle) : handle_(handle) {}
    ~ScopedHandle() {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(handle_);
        }
    }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    HANDLE get() const { return handle_; }

private:
    HANDLE handle_ = nullptr;
};

std::string MakeTempFileName() {
    char pathBuffer[MAX_PATH]{};
    DWORD length = GetTempPathA(MAX_PATH, pathBuffer);
    if (length == 0 || length > MAX_PATH) {
        throw std::runtime_error("GetTempPathA failed.");
    }

    char fileBuffer[MAX_PATH]{};
    if (GetTempFileNameA(pathBuffer, "l4q", 0, fileBuffer) == 0) {
        throw std::runtime_error("GetTempFileNameA failed.");
    }
    return std::string(fileBuffer);
}

ScopedHandle OpenTempFile(const std::string& fileName) {
    HANDLE file = CreateFileA(
        fileName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("CreateFileA failed in test.");
    }
    return ScopedHandle(file);
}

}  // namespace

TEST(QueueFileTests, PreservesFifoOrderWithWrapAround) {
    const std::string fileName = MakeTempFileName();
    {
        auto file = OpenTempFile(fileName);
        queuefile::InitializeQueueFile(file.get(), 3);

        queuefile::PushMessage(file.get(), "m1");
        queuefile::PushMessage(file.get(), "m2");
        queuefile::PushMessage(file.get(), "m3");

        EXPECT_EQ(queuefile::PopMessage(file.get()), "m1");
        queuefile::PushMessage(file.get(), "m4");

        EXPECT_EQ(queuefile::PopMessage(file.get()), "m2");
        EXPECT_EQ(queuefile::PopMessage(file.get()), "m3");
        EXPECT_EQ(queuefile::PopMessage(file.get()), "m4");
    }
    std::remove(fileName.c_str());
}

TEST(QueueFileTests, ThrowsOnOverflow) {
    const std::string fileName = MakeTempFileName();
    {
        auto file = OpenTempFile(fileName);
        queuefile::InitializeQueueFile(file.get(), 1);
        queuefile::PushMessage(file.get(), "first");
        EXPECT_THROW(queuefile::PushMessage(file.get(), "second"), std::runtime_error);
    }
    std::remove(fileName.c_str());
}

TEST(QueueFileTests, ThrowsOnUnderflow) {
    const std::string fileName = MakeTempFileName();
    {
        auto file = OpenTempFile(fileName);
        queuefile::InitializeQueueFile(file.get(), 2);
        EXPECT_THROW(queuefile::PopMessage(file.get()), std::runtime_error);
    }
    std::remove(fileName.c_str());
}
