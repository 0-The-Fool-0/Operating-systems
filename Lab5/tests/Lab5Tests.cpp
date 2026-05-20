#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <gtest/gtest.h>

#include "EmployeeRepository.h"
#include "RecordLockManager.h"

namespace {

std::string MakeTempFileName() {
    char path_buffer[MAX_PATH]{};
    const DWORD length = GetTempPathA(MAX_PATH, path_buffer);
    if (length == 0 || length > MAX_PATH) {
        throw std::runtime_error("GetTempPathA failed.");
    }
    char file_buffer[MAX_PATH]{};
    if (GetTempFileNameA(path_buffer, "l5t", 0, file_buffer) == 0) {
        throw std::runtime_error("GetTempFileNameA failed.");
    }
    return std::string(file_buffer);
}

employee MakeEmployee(int id, const char* name, double hours) {
    employee e{};
    e.num = id;
    std::memset(e.name, 0, sizeof(e.name));
    const size_t len = std::strlen(name);
    const size_t copy = (len < (sizeof(e.name) - 1U)) ? len : (sizeof(e.name) - 1U);
    std::memcpy(e.name, name, copy);
    e.hours = hours;
    return e;
}

}  // namespace

TEST(Lab5RepositoryTests, InitializesAndReadsRecords) {
    const std::string file = MakeTempFileName();
    lab5::EmployeeRepository repo(file);
    repo.Initialize({MakeEmployee(1, "Ann", 8.5), MakeEmployee(2, "Bob", 7.0)});

    const std::vector<employee> all = repo.ReadAll();
    ASSERT_EQ(all.size(), 2U);
    EXPECT_EQ(all[0].num, 1);
    EXPECT_STREQ(all[0].name, "Ann");
    EXPECT_DOUBLE_EQ(all[0].hours, 8.5);
    EXPECT_EQ(all[1].num, 2);
    EXPECT_STREQ(all[1].name, "Bob");
    EXPECT_DOUBLE_EQ(all[1].hours, 7.0);

    std::remove(file.c_str());
}

TEST(Lab5RepositoryTests, UpdatesById) {
    const std::string file = MakeTempFileName();
    lab5::EmployeeRepository repo(file);
    repo.Initialize({MakeEmployee(11, "Kate", 6.0)});

    employee changed = MakeEmployee(11, "Maria", 12.0);
    ASSERT_TRUE(repo.Update(changed));

    employee read_back{};
    ASSERT_TRUE(repo.ReadById(11, read_back));
    EXPECT_STREQ(read_back.name, "Maria");
    EXPECT_DOUBLE_EQ(read_back.hours, 12.0);

    std::remove(file.c_str());
}

TEST(Lab5LocksTests, AllowsConcurrentReaders) {
    lab5::RecordLockManager locks;
    std::atomic<int> inside{0};
    std::atomic<int> max_inside{0};

    auto reader = [&]() {
        locks.Acquire(5, lab5::LockMode::Read);
        const int now = ++inside;
        if (now > max_inside) {
            max_inside = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        --inside;
        locks.Release(5, lab5::LockMode::Read);
    };

    std::thread r1(reader);
    std::thread r2(reader);
    r1.join();
    r2.join();

    EXPECT_GE(max_inside.load(), 2);
}

TEST(Lab5LocksTests, WriterWaitsUntilReadersLeave) {
    lab5::RecordLockManager locks;
    std::atomic<bool> writer_entered{false};

    locks.Acquire(7, lab5::LockMode::Read);

    std::thread writer([&]() {
        locks.Acquire(7, lab5::LockMode::Write);
        writer_entered = true;
        locks.Release(7, lab5::LockMode::Write);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    EXPECT_FALSE(writer_entered.load());

    locks.Release(7, lab5::LockMode::Read);
    writer.join();
    EXPECT_TRUE(writer_entered.load());
}
