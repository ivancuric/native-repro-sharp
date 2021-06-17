#pragma once

#include <tuple>

#include "ffmpeg_include.hpp"

namespace ffmpeg {

  struct VideoMode {
  public:
    VideoMode(): w(0), h(0), fps(0), profile(false) {}

    VideoMode(int x, int y, int _fps, bool _profile)
      : w(x), h(y), fps(_fps), profile(_profile) {}

    inline bool isValid() const {
      return w * h * fps > 0;
    }

    bool operator<(const VideoMode& o) const;

    AVDictionary* toOptions() const;

    int w;
    int h;
    int fps;
    // Stats are recorded always, if this is true they are also saved to file
    bool profile;
  };

} //namespace ffmpeg
