#pragma once

#include "../common.hpp"

namespace video {
  int getInt(const Napi::Object& obj, const char* name, int emptyValue);
  bool getBool(const Napi::Object& obj, const char* name, bool emptyValue);
  std::string getString(const Napi::Object& obj, const char* name, std::string emptyValue);
}
