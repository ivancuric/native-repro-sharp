#include "CapturePrint.hpp"

#include <cstdio>
#include <thread>
#include <vector>
#include <iostream>

#include "ffmpeg_include.hpp"

namespace ffmpeg {

  static CapturePrint* cp = nullptr;

  static void avlog(void*, int level, const char* format, va_list va) {
    if (level < AV_LOG_ERROR) {
      return;
    }
    char buf[10000];
    size_t n = static_cast<size_t>(vsprintf(buf, format, va));
    buf[n] = 0;
    std::lock_guard<std::mutex> g(cp->m_mutex);
    size_t s = cp->m_target.size();
    cp->m_target.resize(s + n);
    memcpy(cp->m_target.data() + s, buf, n);
  }

  CapturePrint::CapturePrint(std::vector<char>& target)
    : m_target(target)
  {
    av_log_set_callback(&avlog);
    cp = this;
  }

  CapturePrint::~CapturePrint() {
    av_log_set_callback(av_log_default_callback);
    cp = nullptr;
  }

} // namespace ffmpeg
