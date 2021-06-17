#include "napi_include.hpp"

#include "Frame.hpp"

#include <iostream>

namespace video {

  // Creates simple image sequence for testing out rendering
  std::shared_ptr<FrameData> FrameData::createTestTexture(int phase, int w, int h, int frameNumber) {
    phase = std::min(phase, 3);
    const int hFrac = h / 5;
    const int xMargin = (w - h) / 2;

    const int pixelSize = 3;
    unsigned length = static_cast<unsigned>(w*h*pixelSize);
    uint8_t* texture = new uint8_t[length];

    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {

        bool insideDot = y > (2*hFrac) && y <= (3*hFrac);
        int xn = x - xMargin;
        if(xn >= 0 && xn < phase*2*hFrac) {
          xn = xn % (2*hFrac);
          insideDot = insideDot && xn <= hFrac;
        } else {
          insideDot = false;
        }

        if(insideDot) {
          // #46b84b, Green
          texture[pixelSize*(y*w + x)]     = 0x46; // r
          texture[pixelSize*(y*w + x) + 1] = 0xb8; // g
          texture[pixelSize*(y*w + x) + 2] = 0x4b; // b
        } else {
          // BLACK
          texture[pixelSize*(y*w + x)]     = 0;
          texture[pixelSize*(y*w + x) + 1] = 0;
          texture[pixelSize*(y*w + x) + 2] = 0;
        }
      }
    }
    return std::make_shared<FrameData>(texture, length, w, h, frameNumber);
  }

  FrameData::FrameData()
    : m_data(nullptr),
      m_length(0),
      m_width(0),
      m_height(0),
      m_frameNumber(0)
  {
  }

  FrameData::FrameData(uint8_t* data, unsigned length, unsigned width, unsigned height, unsigned frame)
    : m_data(data),
      m_length(length),
      m_width(width),
      m_height(height),
      m_frameNumber(frame)
  {
  }

  FrameData::~FrameData() {
    if (m_data)
      delete m_data;
  }

  unsigned FrameData::width() const {
    return m_width;
  }

  unsigned FrameData::height() const {
    return m_height;
  }

  unsigned FrameData::length() const {
    return m_length;
  }

  unsigned FrameData::frameNumber() const {
    return m_frameNumber;
  }

  uint8_t* FrameData::data() {
    return m_data;
  }

  Napi::Object Frame::Init(
    Napi::Env env,
    Napi::Object exports,
    ConstructorMap& ctors)
  {
    Napi::Function func = DefineClass(env, "Frame", {
      InstanceMethod<&Frame::getTextures>("getTextures"),
      InstanceMethod<&Frame::width>("width"),
      InstanceMethod<&Frame::height>("height"),
      InstanceMethod<&Frame::frameNumber>("frameNumber"),
      InstanceMethod<&Frame::planes>("planes"),
    });

    exports.Set("Frame", func);

    auto ctor = std::make_unique<Napi::FunctionReference>();
    *ctor = Napi::Persistent(func);
    ctors[FRAME_CTOR] = std::move(ctor);

    return exports;
  }

  Napi::Value Frame::create(Napi::Env env, std::vector<std::shared_ptr<FrameData>> data)
  {
    if (data.empty())
      return env.Null();
    ConstructorMap& ctors = env.GetInstanceData<InstanceData>()->constructors;
    auto& ctor = ctors[FRAME_CTOR];
    auto frame = ctor->New({});
    Frame::Unwrap(frame)->m_data = data;
    return frame;
  }

  Frame::Frame(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<Frame>(info)
  {}

  Frame::~Frame() {
  }

  std::shared_ptr<FrameData> Frame::getPlane(const Napi::CallbackInfo& info) const {
    size_t plane = 0;
    if (info.Length() > 0 && info[0].IsNumber()) {
      plane = info[0].As<Napi::Number>().Uint32Value();
    }
    return m_data[plane];
  }

  Napi::Value Frame::width(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), getPlane(info)->width());
  }

  Napi::Value Frame::height(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), getPlane(info)->height());
  }

  Napi::Value Frame::planes(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), m_data.size());
  }

  Napi::Value Frame::getTextures(const Napi::CallbackInfo& info) {
    Napi::Array result = Napi::Array::New(info.Env());

    for(size_t i = 0; i < m_data.size(); ++i) {
      std::shared_ptr<FrameData> data = m_data[i];
      auto buffer = Napi::Buffer<uint8_t>::New(
        info.Env(),
        data->data(),
        data->length());
      result.Set(uint32_t(i), buffer);
    }
    return result;
  }

  Napi::Value Frame::frameNumber(const Napi::CallbackInfo& info) {
    return Napi::Number::New(info.Env(), m_data[0]->frameNumber());
  }

  unsigned Frame::frameNumberRaw() const {
    return m_data[0]->frameNumber();
  }

} // namespace video
