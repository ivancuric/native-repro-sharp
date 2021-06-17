#pragma once

#include "../common.hpp"

#include "../ffmpeg/VideoMode.hpp"

#include <memory>
#include <thread>


namespace video {

  class VideoMode {
  public:
    static Napi::Value create(const Napi::CallbackInfo& info, ffmpeg::VideoMode mode);
    static Napi::Value create(const Napi::CallbackInfo& info, int w, int h, int fps);

    static ffmpeg::VideoMode convert(const Napi::Object& obj);
  };

} // namespace video
