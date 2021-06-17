#include "ffmpeg.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "../utils/PerfLogger.hpp"

#include "ffmpeg_include.hpp"
#include "CapturePrint.hpp"

static AVInputFormat* getInputFormat() {
#ifdef _WIN32
  return av_find_input_format("dshow");
#elif __APPLE__
  return av_find_input_format("avfoundation");
#else
  return av_find_input_format("v4l2");
#endif
}

static void freeOptionsAfterUse(AVDictionary** options) {
  AVDictionaryEntry* it = nullptr;
  while((it = av_dict_get(*options, "", it, AV_DICT_IGNORE_SUFFIX))) {
    std::cout << "Unrecognized option "
              << it->key << "=" << it->value << std::endl;
  }
  av_dict_free(options);
  *options = nullptr;
}

static std::vector<std::string> toLines(const std::vector<char>& data) {
  std::vector<std::string> lines;
  auto begin = data.begin();
  auto end = std::find(begin, data.end(), '\n');
  while(end != data.end()) {
    lines.push_back(std::string(begin, end));
    // Do something for the line
    begin = ++end;
    end = std::find(begin, data.end(), '\n');
  }
  return lines;
}

static bool videoDeviceLine(const std::string& line) {
#ifdef _WIN32
  return line.find("Alternative name") == std::string::npos;
#else
  return line.find("Capture screen") == std::string::npos;
#endif
}

static void mangleLineToName(std::string& line) {
#ifdef _WIN32
  auto end = std::find(line.rbegin(), line.rend(), '"');
  ++end;
  auto begin = std::find(end, line.rend(), '"');
  auto fromEnd = static_cast<unsigned long>(begin - line.rbegin());
  line = line.substr(line.size() - fromEnd, begin - end);
#else
  auto i = std::find(line.rbegin(), line.rend(), '[');
  auto fromEnd = static_cast<unsigned long>(i - line.rbegin() + 1);
  line = line.substr(line.size() - fromEnd);
#endif
}

static std::vector<std::string>
parseVideoNames(const std::vector<char>& ffmpegOutput)
{
  auto lines = toLines(ffmpegOutput);

  bool isVideoDeviceLine = false;
  for (auto it = lines.begin(); it != lines.end(); ) {
    std::string& line = *it;
    if (line.find("video devices") != std::string::npos) {
      isVideoDeviceLine = true;
    } else if (line.find("audio devices") != std::string::npos) {
      isVideoDeviceLine = false;
    } else if(isVideoDeviceLine && videoDeviceLine(line)) {
      mangleLineToName(line);
      ++it;
      continue;
    }
    it = lines.erase(it);
  }
  return lines;
}

// Input has form 1920x1080
static std::pair<int, int> parseResolution(const std::string& resolution) {
  auto x = resolution.find('x');
  std::string w = resolution.substr(0, x);
  std::string h = resolution.substr(x + 1);
  return std::make_pair(std::stoi(w), std::stoi(h));
}

static ffmpeg::VideoMode parseMode(const std::string& line) {
  std::pair<int, int> r;
  int fps;
#ifndef _WIN32
  {
    auto e = std::find(line.rbegin(), line.rend(), '@');
    ++e;
    auto b = std::find(e, line.rend(), ' ');
    auto len = static_cast<unsigned long>(b - e);
    auto startIndex = line.size() - static_cast<unsigned long>(b - line.rbegin());
    auto resolutionString = line.substr(startIndex, len);
    r = parseResolution(resolutionString);
  }
  {
    auto it = std::find(line.rbegin(), line.rend(), '[');
    --it;
    auto fromEnd = static_cast<unsigned long>(it - line.rbegin() + 1);
    auto startIndex = line.size() - fromEnd;
    unsigned long l = 0;
    while (isdigit(*it)) {
      ++l;
      --it;
    }
    fps = std::stoi(line.substr(startIndex, l));
  }
#else
  {
    auto ss = line.find(" s=");
    auto it = line.begin();
    std::advance(it, ss + 3);
    it = std::find(it, line.end(), ' ');
    auto resolutionString = line.substr(ss + 3, it - line.begin() - ss - 3);
    r = parseResolution(resolutionString);
  }
  {
    auto ss = line.find("fps=");
    auto it = line.begin();
    std::advance(it, ss + 4);
    it = std::find(it, line.end(), ' ');
    auto fpsString = line.substr(ss + 4, it - line.begin() - ss - 4);
    fps = std::stoi(fpsString);
  }
#endif
  return ffmpeg::VideoMode(r.first, r.second, fps, false);
}

