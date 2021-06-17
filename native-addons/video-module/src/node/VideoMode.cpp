#include "VideoMode.hpp"
#include "Utils.hpp"

namespace video {

  Napi::Value VideoMode::create(const Napi::CallbackInfo& info, ffmpeg::VideoMode mode)
  {
    return create(info, mode.w, mode.h, mode.fps);
  }

  Napi::Value VideoMode::create(const Napi::CallbackInfo& info, int w, int h, int fps)
  {
    Napi::Env env = info.Env();
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("width", w);
    obj.Set("height", h);
    obj.Set("fps", fps);
    return obj;
  }

  ffmpeg::VideoMode VideoMode::convert(const Napi::Object& obj) {
    return ffmpeg::VideoMode(getInt(obj, "width", 0),
                             getInt(obj, "height", 0),
                             getInt(obj, "fps", 0),
                             getBool(obj, "profile", false));
  }

}
