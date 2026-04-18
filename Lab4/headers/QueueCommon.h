#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

constexpr int kMaxMessageLen = 20;

#pragma pack(push, 1)
struct QueueHeader {
    int32_t capacity;
    int32_t head;
    int32_t tail;
    int32_t count;
};
#pragma pack(pop)

inline std::string SanitizeName(const std::string& source) {
    std::string out;
    out.reserve(source.size());
    for (char ch : source) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            out.push_back(ch);
        } else {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = "queue";
    }
    return out;
}

struct SyncNames {
    std::string mutexName;
    std::string emptySemaphoreName;
    std::string fullSemaphoreName;
    std::string readySemaphoreName;
};

inline SyncNames BuildSyncNames(const std::string& fileName) {
    const std::string base = "Global\\Lab4_" + SanitizeName(fileName);
    return SyncNames{
        base + "_mtx",
        base + "_empty",
        base + "_full",
        base + "_ready"
    };
}