static std::vector<ffmpeg::VideoMode>
parseVideoModes(const std::vector<char>& ffmpegOutput) {
  auto lines = toLines(ffmpegOutput);

  std::set<ffmpeg::VideoMode> combinations;
  for(const std::string& line : lines) {
    if (line.find("fps") != std::string::npos && line.size() > 16) {
      combinations.emplace(parseMode(line));
    }
  }
  std::vector<ffmpeg::VideoMode> res;
  std::copy(combinations.begin(), combinations.end(), std::back_inserter(res));
  return res;
}


static std::string getFFmpegIdentifier(const std::string& deviceName) {
#ifndef _WIN32
  auto b = deviceName.find('[');
  auto e = deviceName.find(']');
  return deviceName.substr(b + 1, e - b - 1);
#else
  return "video=" + deviceName;
#endif
}

namespace ffmpeg {

  void init() {
    av_log_set_level(AV_LOG_ERROR);
    avformat_network_init();
    avdevice_register_all();
  }

  std::vector<std::string> inputVideoDevices() {
    std::vector<char> data;
    {
      CapturePrint g(data);

      AVFormatContext* avfmt = avformat_alloc_context();
      AVDictionary* options = nullptr;
      av_dict_set(&options, "list_devices", "true", 0);
      AVInputFormat* iformat = getInputFormat();
      avformat_open_input(&avfmt, nullptr, iformat, &options);

      av_dict_free(&options);
      avformat_close_input(&avfmt);
      avformat_free_context(avfmt);
    }
    return parseVideoNames(data);
  }

  std::vector<VideoMode> getVideoModes(const std::string& deviceName) {
    std::vector<char> data;
    {
      CapturePrint g(data);
      AVFormatContext* avfmt = avformat_alloc_context();
      AVInputFormat* iformat = getInputFormat();

      AVDictionary* options = nullptr;
      av_dict_set(&options, "list_options", "true", 0);
      std::string id = getFFmpegIdentifier(deviceName);
      avformat_open_input(&avfmt, id.c_str(), iformat, &options);

      av_dict_free(&options);
      avformat_close_input(&avfmt);
      avformat_free_context(avfmt);
    }
    return parseVideoModes(data);
  }

  std::unique_ptr<StreamContext>
  start(const std::string& deviceName, VideoMode mode)
  {
    std::unique_ptr<StreamContext> ctx = std::make_unique<StreamContext>();
    ctx->profile = mode.profile;
    ctx->name = deviceName;
    ctx->formatContext = avformat_alloc_context();

    AVInputFormat* iformat = getInputFormat();
    std::string id = getFFmpegIdentifier(deviceName);

    AVDictionary *options = mode.toOptions();
    // pixelformat?
    int err = avformat_open_input(&ctx->formatContext, id.c_str(), iformat, &options);
    freeOptionsAfterUse(&options);

    if (err != 0) {
      std::cout << "Failed to open input" << std::endl;
      return nullptr;
    }
    ctx->open = true;
    err = avformat_find_stream_info(ctx->formatContext, nullptr);
    if (err < 0) {
      std::cout << "Failed to find stream info" << std::endl;
      return nullptr;
    }

    // Note now we are selecting codec automatically, see if it is worth
    // of manual fiddle
    ctx->streamIndex = av_find_best_stream(ctx->formatContext,
                                           AVMEDIA_TYPE_VIDEO,
                                           -1, -1, &ctx->codec, 0);
    if (ctx->streamIndex < 0) {
      std::cout << "ERROR " << std::endl;
      return nullptr;
    }
    ctx->codecContext = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(
      ctx->codecContext, ctx->formatContext->streams[ctx->streamIndex]->codecpar);
    ctx->codecContext->codec_id = ctx->codec->id;
    //ctx->codecContext->opaque = ctx.get(); TODO: is this needed
    ctx->codecContext->thread_count = 4; // threads, could also try setting automatically


    // Open codecs
    // video options for avcodec_open2 ??
    err = avcodec_open2(ctx->codecContext, ctx->codec, &options);
    freeOptionsAfterUse(&options);
    if (err < 0) {
      std::cout << "Couldn't open codec" << std::endl;
      return nullptr;
    }
    std::cout << "Opened stream " << deviceName << " with resolution "
              << ctx->codecContext->width << "x" << ctx->codecContext->height
              << std::endl;
    ctx->frame = av_frame_alloc();
    return ctx;
  }

