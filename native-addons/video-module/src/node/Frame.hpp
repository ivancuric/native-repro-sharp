#pragma once

#include "napi_include.hpp"
#include <memory>

#include "../common.hpp"

namespace video {

  // TODO:
  // - Should the raw data be stored within the stream and allocated
  //   deallocated using Circular buffer or similar
  // - If need for shared memory should the implementation lie in Frame or
  //   Stream?

  /**
   *  FrameData contains the actual data associated to the single frame.
   *
   *  FrameData is only usable from the C++. The approach to Frame & FrameData
   *  needs to be two-tiered (FrameData & Frame) because there is a need to
   *  separate the management of the resources from the Node-interface.
   */
  class FrameData {
  public:
    static std::shared_ptr<FrameData> createTestTexture(int phase, int w, int h, int frameNumber);

    FrameData();
    FrameData(uint8_t* data, unsigned length, unsigned width, unsigned height, unsigned frame);
    virtual ~FrameData();

    unsigned width() const;
    unsigned height() const;

    unsigned length() const;
    unsigned frameNumber() const;

    uint8_t* data();

  protected:
    uint8_t* m_data;
    unsigned m_length;
    unsigned m_width;
    unsigned m_height;
    unsigned m_frameNumber;
  };

  /**
   *  Instances of Frame correspond to single video frame (AVFrame in FFmpeg)
   *
   *  Underlying resources associated to the Frame are tracked by counting
   *  references to raw data (std::shared_ptr<FrameData>).
   *
   */
  class Frame : public Napi::ObjectWrap<Frame> {
  public:
    static Napi::Object Init(
      Napi::Env env,
      Napi::Object exports,
      ConstructorMap& ctors);

    static Napi::Value create(Napi::Env env, std::vector<std::shared_ptr<FrameData>> data);

    Frame(const Napi::CallbackInfo& info);
    ~Frame();

    Napi::Value getTextures(const Napi::CallbackInfo& info);

    Napi::Value width(const Napi::CallbackInfo& info);
    Napi::Value height(const Napi::CallbackInfo& info);
    Napi::Value planes(const Napi::CallbackInfo& info);

    Napi::Value frameNumber(const Napi::CallbackInfo& info);
    unsigned frameNumberRaw() const;

  private:
    std::shared_ptr<FrameData> getPlane(const Napi::CallbackInfo& info) const;

    std::vector<std::shared_ptr<FrameData>> m_data;
  };

} // namespace video
