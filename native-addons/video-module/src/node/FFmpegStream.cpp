#include "FFmpegStream.hpp"
#include "../ffmpeg/ffmpeg.hpp"
#include "../ffmpeg/AVFrameData.hpp"

#include "../utils/PerfLogger.hpp"

#include "Utils.hpp"
#include "VideoMode.hpp"

#include <iostream>

namespace video {

  Napi::Object FFmpegStream::Init(
    Napi::Env env,
    Napi::Object exports,
    ConstructorMap& ctors)
  {
    // Accessor & method callbacks between C++ & JS
    Napi::Function func = DefineClass(env, "FFmpegStream", {
      InstanceMethod<&FFmpegStream::name>("name"),
      InstanceMethod<&FFmpegStream::addFrameCallback>("addFrameCallback"),
      InstanceMethod<&FFmpegStream::clearFrameCallbacks>("clearFrameCallbacks"),
      InstanceMethod<&FFmpegStream::start>("start"),
      InstanceMethod<&FFmpegStream::stop>("stop"),
      InstanceMethod<&FFmpegStream::videoModes>("videoModes"),
      InstanceMethod<&FFmpegStream::format>("format"),
      InstanceMethod<&FFmpegStream::enableRemoteStream>("enableRemoteStream"),
      InstanceMethod<&FFmpegStream::disableRemoteStream>("disableRemoteStream"),
      InstanceMethod<&FFmpegStream::isActive>("isActive"),
      InstanceMethod<&FFmpegStream::latestFrameStats>("latestFrameStats"),
      InstanceMethod<&FFmpegStream::setEventListener>("setEventListener"),
      InstanceMethod<&FFmpegStream::removeEventListener>("removeEventListener"),
      InstanceMethod<&FFmpegStream::isRecording>("isRecording"),
      InstanceMethod<&FFmpegStream::startRecording>("startRecording"),
      InstanceMethod<&FFmpegStream::stopRecording>("stopRecording"),
      InstanceMethod<&FFmpegStream::takeSnapshot>("takeSnapshot"),
    });

    exports.Set("FFmpegStream", func);

    auto ctor = std::make_unique<Napi::FunctionReference>();
    *ctor = Napi::Persistent(func);
    ctors[FFMPEG_CTOR] = std::move(ctor);

    ffmpeg::init();

    return exports;
  }

  std::vector<Napi::Value>
  FFmpegStream::listDevices(const Napi::CallbackInfo& info) {
    std::vector<Napi::Value> result;
    auto deviceNames = ffmpeg::inputVideoDevices();
    for(const std::string& deviceName : deviceNames) {
      result.push_back(FFmpegStream::create(info, deviceName));
    }
    return result;
  }

  Napi::Value FFmpegStream::create(
    const Napi::CallbackInfo& info,
    const std::string& name)
  {
    ConstructorMap& ctors = info.Env().GetInstanceData<InstanceData>()->constructors;
    auto& ctor = ctors[FFMPEG_CTOR];
    return ctor->New({ Napi::String::New(info.Env(), name) });
  }

