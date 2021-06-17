#pragma once

#include "ffmpeg_include.hpp"

#include <string>

namespace ffmpeg {

  struct OutputContext {
    OutputContext(const std::string& fileName, bool snapshot)
      : outputPath(fileName),
        isSnapshot(snapshot)
    {}

    const std::string outputPath;
    bool isSnapshot;
    AVFormatContext* formatContext = nullptr;
    AVOutputFormat* outputFormat = nullptr;
    AVCodec* codec = nullptr;
    AVStream *stream = nullptr;
    AVCodecContext* codecContext = nullptr;
    AVFrame* frame = nullptr;
    SwsContext* swsContext = nullptr;
    int64_t nextPts = 0;
    bool encodingFrames = false;
  };

}
