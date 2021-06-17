#include "Video.hpp"
#include "DummyStream.hpp"
#include "Frame.hpp"
#include "PerfLoggerWrapper.hpp"
#include "RemoteStream.hpp"
#include "VideoMode.hpp"

#include "FFmpegStream.hpp"

#include "../ffmpeg/ffmpeg.hpp"

#include "../utils/PerfLogger.hpp"

namespace video {

  Napi::Array listStreams(const Napi::CallbackInfo& info) {
    Napi::Array result = Napi::Array::New(info.Env());
    result.Set(uint32_t(0), DummyStream::create(info));
    std::vector<Napi::Value> videoStreams = FFmpegStream::listDevices(info);
    for (size_t i = 0; i < videoStreams.size(); ++i) {
      result.Set(uint32_t(i+1), videoStreams[i]);
    }
    return result;
  }

  void logPerf(const Napi::CallbackInfo& info) {
    if (info.Length() < 3 || !info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber()) {
      throw Napi::TypeError::New(info.Env(), "Expects arguments to be string, number and number");
    }
    // We default to write into file now
    auto logger = utils::PerfLogger::instance(info[0].As<Napi::String>());
    logger->log(
      static_cast<utils::Key>(info[1].As<Napi::Number>().Int32Value()),
      info[2].As<Napi::Number>().Int32Value());
  }

  void showFormats(const Napi::CallbackInfo&) {
    ffmpeg::showFormats();
  }

  Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("listStreams", Napi::Function::New(env, listStreams));
    exports.Set("logPerf", Napi::Function::New(env, logPerf));
    exports.Set("showFormats", Napi::Function::New(env, showFormats));
    auto instanceData = new InstanceData();
    FFmpegStream::Init(env, exports, instanceData->constructors);
    DummyStream::Init(env, exports, instanceData->constructors);
    Frame::Init(env, exports, instanceData->constructors);
    PerfLoggerWrapper::Init(env, exports, instanceData->constructors);
    RemoteStream::Init(env, exports, instanceData->constructors);

    env.SetInstanceData<InstanceData>(instanceData);

    return exports;
  }
}
