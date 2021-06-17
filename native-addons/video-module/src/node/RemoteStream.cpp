#include "RemoteStream.hpp"

#include "Frame.hpp"

#include <iostream>
namespace video {

  Napi::Object RemoteStream::Init(
    Napi::Env env,
    Napi::Object exports,
    ConstructorMap& ctors)
  {
    Napi::Function func = DefineClass(env, "RemoteStream", {
      InstanceMethod<&RemoteStream::getLatestFrame>("getLatestFrame"),
      InstanceMethod<&RemoteStream::streamId>("streamId"),
    });
    exports.Set("RemoteStream", func);

    auto ctor = std::make_unique<Napi::FunctionReference>();
    *ctor = Napi::Persistent(func);
    ctors[REMOTE_STREAM_CTOR] = std::move(ctor);

    return exports;
  }

  RemoteStream::RemoteStream(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<RemoteStream>(info)
  {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      throw Napi::TypeError::New(env, "Expected first argument to be string");
    }
    std::string remoteId = info[0].As<Napi::String>();
    m_sharedMemory = utils::SharedMemory::initReader(remoteId);
  }

  RemoteStream::~RemoteStream() {
    m_sharedMemory.reset();
  }

  Napi::Value RemoteStream::streamId(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), m_sharedMemory->memoryId());
  }

  Napi::Value RemoteStream::getLatestFrame(const Napi::CallbackInfo& info) {
    auto data = m_sharedMemory->read();
    return Frame::create(info.Env(), data);
  }

} // namespace video
