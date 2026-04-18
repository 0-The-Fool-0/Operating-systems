#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

#include "QueueCommon.h"

namespace queuefile {

std::string FormatWindowsError(const std::string& context, DWORD errorCode);
void InitializeQueueFile(HANDLE file, int capacity);
void PushMessage(HANDLE file, const std::string& message);
std::string PopMessage(HANDLE file);

}  // namespace queuefile
