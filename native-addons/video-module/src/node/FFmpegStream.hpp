#pragma once

#include "napi_include.hpp"

#include <memory>
#include <string>
#include <vector>

#include "../common.hpp"

#include "Stream.hpp"

#include "../ffmpeg/StreamContext.hpp"
#include "../ffmpeg/OutputContext.hpp"

namespace video {

  class FFmpegStream : public Napi::ObjectWrap<FFmpegStream> {
  public:
    static Napi::Object Init(
      Napi::Env env,
      Napi::Object exports,
      ConstructorMap& ctors);

    static std::vector<Napi::Value> listDevices(const Napi::CallbackInfo& info);

    static Napi::Value create(
      const Napi::CallbackInfo& info,
      const std::string& name);


    FFmpegStream(const Napi::CallbackInfo& info);
    FFmpegStream(const Napi::CallbackInfo& info, const std::string& name);
    ~FFmpegStream();

    Napi::Value name(const Napi::CallbackInfo& info);
    Napi::Value videoModes(const Napi::CallbackInfo& info);
    Napi::Value format(const Napi::CallbackInfo& info);

    const std::string& cppName() const;

    void takeSnapshot(const Napi::CallbackInfo&);

    void start(const Napi::CallbackInfo&);
    void start(const ffmpeg::VideoMode& mode);
    void stop(const Napi::CallbackInfo&);
    void stop();

    void addFrameCallback(const Napi::CallbackInfo& info);
    void clearFrameCallbacks(const Napi::CallbackInfo&);

    void setEventListener(const Napi::CallbackInfo& info);
    void removeEventListener(const Napi::CallbackInfo& info);

    Napi::Value enableRemoteStream(const Napi::CallbackInfo& info);
    void disableRemoteStream(const Napi::CallbackInfo& info);

    Napi::Value isActive(const Napi::CallbackInfo& info);

    Napi::Value latestFrameStats(const Napi::CallbackInfo& info);

    Napi::Value isRecording(const Napi::CallbackInfo& info);
    void startRecording(const Napi::CallbackInfo& info);
    void startRecording(const std::string& outputPath, bool snapshot);
    void stopRecording(const Napi::CallbackInfo& info);

  private:
    Stream m_base;
    std::unique_ptr<ffmpeg::StreamContext> m_ctx;
    std::unique_ptr<std::thread> m_workerThread;
    bool m_running;
    bool m_recording;
    std::unique_ptr<ffmpeg::OutputContext> m_recordingContext; // only use in main thread when creating
  };

} // namespace video
