#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>

namespace lab5 {

std::string FormatWindowsError(const std::string& context, DWORD error_code);

}  // namespace lab5
