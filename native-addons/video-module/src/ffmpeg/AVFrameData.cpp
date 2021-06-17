#include "AVFrameData.hpp"

namespace ffmpeg {

  AVFrameData::AVFrameData(bool masterFrame)
    : m_masterFrame(masterFrame)
  {
    if(m_masterFrame) {
      m_avFrame = av_frame_alloc();
    }
  }

  AVFrameData::AVFrameData(AVFrame* avFrame)
    : m_masterFrame(true)
  {
    m_avFrame = av_frame_alloc();
    refFrame(avFrame, 0);
  }

  void AVFrameData::refFrame(AVFrame* avFrame, size_t plane) {
    if (m_masterFrame) {
      av_frame_ref(m_avFrame, avFrame);
    }
    m_data = avFrame->data[plane];
    m_length = static_cast<unsigned>(avFrame->height * avFrame->linesize[plane]);
    m_width =  static_cast<unsigned>(avFrame->width);
    m_height = static_cast<unsigned>(avFrame->height);
  }

  void AVFrameData::setFrameNumber(unsigned frameNumber) {
    m_frameNumber = frameNumber;
  }

  AVFrameData::~AVFrameData() {
    m_data = nullptr;
    if (m_masterFrame) {
      av_frame_unref(m_avFrame);
      av_frame_free(&m_avFrame);
    }
  }
} // namespace ffmpeg
