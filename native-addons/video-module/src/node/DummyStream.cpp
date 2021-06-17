#include "DummyStream.hpp"

#include <chrono>
#include <iostream>
#include <memory>

#include "VideoMode.hpp"
#include "../utils/PerfLogger.hpp"

namespace video {

  Napi::Object DummyStream::Init(
    Napi::Env env,
    Napi::Object exports,
    ConstructorMap& ctors)
  {
    Napi::Function func = DefineClass(env, "DummyStream", {
      InstanceMethod<&DummyStream::name>("name"),
      InstanceMethod<&DummyStream::addFrameCallback>("addFrameCallback"),
      InstanceMethod<&DummyStream::clearFrameCallbacks>("clearFrameCallbacks"),
      InstanceMethod<&DummyStream::start>("start"),
      InstanceMethod<&DummyStream::stop>("stop"),
      InstanceMethod<&DummyStream::videoModes>("videoModes"),
      InstanceMethod<&DummyStream::format>("format"),
      InstanceMethod<&DummyStream::enableRemoteStream>("enableRemoteStream"),
      InstanceMethod<&DummyStream::disableRemoteStream>("disableRemoteStream"),
      InstanceMethod<&DummyStream::isActive>("isActive"),
      InstanceMethod<&DummyStream::latestFrameStats>("latestFrameStats"),
      InstanceMethod<&DummyStream::setEventListener>("setEventListener"),
      InstanceMethod<&DummyStream::removeEventListener>("removeEventListener"),
    });

    exports.Set("DummyStream", func);

    auto ctor = std::make_unique<Napi::FunctionReference>();
    *ctor = Napi::Persistent(func);
    ctors[DUMMY_CTOR] = std::move(ctor);

    return exports;
  }

  Napi::Value DummyStream::create(const Napi::CallbackInfo& info) {
    ConstructorMap& ctors = info.Env().GetInstanceData<InstanceData>()->constructors;
    auto& ctor = ctors[DUMMY_CTOR];
    return ctor->New({});
  }

  DummyStream::DummyStream(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<DummyStream>(info),
      m_base("dummy stream"),
      m_workerThread(nullptr),
      m_running(false)
  {
  }

  DummyStream::~DummyStream() {
    if (m_running) {
      stop();
    }
  }

  Napi::Value DummyStream::name(const Napi::CallbackInfo& info) {
    return m_base.name(info);
  }

  Napi::Value DummyStream::videoModes(const Napi::CallbackInfo& info) {
    Napi::Array result = Napi::Array::New(info.Env());

    result.Set(uint32_t(0), VideoMode::create(info, 1280, 720, 1));
    result.Set(uint32_t(1), VideoMode::create(info, 1280, 720, 10));
    result.Set(uint32_t(2), VideoMode::create(info, 1920, 1080, 1));
    result.Set(uint32_t(3), VideoMode::create(info, 1920, 1080, 10));
    result.Set(uint32_t(4), VideoMode::create(info, 3840, 2160, 1));
    result.Set(uint32_t(5), VideoMode::create(info, 3840, 2160, 10));

    return result;
  }

  Napi::Value DummyStream::format(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), "rgb888");
  }

  void DummyStream::start(const Napi::CallbackInfo& info) {
    ffmpeg::VideoMode mode = m_base.videoMode(info);

    if (m_running) {
      return;
    }
    if (m_workerThread) {
      m_workerThread->join();
      m_workerThread.reset();
    }

    m_running = true;
    auto work = [this, mode] {
      m_base.emitStreamStarted();
      const int stepLength = 10;
      const int iterationLimit = (1000/stepLength) / mode.fps;

      int iterations = 0;
      int frames = 1;
      while (this->m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(stepLength));
        ++iterations;
        if(iterations == iterationLimit) {
          int phase = frames % 4;
          utils::PerfLogger::logEntry(m_base.cppName(), utils::Key::Received, frames, mode.profile);
          auto data = FrameData::createTestTexture(phase, mode.w, mode.h, frames);
          utils::PerfLogger::logEntry(m_base.cppName(), utils::Key::Decoded, frames, mode.profile);
          ++frames;
          m_base.frameProduced({ data }, mode.profile);
          iterations = 0;
        }
      }
      m_base.emitStreamStopped();
    };
    m_workerThread = std::make_unique<std::thread>(work);
  }

  void DummyStream::stop(const Napi::CallbackInfo&) {
    stop();
  }

  void DummyStream::stop() {
    if (!m_running) {
      return;
    }
    m_running = false;
    m_workerThread->join();
    m_workerThread.reset();
  }

  // Couldn't get real polymorphism to work so need to do this by hand..
  void DummyStream::addFrameCallback(const Napi::CallbackInfo& info) {
    m_base.addFrameCallback(info);
  }

  void DummyStream::clearFrameCallbacks(const Napi::CallbackInfo& info) {
    m_base.clearFrameCallbacks(info);
  }

  void DummyStream::setEventListener(const Napi::CallbackInfo& info) {
    m_base.setEventListener(info);
  }

  void DummyStream::removeEventListener(const Napi::CallbackInfo&) {
    m_base.removeEventListener();
  }


  Napi::Value DummyStream::enableRemoteStream(const Napi::CallbackInfo& info) {
    return m_base.enableRemoteStream(info);
  }

  void DummyStream::disableRemoteStream(const Napi::CallbackInfo& info) {
    m_base.disableRemoteStream(info);
  }

  Napi::Value DummyStream::isActive(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), m_running);
  }

  Napi::Value DummyStream::latestFrameStats(const Napi::CallbackInfo& info) {
    return m_base.latestFrameStats(info);
  }

} // namespace video
