#pragma once

#include "Stream.hpp"

#include "../common.hpp"

#include <memory>
#include <thread>


namespace video {

  class DummyStream : public Napi::ObjectWrap<DummyStream> {
  public:
    static Napi::Object Init(
      Napi::Env env,
      Napi::Object exports,
      ConstructorMap& ctors);
    static Napi::Value create(const Napi::CallbackInfo& info);

    DummyStream(const Napi::CallbackInfo& info);
    ~DummyStream();

    Napi::Value name(const Napi::CallbackInfo& info);
    Napi::Value videoModes(const Napi::CallbackInfo& info);
    Napi::Value format(const Napi::CallbackInfo& info);

    void start(const Napi::CallbackInfo& info);
    void stop(const Napi::CallbackInfo& info);
    void stop();

    void addFrameCallback(const Napi::CallbackInfo& info);
    void clearFrameCallbacks(const Napi::CallbackInfo&);

    Napi::Value enableRemoteStream(const Napi::CallbackInfo& info);
    void disableRemoteStream(const Napi::CallbackInfo& info);

    void setEventListener(const Napi::CallbackInfo& info);
    void removeEventListener(const Napi::CallbackInfo& info);

    Napi::Value isActive(const Napi::CallbackInfo& info);

    Napi::Value latestFrameStats(const Napi::CallbackInfo& info);

  private:
    Stream m_base;
    std::unique_ptr<std::thread> m_workerThread;
    bool m_running;
  };

} // namespace video
