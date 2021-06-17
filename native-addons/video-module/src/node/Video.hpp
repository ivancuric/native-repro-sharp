#pragma once

#include "napi_include.hpp"
#include <map>

#include "Stream.hpp"

namespace video {
  Napi::Array listStreams(const Napi::CallbackInfo& info);
  void logPerf(const Napi::CallbackInfo& info);
  void showFormats(const Napi::CallbackInfo& info);

  Napi::Object Init(Napi::Env env, Napi::Object exports);

#include "../disable_warnings_begin.hpp"
  NODE_API_MODULE(video, Init)
#include "../disable_warnings_end.hpp"
}
