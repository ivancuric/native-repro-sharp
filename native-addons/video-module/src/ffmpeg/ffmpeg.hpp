#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "OutputContext.hpp"
#include "StreamContext.hpp"
#include "VideoMode.hpp"

namespace ffmpeg {

  void init();
  std::vector<std::string> inputVideoDevices();

  std::vector<VideoMode> getVideoModes(const std::string& deviceName);

  std::unique_ptr<StreamContext> start(const std::string& deviceName, VideoMode mode);
  std::optional<std::string> initOutput(std::unique_ptr<OutputContext>& ctx, std::unique_ptr<StreamContext>& input);
  AVFrame* initFrame(AVPixelFormat pixFmt, int width, int height);
  void currentFrameForOutput(std::unique_ptr<StreamContext>& input, std::unique_ptr<OutputContext>& output);

  void addFrameToOutput(std::unique_ptr<OutputContext>& output);
  void releaseFrameData(std::unique_ptr<OutputContext>& output);
  void stopOutput(std::unique_ptr<OutputContext>& output);

  int prepareFrame(StreamContext& ctx);
  int receiveFrame(StreamContext& ctx);
  void stop(StreamContext& ctx);

  void showFormats();

#include "../disable_warnings_begin.hpp"
  inline bool tryagain(int errorCode) {
    return errorCode == AVERROR(EAGAIN) || errorCode == AVERROR_EOF;
  }
#include "../disable_warnings_end.hpp"


} // namespace ffmpeg
