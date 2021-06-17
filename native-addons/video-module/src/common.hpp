#pragma once

#include <map>
#include <memory>

#include "node/napi_include.hpp"

namespace video {
  using ConstructorMap = std::map<int, std::unique_ptr<Napi::FunctionReference>>;

  enum {
    FFMPEG_CTOR,
    DUMMY_CTOR,
    FRAME_CTOR,
    PERF_LOG_CTOR,
    REMOTE_STREAM_CTOR
  };

  struct InstanceData {
    ConstructorMap constructors;
  };

}
