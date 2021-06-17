#pragma once

#include <mutex>
#include <vector>

namespace ffmpeg {

  // Class to capture FFmpeg prints. Some of the APIs only emit prints
  class CapturePrint {
  public:
    CapturePrint(std::vector<char>& target);
    ~CapturePrint();

  public:
    std::mutex m_mutex;
    std::vector<char>& m_target;
  };

} // namespace ffmpeg
