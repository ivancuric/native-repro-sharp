#pragma once

#include <cstdint>

extern "C" {
#include "../disable_warnings_begin.hpp"

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>

#include "../disable_warnings_end.hpp"
}
