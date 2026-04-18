#include "QueueFile.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>

namespace queuefile {
namespace {

bool ReadExact(HANDLE file, void* buffer, DWORD bytesToRead) {
    DWORD totalRead = 0;
    auto* out = static_cast<std::byte*>(buffer);
    while (totalRead < bytesToRead) {
        DWORD chunk = 0;
        if (!ReadFile(file, out + totalRead, bytesToRead - totalRead, &chunk, nullptr)) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        totalRead += chunk;
    }
    return true;
}

bool WriteExact(HANDLE file, const void* buffer, DWORD bytesToWrite) {
    DWORD totalWritten = 0;
    const auto* in = static_cast<const std::byte*>(buffer);
    while (totalWritten < bytesToWrite) {
        DWORD chunk = 0;
        if (!WriteFile(file, in + totalWritten, bytesToWrite - totalWritten, &chunk, nullptr)) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        totalWritten += chunk;
    }
    return true;
}

std::streamoff SlotOffset(int index) {
    return static_cast<std::streamoff>(sizeof(QueueHeader)) +
           static_cast<std::streamoff>(index) * kMaxMessageLen;
}

void SeekTo(HANDLE file, std::streamoff offset) {
    LARGE_INTEGER move;
    move.QuadPart = offset;
    if (SetFilePointerEx(file, move, nullptr, FILE_BEGIN) == 0) {
        throw std::runtime_error(FormatWindowsError("SetFilePointerEx failed", GetLastError()));
    }
}

QueueHeader ReadHeader(HANDLE file) {
    QueueHeader header{};
    SeekTo(file, 0);
    if (!ReadExact(file, &header, sizeof(header))) {
        throw std::runtime_error(FormatWindowsError("Failed to read queue header", GetLastError()));
    }
    if (header.capacity <= 0 || header.count < 0 || header.count > header.capacity) {
        throw std::runtime_error("Queue header is corrupted.");
    }
    return header;
}

void WriteHeader(HANDLE file, const QueueHeader& header) {
    SeekTo(file, 0);
    if (!WriteExact(file, &header, sizeof(header))) {
        throw std::runtime_error(FormatWindowsError("Failed to write queue header", GetLastError()));
    }
}

}  // namespace

std::string FormatWindowsError(const std::string& context, DWORD errorCode) {
    LPSTR messageBuffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD size = FormatMessageA(
        flags, nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

    std::string message = context + ". Error code: " + std::to_string(errorCode);
    if (size != 0 && messageBuffer != nullptr) {
        message += " (";
        message += messageBuffer;
        message += ")";
    }
    if (messageBuffer != nullptr) {
        LocalFree(messageBuffer);
    }
    return message;
}

void InitializeQueueFile(HANDLE file, int capacity) {
    if (capacity <= 0) {
        throw std::invalid_argument("Queue capacity must be greater than zero.");
    }

    QueueHeader header{};
    header.capacity = capacity;
    header.head = 0;
    header.tail = 0;
    header.count = 0;

    WriteHeader(file, header);

    const std::array<char, kMaxMessageLen> emptySlot{};
    for (int i = 0; i < capacity; ++i) {
        if (!WriteExact(file, emptySlot.data(), static_cast<DWORD>(emptySlot.size()))) {
            throw std::runtime_error(FormatWindowsError("Failed to initialize queue slot", GetLastError()));
        }
    }

    if (FlushFileBuffers(file) == 0) {
        throw std::runtime_error(FormatWindowsError("FlushFileBuffers failed", GetLastError()));
    }
}

void PushMessage(HANDLE file, const std::string& message) {
    if (message.size() >= static_cast<size_t>(kMaxMessageLen)) {
        throw std::invalid_argument("Message must contain less than 20 characters.");
    }

    QueueHeader header = ReadHeader(file);
    if (header.count == header.capacity) {
        throw std::runtime_error("Queue overflow in PushMessage.");
    }

    std::array<char, kMaxMessageLen> raw{};
    std::memcpy(raw.data(), message.c_str(), std::min(message.size(), static_cast<size_t>(kMaxMessageLen - 1)));

    SeekTo(file, SlotOffset(header.tail));
    if (!WriteExact(file, raw.data(), static_cast<DWORD>(raw.size()))) {
        throw std::runtime_error(FormatWindowsError("Failed to write message to queue slot", GetLastError()));
    }

    header.tail = (header.tail + 1) % header.capacity;
    ++header.count;
    WriteHeader(file, header);

    if (FlushFileBuffers(file) == 0) {
        throw std::runtime_error(FormatWindowsError("FlushFileBuffers failed after push", GetLastError()));
    }
}

std::string PopMessage(HANDLE file) {
    QueueHeader header = ReadHeader(file);
    if (header.count == 0) {
        throw std::runtime_error("Queue underflow in PopMessage.");
    }

    std::array<char, kMaxMessageLen> raw{};
    SeekTo(file, SlotOffset(header.head));
    if (!ReadExact(file, raw.data(), static_cast<DWORD>(raw.size()))) {
        throw std::runtime_error(FormatWindowsError("Failed to read message from queue slot", GetLastError()));
    }

    raw[kMaxMessageLen - 1] = '\0';
    std::string message(raw.data());

    header.head = (header.head + 1) % header.capacity;
    --header.count;
    WriteHeader(file, header);

    if (FlushFileBuffers(file) == 0) {
        throw std::runtime_error(FormatWindowsError("FlushFileBuffers failed after pop", GetLastError()));
    }

    return message;
}

}  // namespace queuefile