  AVFrame *initFrame(AVPixelFormat pixFmt, int width, int height) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
      return nullptr;
    }
    frame->format = pixFmt;
    frame->width = width;
    frame->height = height;
    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
      av_frame_free(&frame);
      return nullptr;
    }
    return frame;
  }

  void showFormats() {
    void *opaque = nullptr;
    while (const AVOutputFormat* format = av_muxer_iterate(&opaque)) {
      std::cout << format->name << " ";
      if (format->extensions)
        std::cout << " ext: " << format->extensions;
      if (format->video_codec)
        std::cout << " vcodec: " << format->video_codec;
      std::cout << std::endl;
    }
  }

  std::optional<std::string> initOutput(std::unique_ptr<OutputContext>& ctx, std::unique_ptr<StreamContext>& input) {

    avformat_alloc_output_context2(&ctx->formatContext, nullptr, nullptr, ctx->outputPath.c_str());
    if (!ctx->formatContext) {
      // Could not deduce output format from file extension -> mpeg
      std::string format = ctx->isSnapshot ? "jpg" : "mpeg";
      avformat_alloc_output_context2(&ctx->formatContext, nullptr, format.c_str(), ctx->outputPath.c_str());
    }
    if (!ctx->formatContext) {
      return std::make_optional("Couldn't allocate format context");
    }

    ctx->outputFormat = ctx->formatContext->oformat;
    // Assume video
    ctx->codec = avcodec_find_encoder(ctx->formatContext->oformat->video_codec);
    if (!(ctx->codec)) {
      return std::make_optional("Couldn't find encoder codec");
    }

    ctx->stream = avformat_new_stream(ctx->formatContext, ctx->codec /* nullptr */);
    if (!ctx->stream) {
      return std::make_optional("Couldn't create stream");
    }
    ctx->stream->id = static_cast<int>(ctx->formatContext->nb_streams - 1);

    ctx->codecContext = avcodec_alloc_context3(ctx->codec);
    if (!ctx->codecContext) {
      return std::make_optional("Couldn't allocate codec context");
    }
    ctx->codecContext->codec_id = ctx->formatContext->oformat->video_codec;
    ctx->codecContext->bit_rate = input->codecContext->bit_rate;
    ctx->codecContext->width = input->codecContext->width;
    ctx->codecContext->height = input->codecContext->height;
    // TODO: FPS from input?
    ctx->codecContext->time_base = ctx->stream->time_base = AVRational{1, 25 /* fps */};
    ctx->codecContext->gop_size = 12; // intra frame at most every 12 frames
    ctx->codecContext->pix_fmt = ctx->isSnapshot ? AV_PIX_FMT_YUVJ420P : AV_PIX_FMT_YUV420P;
    if (ctx->formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
      ctx->codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary* options = nullptr;
    // TODO: optionsiin directly from JS-options?
    int ret = avcodec_open2(ctx->codecContext, ctx->codec, &options);
    av_dict_free(&options);
    if (ret < 0) {
      return std::make_optional("Couldn't open codec");
    }
    // Need frame if input format is not yuv420p
    if (input->codecContext->pix_fmt != ctx->codecContext->pix_fmt) {
      ctx->encodingFrames = true;
      ctx->frame = initFrame(ctx->codecContext->pix_fmt, ctx->codecContext->width,
                             ctx->codecContext->height);
      if (!ctx->frame) {
        return std::make_optional("Couldn't init frame");
      }
      ctx->swsContext = sws_getContext(
        input->codecContext->width,
        input->codecContext->height,
        input->codecContext->pix_fmt,
        ctx->codecContext->width,
        ctx->codecContext->height,
        ctx->codecContext->pix_fmt,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
      );
      if (!ctx->swsContext) {
        return std::make_optional("Couldn't allocate pix_fmt transform");
      }
    } else {
      ctx->encodingFrames = false;
    };
    ret = avcodec_parameters_from_context(ctx->stream->codecpar, ctx->codecContext);
    if (ret < 0) {
      return std::make_optional("Couldn't copy parameters to codec");
    }
    av_dump_format(ctx->formatContext, 0, ctx->outputPath.c_str(), 1);
    if (!(ctx->outputFormat->flags & AVFMT_NOFILE)) {
      ret = avio_open(&ctx->formatContext->pb, ctx->outputPath.c_str(), AVIO_FLAG_WRITE);
      if (ret < 0) {
        return std::make_optional("Couldn't open output");
      }
    }
    // TODO: Options again?
    options = nullptr;
    ret = avformat_write_header(ctx->formatContext, &options);
    av_dict_free(&options);
    if (ret < 0) {
      return std::make_optional("Couldn't write header");
    }
    return std::optional<std::string>();
  }

  void currentFrameForOutput(
    std::unique_ptr<StreamContext>& input,
    std::unique_ptr<OutputContext>& output)
  {
    if (!output->encodingFrames) {
      av_frame_ref(output->frame, input->frame);
    }  else {
      // Need to do conversion
      int ok = av_frame_make_writable(output->frame);
      if (ok < 0) {
        // Fail
      }
      sws_scale(output->swsContext, reinterpret_cast<const uint8_t * const *>(input->frame->data), input->frame->linesize,
        0, output->codecContext->height, output->frame->data, output->frame->linesize);
    }
    output->frame->pts = output->nextPts++;
  }

  void addFrameToOutput(std::unique_ptr<OutputContext>& output) {
    int ret = avcodec_send_frame(output->codecContext, output->frame);
    if (ret < 0) {
      // Fail
    }
    while (ret >= 0) {
      AVPacket pkt = { nullptr };
      ret = avcodec_receive_packet(output->codecContext, &pkt);
      if(tryagain(ret)) {
        break;
      } else if (ret < 0) {
        // Fatal fail
      }
      av_packet_rescale_ts(&pkt, output->codecContext->time_base, output->stream->time_base);
      pkt.stream_index = output->stream->index;

      // Write
      ret = av_interleaved_write_frame(output->formatContext, &pkt);
      av_packet_unref(&pkt);
      if (ret < 0) {
        // Fail
      }
    }
  }

  void releaseFrameData(std::unique_ptr<OutputContext>& output) {
    if (!output->encodingFrames) {
      av_frame_unref(output->frame);
    }
  }

  void stopOutput(std::unique_ptr<OutputContext>& output) {
    av_write_trailer(output->formatContext);
    avcodec_free_context(&output->codecContext);
    if (output->encodingFrames) {
      av_frame_free(&output->frame);
    }
    sws_freeContext(output->swsContext);

    if (!(output->outputFormat->flags & AVFMT_NOFILE)) {
      avio_closep(&output->formatContext->pb);
    }
    avformat_free_context(output->formatContext);
    output.reset();
  }

  int prepareFrame(StreamContext& ctx) {
    AVPacket packet;
    int err = av_read_frame(ctx.formatContext, &packet);
    if (err < 0) {
      return err;
    }
    utils::PerfLogger::logEntry(ctx.name, utils::Key::Received, ctx.frameNumber, ctx.profile);

    err = avcodec_send_packet(ctx.codecContext, &packet);
    if(err < 0) {
      std::cout << "Error in avcodec_send_packet" << std::endl;
      av_packet_unref(&packet);
      return err;
    }

    av_packet_unref(&packet);
    return err;
  }

  int receiveFrame(StreamContext& ctx) {
    return avcodec_receive_frame(ctx.codecContext, ctx.frame);
  }

  void stop(StreamContext& ctx) {
    av_frame_unref(ctx.frame);
    avcodec_send_packet(ctx.codecContext, nullptr);
  }

} // namespace ffmpeg
