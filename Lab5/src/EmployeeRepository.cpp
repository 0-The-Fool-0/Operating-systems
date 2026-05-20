#include "EmployeeRepository.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "WinError.h"

namespace lab5 {

namespace {

bool ReadExact(HANDLE file, void* buffer, DWORD bytes_to_read) {
    DWORD total_read = 0;
    auto* out = static_cast<unsigned char*>(buffer);
    while (total_read < bytes_to_read) {
        DWORD chunk = 0;
        if (ReadFile(file, out + total_read, bytes_to_read - total_read, &chunk, nullptr) == 0) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        total_read += chunk;
    }
    return true;
}

bool WriteExact(HANDLE file, const void* buffer, DWORD bytes_to_write) {
    DWORD total_written = 0;
    const auto* in = static_cast<const unsigned char*>(buffer);
    while (total_written < bytes_to_write) {
        DWORD chunk = 0;
        if (WriteFile(file, in + total_written, bytes_to_write - total_written, &chunk, nullptr) == 0) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        total_written += chunk;
    }
    return true;
}

}  // namespace

std::string FormatWindowsError(const std::string& context, DWORD error_code) {
    LPSTR message_buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD size = FormatMessageA(
        flags, nullptr, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr);

    std::string message = context + ". Error code: " + std::to_string(error_code);
    if (size != 0 && message_buffer != nullptr) {
        message += " (";
        message += message_buffer;
        message += ")";
    }
    if (message_buffer != nullptr) {
        LocalFree(message_buffer);
    }
    return message;
}

EmployeeRepository::EmployeeRepository(std::string file_name)
    : file_name_(std::move(file_name)) {
}

void EmployeeRepository::Initialize(const std::vector<employee>& items) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    HANDLE file = CreateFileA(
        file_name_.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(FormatWindowsError("Cannot create employee file", GetLastError()));
    }

    const DWORD bytes_total = static_cast<DWORD>(items.size() * sizeof(employee));
    if (bytes_total > 0 && WriteExact(file, items.data(), bytes_total) == false) {
        const DWORD code = GetLastError();
        CloseHandle(file);
        throw std::runtime_error(FormatWindowsError("Cannot write employee file", code));
    }

    if (FlushFileBuffers(file) == 0) {
        const DWORD code = GetLastError();
        CloseHandle(file);
        throw std::runtime_error(FormatWindowsError("FlushFileBuffers failed", code));
    }
    CloseHandle(file);
}

std::vector<employee> EmployeeRepository::ReadAll() const {
    std::lock_guard<std::mutex> lock(file_mutex_);
    HANDLE file = CreateFileA(
        file_name_.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(FormatWindowsError("Cannot open employee file", GetLastError()));
    }

    LARGE_INTEGER size{};
    if (GetFileSizeEx(file, &size) == 0) {
        const DWORD code = GetLastError();
        CloseHandle(file);
        throw std::runtime_error(FormatWindowsError("GetFileSizeEx failed", code));
    }
    if ((size.QuadPart % static_cast<LONGLONG>(sizeof(employee))) != 0) {
        CloseHandle(file);
        throw std::runtime_error("Employee file is corrupted.");
    }

    const size_t count = static_cast<size_t>(size.QuadPart / static_cast<LONGLONG>(sizeof(employee)));
    std::vector<employee> result(count);
    if (count > 0 && ReadExact(file, result.data(), static_cast<DWORD>(count * sizeof(employee))) == false) {
        const DWORD code = GetLastError();
        CloseHandle(file);
        throw std::runtime_error(FormatWindowsError("Cannot read employee file", code));
    }
    CloseHandle(file);
    return result;
}

bool EmployeeRepository::ReadById(int id, employee& out) const {
    const std::vector<employee> all = ReadAll();
    const auto it = std::find_if(all.begin(), all.end(), [id](const employee& value) {
        return value.num == id;
    });
    if (it == all.end()) {
        return false;
    }
    out = *it;
    return true;
}

bool EmployeeRepository::Update(const employee& value) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    HANDLE file = CreateFileA(
        file_name_.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(FormatWindowsError("Cannot open employee file", GetLastError()));
    }

    employee current{};
    LARGE_INTEGER offset{};
    bool found = false;
    for (;;) {
        DWORD bytes_read = 0;
        if (ReadFile(file, &current, sizeof(current), &bytes_read, nullptr) == 0) {
            const DWORD code = GetLastError();
            CloseHandle(file);
            throw std::runtime_error(FormatWindowsError("ReadFile failed during update", code));
        }
        if (bytes_read == 0) {
            break;
        }
        if (bytes_read != sizeof(current)) {
            CloseHandle(file);
            throw std::runtime_error("Employee file is corrupted.");
        }
        if (current.num == value.num) {
            found = true;
            break;
        }
    }

    if (!found) {
        CloseHandle(file);
        return false;
    }

    offset.QuadPart = -static_cast<LONGLONG>(sizeof(employee));
    if (SetFilePointerEx(file, offset, nullptr, FILE_CURRENT) == 0) {
        const DWORD code = GetLastError();
        CloseHandle(file);
        throw std::runtime_error(FormatWindowsError("SetFilePointerEx failed", code));
    }
    if (WriteExact(file, &value, sizeof(value)) == false) {
        const DWORD code = GetLastError();
        CloseHandle(file);
        throw std::runtime_error(FormatWindowsError("WriteFile failed during update", code));
    }
    if (FlushFileBuffers(file) == 0) {
        const DWORD code = GetLastError();
        CloseHandle(file);
        throw std::runtime_error(FormatWindowsError("FlushFileBuffers failed", code));
    }

    CloseHandle(file);
    return true;
}

}  // namespace lab5
