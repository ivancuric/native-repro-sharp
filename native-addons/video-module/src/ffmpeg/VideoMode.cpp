#include "VideoMode.hpp"

#include <string>

namespace ffmpeg {

  bool VideoMode::operator<(const VideoMode& o) const {
      return std::make_tuple(w, h, fps) < std::make_tuple(o.w, o.h, o.fps);
  }

  AVDictionary* VideoMode::toOptions() const {
    AVDictionary* options = nullptr;
    av_dict_set(&options, "framerate", std::to_string(fps).c_str(), 0);
    std::string size = std::to_string(w) + "x" + std::to_string(h);
    av_dict_set(&options, "video_size", size.c_str(), 0);
    return options;
  }

} //namespace ffmpeg
