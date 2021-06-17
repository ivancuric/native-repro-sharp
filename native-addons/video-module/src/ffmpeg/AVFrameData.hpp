#pragma once
#include "../node/Frame.hpp"

#include "ffmpeg_include.hpp"

namespace ffmpeg {

  class AVFrameData : public video::FrameData {
  public:
    AVFrameData(bool masterFrame);
    AVFrameData(AVFrame* avFrame);

    void refFrame(AVFrame* avFrame, size_t plane);
    void setFrameNumber(unsigned frameNumber);
    virtual ~AVFrameData();

  private:
    AVFrame* m_avFrame;
    bool m_masterFrame;
  };
} // namespace ffmpeg
