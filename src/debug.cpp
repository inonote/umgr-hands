// Copyright (c) 2025 inonote
// Use of this source code is governed by the MIT License
// that can be found at https://github.com/inonote/umgr-hands/blob/main/LICENSE

#include "debug.h"
#include <cwchar>
#include <cstdarg>

namespace umgrhands {

int printLog(const wchar_t* format, ...) {
  std::va_list args;
  va_start(args, format);
  int ret = std::vfwprintf(stdout, format, args);
  std::fputws(L"\r\n", stdout);
  va_end(args);
  return ret;
}

int printError(const wchar_t* format, ...) {
  std::va_list args;
  va_start(args, format);
  int ret = std::vfwprintf(stderr, format, args);
  std::fputws(L"\r\n", stderr);
  va_end(args);
  return ret;
}

}
