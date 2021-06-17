#pragma once

#include "ffmpeg_include.hpp"

#include <string>

namespace ffmpeg {

  struct StreamContext {
    AVFormatContext* formatContext = nullptr;
    AVCodec* codec = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVCodecParserContext* parser = nullptr;
    AVFrame* frame = nullptr;
    int streamIndex = -1;
    bool open = false;

    int frameNumber = 0;
    bool profile = false;
    std::string name = "";

    ~StreamContext();
  };

} //namespace ffmpeg
