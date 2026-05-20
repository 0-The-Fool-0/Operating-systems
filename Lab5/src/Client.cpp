#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "Protocol.h"
#include "ScopedHandle.h"
#include "WinError.h"

namespace {

bool ReadExact(HANDLE handle, void* buffer, DWORD bytes_to_read) {
    DWORD total_read = 0;
    auto* out = static_cast<unsigned char*>(buffer);
    while (total_read < bytes_to_read) {
        DWORD chunk = 0;
        if (ReadFile(handle, out + total_read, bytes_to_read - total_read, &chunk, nullptr) == 0) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        total_read += chunk;
    }
    return true;
}

bool WriteExact(HANDLE handle, const void* buffer, DWORD bytes_to_write) {
    DWORD total_written = 0;
    const auto* in = static_cast<const unsigned char*>(buffer);
    while (total_written < bytes_to_write) {
        DWORD chunk = 0;
        if (WriteFile(handle, in + total_written, bytes_to_write - total_written, &chunk, nullptr) == 0) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        total_written += chunk;
    }
    return true;
}

lab5::Response SendRequest(HANDLE pipe, const lab5::Request& request) {
    if (WriteExact(pipe, &request, sizeof(request)) == false) {
        throw std::runtime_error(lab5::FormatWindowsError("WriteFile(request) failed", GetLastError()));
    }
    lab5::Response response{};
    if (ReadExact(pipe, &response, sizeof(response)) == false) {
        throw std::runtime_error(lab5::FormatWindowsError("ReadFile(response) failed", GetLastError()));
    }
    return response;
}

int ReadPositiveInt(const std::string& prompt) {
    std::cout << prompt;
    int value = 0;
    if (!(std::cin >> value) || value <= 0) {
        throw std::invalid_argument("Expected positive integer.");
    }
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    return value;
}

double ReadNonNegativeDouble(const std::string& prompt) {
    std::cout << prompt;
    double value = 0.0;
    if (!(std::cin >> value) || value < 0.0) {
        throw std::invalid_argument("Hours must be non-negative.");
    }
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    return value;
}

void PrintEmployee(const employee& e) {
    std::cout << "id=" << e.num << ", name=" << e.name << ", hours=" << e.hours << "\n";
}

void ReleaseLockedRecord(HANDLE pipe, int id) {
    lab5::Request release{};
    release.command = lab5::CommandType::ReleaseRecord;
    release.id = id;
    const lab5::Response r = SendRequest(pipe, release);
    std::cout << "Release result: " << r.message << "\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const std::string pipe_name = (argc > 1) ? argv[1] : std::string(lab5::kDefaultPipeName);
        std::cout << "Lab5 Client\n";
        std::cout << "Connecting to " << pipe_name << "...\n";

        if (WaitNamedPipeA(pipe_name.c_str(), NMPWAIT_WAIT_FOREVER) == 0) {
            throw std::runtime_error(lab5::FormatWindowsError("WaitNamedPipeA failed", GetLastError()));
        }

        lab5::ScopedHandle pipe(CreateFileA(
            pipe_name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr));
        if (!pipe.Valid()) {
            throw std::runtime_error(lab5::FormatWindowsError("CreateFileA(pipe) failed", GetLastError()));
        }

        DWORD mode = PIPE_READMODE_MESSAGE;
        if (SetNamedPipeHandleState(pipe.Get(), &mode, nullptr, nullptr) == 0) {
            throw std::runtime_error(lab5::FormatWindowsError("SetNamedPipeHandleState failed", GetLastError()));
        }

        for (;;) {
            std::cout << "\nChoose operation:\n";
            std::cout << "  1 - modify record\n";
            std::cout << "  2 - read record\n";
            std::cout << "  3 - exit\n> ";
            std::string cmd;
            std::getline(std::cin, cmd);

            if (cmd == "3" || cmd == "exit") {
                lab5::Request req{};
                req.command = lab5::CommandType::Disconnect;
                req.id = 0;
                (void)SendRequest(pipe.Get(), req);
                break;
            }

            if (cmd != "1" && cmd != "2") {
                std::cout << "Unknown operation.\n";
                continue;
            }

            const int id = ReadPositiveInt("Enter employee id: ");
            lab5::Request req{};
            req.command = (cmd == "1") ? lab5::CommandType::AcquireWrite : lab5::CommandType::AcquireRead;
            req.id = id;
            const lab5::Response acquire = SendRequest(pipe.Get(), req);
            std::cout << "Server: " << acquire.message << "\n";
            if (acquire.status != lab5::ResponseStatus::Ok) {
                continue;
            }

            std::cout << "Record from server:\n";
            PrintEmployee(acquire.data);

            if (cmd == "1") {
                employee edited = acquire.data;
                std::cout << "New name (up to 9 chars): ";
                std::string name;
                std::getline(std::cin, name);
                std::memset(edited.name, 0, sizeof(edited.name));
                const size_t count = std::min(name.size(), sizeof(edited.name) - 1U);
                std::memcpy(edited.name, name.data(), count);
                edited.hours = ReadNonNegativeDouble("New hours: ");

                std::cout << "Send update? (y/n): ";
                std::string approve;
                std::getline(std::cin, approve);
                if (approve == "y" || approve == "Y") {
                    lab5::Request update{};
                    update.command = lab5::CommandType::UpdateRecord;
                    update.id = id;
                    update.data = edited;
                    const lab5::Response updated = SendRequest(pipe.Get(), update);
                    std::cout << "Update result: " << updated.message << "\n";
                    if (updated.status == lab5::ResponseStatus::Ok) {
                        PrintEmployee(updated.data);
                    }
                }
            }

            std::cout << "Press Enter to release record...";
            std::string dummy;
            std::getline(std::cin, dummy);
            ReleaseLockedRecord(pipe.Get(), id);
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << "\n";
        return 1;
    }
}
