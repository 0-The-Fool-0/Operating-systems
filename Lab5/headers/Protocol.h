#pragma once

#include <cstring>
#include <stdexcept>
#include <string>

#include "Employee.h"

namespace lab5 {

constexpr int kMessageTextSize = 128;
constexpr const char* kDefaultPipeName = "\\\\.\\pipe\\lab5_employee_pipe";

enum class CommandType : int {
    AcquireRead = 1,
    AcquireWrite = 2,
    UpdateRecord = 3,
    ReleaseRecord = 4,
    Disconnect = 5
};

enum class ResponseStatus : int {
    Ok = 0,
    NotFound = 1,
    Error = 2
};

struct Request {
    CommandType command;
    int id;
    employee data;
};

struct Response {
    ResponseStatus status;
    employee data;
    char message[kMessageTextSize];
};

inline void SetMessage(char (&target)[kMessageTextSize], const std::string& text) {
    std::memset(target, 0, sizeof(target));
    const size_t count = (text.size() < (sizeof(target) - 1U)) ? text.size() : (sizeof(target) - 1U);
    std::memcpy(target, text.data(), count);
}

}  // namespace lab5
