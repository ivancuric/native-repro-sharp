#include "PerfLoggerWrapper.hpp"

namespace video {

  Napi::Object PerfLoggerWrapper::Init(
    Napi::Env env,
    Napi::Object exports,
    ConstructorMap& ctors)
  {
    Napi::Function func = DefineClass(env, "PerfLogger", {
      InstanceMethod<&PerfLoggerWrapper::log>("log"),
      InstanceMethod<&PerfLoggerWrapper::flush>("flush"),
    });

    exports.Set("PerfLogger", func);

    auto ctor = std::make_unique<Napi::FunctionReference>();
    *ctor = Napi::Persistent(func);
    ctors[PERF_LOG_CTOR] = std::move(ctor);

    return exports;
  }

  PerfLoggerWrapper::PerfLoggerWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<PerfLoggerWrapper>(info)
  {
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsBoolean()) {
      throw Napi::TypeError::New(info.Env(), "Expected first argument to be a string and second boolean");
    }
    m_perfLogger = utils::PerfLogger::instance(info[0].As<Napi::String>(), info[1].As<Napi::Boolean>());
  }

  PerfLoggerWrapper::~PerfLoggerWrapper() {}

  void PerfLoggerWrapper::log(const Napi::CallbackInfo& info) {
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
      throw Napi::TypeError::New(info.Env(), "Expected arguments to be integers");
    }
    m_perfLogger->log(
      static_cast<utils::Key>(info[0].As<Napi::Number>().Int32Value()),
      info[1].As<Napi::Number>().Int32Value());
  }

  void PerfLoggerWrapper::flush(const Napi::CallbackInfo&) {
    m_perfLogger->releaseMemory(false);
  }


} // namespace video
