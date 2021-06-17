#pragma once

#include "../common.hpp"
#include "../utils/SharedMemory.hpp"

namespace video {

  class RemoteStream : public Napi::ObjectWrap<RemoteStream> {
  public:
    static Napi::Object Init(
      Napi::Env env,
      Napi::Object exports,
      ConstructorMap& ctors);

    // Takes in name
    RemoteStream(const Napi::CallbackInfo& info);
    ~RemoteStream();

    Napi::Value getLatestFrame(const Napi::CallbackInfo& info);
    Napi::Value streamId(const Napi::CallbackInfo& info);

  private:
    std::unique_ptr<utils::SharedMemory> m_sharedMemory;
  };

} // namespace video