  FFmpegStream::FFmpegStream(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<FFmpegStream>(info),
      m_ctx(nullptr),
      m_workerThread(nullptr),
      m_running(false),
      m_recording(false),
      m_recordingContext(nullptr)
  {
    if (info.Length() == 0 || !info[0].IsString()) {
      Napi::Env env = info.Env();
      Napi::TypeError::New(env, "Requires String as parameter")
        .ThrowAsJavaScriptException();
    }

    m_base.setName(info[0].As<Napi::String>());
  }

  FFmpegStream::FFmpegStream(
    const Napi::CallbackInfo& info,
    const std::string& name)
    : Napi::ObjectWrap<FFmpegStream>(info),
      m_base(name),
      m_recording(false),
      m_recordingContext(nullptr)
  {
  }

  FFmpegStream::~FFmpegStream() {
    stop();
  }

  Napi::Value FFmpegStream::name(const Napi::CallbackInfo& info) {
    return m_base.name(info);
  }

  Napi::Value FFmpegStream::format(const Napi::CallbackInfo& info) {
    // See libavutil/pixfmt.h
    if (m_ctx && m_ctx->codecContext->pix_fmt == AV_PIX_FMT_UYVY422) {
      return Napi::String::New(info.Env(), "uyvu422");
    } else if (m_ctx && m_ctx->codecContext->pix_fmt == AV_PIX_FMT_YUVJ422P) {
      return Napi::String::New(info.Env(), "yuvj422p");
    } else {
      return Napi::Number::New(info.Env(), m_ctx->codecContext->pix_fmt);
    }
  }

  Napi::Value FFmpegStream::videoModes(const Napi::CallbackInfo& info) {
    Napi::Array result = Napi::Array::New(info.Env());

    auto modes = ffmpeg::getVideoModes(m_base.cppName());
    for(size_t i = 0; i < modes.size(); ++i) {
      result.Set(uint32_t(i), VideoMode::create(info, modes[i]));
    }

    return result;
  }

  void FFmpegStream::stop(const Napi::CallbackInfo&) {
    stop();
  }

  void FFmpegStream::stop() {
    if (m_running) {
      m_base.emitStreamStopRequested();
      m_running = false;
    }
    // Ensure clean up is always executed
    m_base.stop(false);
    if (m_workerThread) {
      m_workerThread->detach();
      m_workerThread.reset();
    }
  }

  const std::string& FFmpegStream::cppName() const {
    return m_base.cppName();
  }

  void FFmpegStream::takeSnapshot(const Napi::CallbackInfo& info) {
    if (m_running) {
      return;
    }

    if (info.Length() == 1 && info[0].IsObject()) {
      ffmpeg::VideoMode mode = m_base.videoMode(info);
      Napi::Object params = info[0].As<Napi::Object>();
      std::string target = getString(params, "outputPath", "test.jpg");
      startRecording(target, true);
      start(mode);
      return;
    }
    m_base.emitStreamStartFailed();
  }

  void FFmpegStream::start(const Napi::CallbackInfo& info) {
    ffmpeg::VideoMode mode = m_base.videoMode(info);
    start(mode);
  }

  void FFmpegStream::start(const ffmpeg::VideoMode& mode) {
    if (m_running) {
      return;
    }
    if (m_workerThread) {
      m_workerThread->detach();
      m_workerThread.reset();
    }

    m_running = true;

    auto work = [this, mode] {
      m_ctx = ffmpeg::start(m_base.cppName(), mode);
      if (!m_ctx) {
        m_running = false;
        m_base.emitStreamStartFailed();
        return;
      }
      m_base.emitStreamStarted();
      unsigned frameCount = static_cast<unsigned>(m_ctx->frameNumber);
      bool isRecording = false;
      std::unique_ptr<ffmpeg::OutputContext> recordingContext;
      while (m_running) {
        if (isRecording != m_recording) {
          if (isRecording) {
            ffmpeg::stopOutput(recordingContext);
            isRecording = false;
            m_base.emitStreamStoppedRecording();
          } else {
            isRecording = true;
            recordingContext = std::move(m_recordingContext);
            std::optional<std::string> error = ffmpeg::initOutput(recordingContext, m_ctx);
            if (error) {
              m_recording = isRecording = false;
              m_base.emitStreamFailedRecording(error.value());
            } else if (!recordingContext->isSnapshot) {
              m_base.emitStreamStartedRecording();
            }
          }
        }

        av_frame_unref(m_ctx->frame);
        int err = ffmpeg::prepareFrame(*m_ctx);
        int tries = 1;
        while(ffmpeg::tryagain(err)) {
          if (tries > 500) {
            break;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          err = ffmpeg::prepareFrame(*m_ctx);
          ++tries;
        }
        if (err != 0) {
          m_running = false;
          m_base.emitStreamFatalError();
          break;
        }

        while(err >= 0) {
          err = ffmpeg::receiveFrame(*m_ctx);
          if (ffmpeg::tryagain(err)) {
            break;
          } else if (err != 0) {
            m_running = false;
            m_base.emitStreamFatalError();
            break;
          }
          utils::PerfLogger::logEntry(m_ctx->name, utils::Key::Decoded, m_ctx->frameNumber, m_ctx->profile);
          m_ctx->frameNumber = static_cast<int>(frameCount) + 1;
          std::vector<std::shared_ptr<video::FrameData>> data;
          for (size_t i = 0; i < 8; ++i) {
            int lineSize = m_ctx->frame->linesize[i];
            if (lineSize != 0) {
              // First plane is "master plane" that keeps reference to FFmpeg data
              // alive. Rest of the planes are just forward references to original
              // AVFrame plane data
              auto frameData = std::make_shared<ffmpeg::AVFrameData>(data.empty());
              frameData->refFrame(m_ctx->frame, i);
              frameData->setFrameNumber(frameCount);
              data.push_back(frameData);
            }
          }
          ++frameCount;
          m_base.frameProduced(data, m_ctx->profile);
          if (isRecording) {
            ffmpeg::currentFrameForOutput(m_ctx, recordingContext);
            ffmpeg::addFrameToOutput(recordingContext);
            ffmpeg::releaseFrameData(recordingContext);
            if (recordingContext->isSnapshot) {
              m_base.emitStreamSnapShotTaken();
              m_running = false;
              m_recording = false;
              break;
            }
          }
        }
      }
      if (isRecording) {
        bool snapshot = recordingContext->isSnapshot;
        ffmpeg::stopOutput(recordingContext);
        if (!snapshot) {
          m_base.emitStreamStoppedRecording();
        }
        recordingContext.reset();
      }
      ffmpeg::stop(*m_ctx);
      m_ctx.reset();
      m_base.emitStreamStopped();
    };
    m_workerThread = std::make_unique<std::thread>(work);
  }

  // Couldn't get real polymorphism to work so need to do this by hand +
  // copy pasting..
  void FFmpegStream::addFrameCallback(const Napi::CallbackInfo& info) {
    m_base.addFrameCallback(info);
  }

  void FFmpegStream::clearFrameCallbacks(const Napi::CallbackInfo& info) {
    m_base.clearFrameCallbacks(info);
  }

  void FFmpegStream::setEventListener(const Napi::CallbackInfo& info) {
    m_base.setEventListener(info);
  }

  void FFmpegStream::removeEventListener(const Napi::CallbackInfo&) {
    m_base.removeEventListener();
  }

  Napi::Value FFmpegStream::enableRemoteStream(const Napi::CallbackInfo& info) {
    return m_base.enableRemoteStream(info);
  }

  void FFmpegStream::disableRemoteStream(const Napi::CallbackInfo& info) {
    m_base.disableRemoteStream(info);
  }

  Napi::Value FFmpegStream::isActive(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), m_running);
  }

  Napi::Value FFmpegStream::latestFrameStats(const Napi::CallbackInfo& info) {
    return m_base.latestFrameStats(info);
  }

  Napi::Value FFmpegStream::isRecording(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), m_recording);
  }

  void FFmpegStream::startRecording(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() == 0 || !info[0].IsObject()) {
      Napi::TypeError::New(env, "Requires RecordingOptions as parameter")
              .ThrowAsJavaScriptException();
    }
    Napi::Object options = info[0].As<Napi::Object>();
    Napi::Value target = options["outputPath"];
    if (!target.IsString()) {
      Napi::TypeError::New(env, "outputPath needs to be string")
              .ThrowAsJavaScriptException();
    }
    startRecording(target.As<Napi::String>(), false);
  }

  void FFmpegStream::startRecording(const std::string& outputPath, bool snapshot) {
    m_recordingContext = std::make_unique<ffmpeg::OutputContext>(outputPath, snapshot);
    m_recording = true;
  }

  void FFmpegStream::stopRecording(const Napi::CallbackInfo&) {
    m_recording = false;
  }

} // namespace video
