#pragma once

#include "napi_include.hpp"
#include <memory>

#include "../common.hpp"

#include "../utils/PerfLogger.hpp"

namespace video {

  class PerfLoggerWrapper : public Napi::ObjectWrap<PerfLoggerWrapper> {
  public:
    static Napi::Object Init(
      Napi::Env env,
      Napi::Object exports,
      ConstructorMap& ctors);

    PerfLoggerWrapper(const Napi::CallbackInfo& info);
    ~PerfLoggerWrapper();

    void log(const Napi::CallbackInfo& info);
    void flush(const Napi::CallbackInfo& info);

  private:
    std::shared_ptr<utils::PerfLogger> m_perfLogger;
  };

} // namespace video
