#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "EmployeeRepository.h"
#include "Protocol.h"
#include "RecordLockManager.h"
#include "ScopedHandle.h"
#include "WinError.h"
#ifndef ERROR_PIPE_CONNECTED
#define ERROR_PIPE_CONNECTED 232L
#endif
#ifndef ERROR_IO_PENDING
#define ERROR_IO_PENDING 997L
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 0x00000102L
#endif
namespace {

std::mutex g_log_mutex;

void LogServerEvent(const std::string& text) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm{};
    localtime_s(&local_tm, &now_time);
    std::cout << "[" << std::put_time(&local_tm, "%H:%M:%S") << "] " << text << "\n";
}

std::string GetExecutableDirectory() {
    std::array<char, MAX_PATH> path{};
    const DWORD len = GetModuleFileNameA(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (len == 0 || len >= path.size()) {
        throw std::runtime_error(lab5::FormatWindowsError("GetModuleFileNameA failed", GetLastError()));
    }
    std::string full_path(path.data(), len);
    const size_t pos = full_path.find_last_of("\\/");
    if (pos == std::string::npos) {
        return ".";
    }
    return full_path.substr(0, pos);
}

bool ConnectPipeWithTimeout(HANDLE pipe, DWORD timeout_ms) {
    OVERLAPPED ov{};
    ov.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (ov.hEvent == nullptr) {
        throw std::runtime_error(lab5::FormatWindowsError("CreateEventA failed", GetLastError()));
    }

    const BOOL result = ConnectNamedPipe(pipe, &ov);
    if (result) {
        CloseHandle(ov.hEvent);
        return true;
    }

    const DWORD err = GetLastError();
    if (err == ERROR_PIPE_CONNECTED) {
        CloseHandle(ov.hEvent);
        return true;
    }
    if (err != ERROR_IO_PENDING) {
        CloseHandle(ov.hEvent);
        throw std::runtime_error(lab5::FormatWindowsError("ConnectNamedPipe failed", err));
    }

    const DWORD wait_result = WaitForSingleObject(ov.hEvent, timeout_ms);
    if (wait_result == WAIT_OBJECT_0) {
        DWORD transferred = 0;
        const BOOL ok = GetOverlappedResult(pipe, &ov, &transferred, FALSE);
        CloseHandle(ov.hEvent);
        if (ok == 0) {
            throw std::runtime_error(lab5::FormatWindowsError("GetOverlappedResult failed", GetLastError()));
        }
        return true;
    }
    if (wait_result == WAIT_TIMEOUT) {
        CancelIo(pipe);
        CloseHandle(ov.hEvent);
        return false;
    }

    const DWORD wait_err = GetLastError();
    CloseHandle(ov.hEvent);
    throw std::runtime_error(lab5::FormatWindowsError("WaitForSingleObject failed", wait_err));
}

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

int ReadPositiveInt(const std::string& prompt) {
    std::cout << prompt;
    int value = 0;
    if (!(std::cin >> value) || value <= 0) {
        throw std::invalid_argument("Value must be a positive integer.");
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

employee ReadEmployeeFromConsole(int index) {
    employee e{};
    std::cout << "Employee #" << index << "\n";
    e.num = ReadPositiveInt("  id: ");

    std::cout << "  name (up to 9 chars): ";
    std::string name;
    std::getline(std::cin, name);
    if (name.empty()) {
        throw std::invalid_argument("Name cannot be empty.");
    }
    std::memset(e.name, 0, sizeof(e.name));
    const size_t max_count = sizeof(e.name) - 1U;
    const size_t copy_count = std::min(name.size(), max_count);
    std::memcpy(e.name, name.data(), copy_count);

    e.hours = ReadNonNegativeDouble("  hours: ");
    return e;
}

void ValidateUniqueIds(const std::vector<employee>& employees) {
    std::unordered_set<int> seen_ids;
    seen_ids.reserve(employees.size());
    for (const employee& e : employees) {
        if (!seen_ids.insert(e.num).second) {
            throw std::invalid_argument("Duplicate employee id found: " + std::to_string(e.num));
        }
    }
}

void PrintEmployees(const std::vector<employee>& employees, const std::string& title) {
    std::cout << "\n" << title << "\n";
    if (employees.empty()) {
        std::cout << "  <empty>\n";
        return;
    }
    for (const employee& e : employees) {
        std::cout << "  id=" << e.num << ", name=" << e.name << ", hours=" << e.hours << "\n";
    }
}

struct SessionState {
    bool has_lock = false;
    int locked_id = 0;
    lab5::LockMode mode = lab5::LockMode::Read;
};

void SendResponseOrThrow(HANDLE pipe, const lab5::Response& response) {
    if (WriteExact(pipe, &response, sizeof(response)) == false) {
        throw std::runtime_error(lab5::FormatWindowsError("WriteFile(response) failed", GetLastError()));
    }
}

void HandleClient(HANDLE pipe, lab5::EmployeeRepository& repository, lab5::RecordLockManager& locks) {
    SessionState session{};
    try {
        for (;;) {
            lab5::Request request{};
            if (ReadExact(pipe, &request, sizeof(request)) == false) {
                break;
            }

            lab5::Response response{};
            response.status = lab5::ResponseStatus::Ok;
            std::memset(&response.data, 0, sizeof(response.data));
            lab5::SetMessage(response.message, "OK");

            if (request.command == lab5::CommandType::AcquireRead ||
                request.command == lab5::CommandType::AcquireWrite) {
                if (session.has_lock) {
                    response.status = lab5::ResponseStatus::Error;
                    lab5::SetMessage(response.message, "Release current record first.");
                    SendResponseOrThrow(pipe, response);
                    continue;
                }

                const lab5::LockMode mode = (request.command == lab5::CommandType::AcquireRead)
                    ? lab5::LockMode::Read
                    : lab5::LockMode::Write;
                LogServerEvent(
                    std::string("Client requests ") + ((mode == lab5::LockMode::Read) ? "READ" : "WRITE") +
                    " lock for id=" + std::to_string(request.id));
                locks.Acquire(request.id, mode);

                employee found{};
                if (repository.ReadById(request.id, found) == false) {
                    locks.Release(request.id, mode);
                    response.status = lab5::ResponseStatus::NotFound;
                    lab5::SetMessage(response.message, "Employee not found.");
                    SendResponseOrThrow(pipe, response);
                    continue;
                }

                session.has_lock = true;
                session.locked_id = request.id;
                session.mode = mode;
                response.data = found;
                LogServerEvent(
                    std::string("Lock granted for id=") + std::to_string(request.id) +
                    ((mode == lab5::LockMode::Read) ? " (READ)" : " (WRITE)"));
                SendResponseOrThrow(pipe, response);
                continue;
            }

            if (request.command == lab5::CommandType::UpdateRecord) {
                if (!session.has_lock || session.mode != lab5::LockMode::Write || session.locked_id != request.id) {
                    response.status = lab5::ResponseStatus::Error;
                    lab5::SetMessage(response.message, "Write lock for this id is required.");
                    SendResponseOrThrow(pipe, response);
                    continue;
                }
                employee updated = request.data;
                updated.num = request.id;
                if (repository.Update(updated) == false) {
                    response.status = lab5::ResponseStatus::NotFound;
                    lab5::SetMessage(response.message, "Employee not found.");
                } else {
                    response.data = updated;
                    lab5::SetMessage(response.message, "Record updated.");
                    LogServerEvent("Record updated for id=" + std::to_string(request.id));
                }
                SendResponseOrThrow(pipe, response);
                continue;
            }

            if (request.command == lab5::CommandType::ReleaseRecord) {
                if (!session.has_lock) {
                    response.status = lab5::ResponseStatus::Error;
                    lab5::SetMessage(response.message, "No locked record.");
                } else {
                    locks.Release(session.locked_id, session.mode);
                    LogServerEvent("Lock released for id=" + std::to_string(session.locked_id));
                    session.has_lock = false;
                    lab5::SetMessage(response.message, "Record released.");
                }
                SendResponseOrThrow(pipe, response);
                continue;
            }

            if (request.command == lab5::CommandType::Disconnect) {
                if (session.has_lock) {
                    LogServerEvent("Client disconnected, auto-release id=" + std::to_string(session.locked_id));
                    locks.Release(session.locked_id, session.mode);
                    session.has_lock = false;
                }
                SendResponseOrThrow(pipe, response);
                break;
            }

            response.status = lab5::ResponseStatus::Error;
            lab5::SetMessage(response.message, "Unknown command.");
            SendResponseOrThrow(pipe, response);
        }
    } catch (const std::exception&) {
        if (session.has_lock) {
            locks.Release(session.locked_id, session.mode);
        }
    }
}

}  // namespace

int main() {
    try {
        std::cout << "Lab5 Server (Named Pipes)\n";

        std::string file_name;
        std::cout << "Enter binary file name: ";
        std::getline(std::cin, file_name);
        if (file_name.empty()) {
            throw std::invalid_argument("File name cannot be empty.");
        }

        if (GetFileAttributesA(file_name.c_str()) != INVALID_FILE_ATTRIBUTES) {
            std::cout << "File exists. Overwrite? (y/n): ";
            std::string answer;
            std::getline(std::cin, answer);
            if (answer != "y" && answer != "Y") {
                std::cout << "Cancelled.\n";
                return 0;
            }
        }

        const int employee_count = ReadPositiveInt("Enter employee count: ");
        std::vector<employee> employees;
        employees.reserve(static_cast<size_t>(employee_count));
        for (int i = 1; i <= employee_count; ++i) {
            employees.push_back(ReadEmployeeFromConsole(i));
        }
        ValidateUniqueIds(employees);

        lab5::EmployeeRepository repository(file_name);
        repository.Initialize(employees);
        PrintEmployees(repository.ReadAll(), "Initial file data:");

        const int client_count = ReadPositiveInt("Enter client process count: ");
        const std::string exe_dir = GetExecutableDirectory();

        std::vector<PROCESS_INFORMATION> clients;
        clients.reserve(static_cast<size_t>(client_count));
        for (int i = 0; i < client_count; ++i) {
            STARTUPINFOA si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};

            std::string client_exe = exe_dir + "\\Client.exe";
            std::string cmd = "\"" + client_exe + "\" \"" + std::string(lab5::kDefaultPipeName) + "\"";
            std::vector<char> cmd_buffer(cmd.begin(), cmd.end());
            cmd_buffer.push_back('\0');

            if (CreateProcessA(
                    nullptr, cmd_buffer.data(), nullptr, nullptr, FALSE,
                    CREATE_NEW_CONSOLE, nullptr, exe_dir.c_str(), &si, &pi) == 0) {
                std::cerr << "Cannot launch Client #" << (i + 1) << ": "
                          << lab5::FormatWindowsError("CreateProcessA failed", GetLastError()) << "\n";
                continue;
            }
            clients.push_back(pi);
            LogServerEvent("Client process started #" + std::to_string(i + 1));
        }

        lab5::RecordLockManager locks;
        std::vector<std::thread> workers;
        workers.reserve(clients.size());
        for (size_t i = 0; i < clients.size(); ++i) {
            HANDLE pipe = CreateNamedPipeA(
                lab5::kDefaultPipeName,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                sizeof(lab5::Response),
                sizeof(lab5::Request),
                0,
                nullptr);
            if (pipe == INVALID_HANDLE_VALUE) {
                throw std::runtime_error(lab5::FormatWindowsError("CreateNamedPipeA failed", GetLastError()));
            }

            const bool connected = ConnectPipeWithTimeout(pipe, 30000);
            if (!connected) {
                LogServerEvent("ConnectNamedPipe timeout. Skipping one client session.");
                CloseHandle(pipe);
                continue;
            }
            LogServerEvent("Client connected to named pipe instance.");

            workers.emplace_back([pipe, &repository, &locks]() {
                HandleClient(pipe, repository, locks);
                FlushFileBuffers(pipe);
                DisconnectNamedPipe(pipe);
                CloseHandle(pipe);
                LogServerEvent("Client session thread finished.");
            });
        }

        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        for (PROCESS_INFORMATION& pi : clients) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            if (pi.hThread != nullptr) {
                CloseHandle(pi.hThread);
            }
            if (pi.hProcess != nullptr) {
                CloseHandle(pi.hProcess);
            }
        }

        PrintEmployees(repository.ReadAll(), "Modified file data:");

        std::cout << "\nType \"exit\" to stop server: ";
        std::string cmd;
        for (;;) {
            std::getline(std::cin, cmd);
            if (cmd == "exit") {
                break;
            }
            std::cout << "Please type \"exit\": ";
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << "\n";
        return 1;
    }
}
