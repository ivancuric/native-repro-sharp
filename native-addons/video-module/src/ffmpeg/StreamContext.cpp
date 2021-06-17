#include "StreamContext.hpp"

namespace ffmpeg {

  StreamContext::~StreamContext() {
    av_frame_free(&frame);
    av_parser_close(parser);
    avcodec_free_context(&codecContext);
    if (open) {
      avformat_close_input(&formatContext);
    }
    avformat_free_context(formatContext);
  }

} //namespace ffmpeg
