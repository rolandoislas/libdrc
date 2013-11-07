#include <array>
#include <cassert>
#include <drc/internal/video-converter.h>
#include <drc/pixel-format.h>
#include <drc/screen.h>
#include <future>
#include <utility>

extern "C" {
#include <libswscale/swscale.h>
}  // extern "C"

#undef PixelFormat // fuck ffmpeg

namespace drc {

namespace {

AVPixelFormat ConvertPixFmt(PixelFormat pixfmt) {
  switch (pixfmt) {
    case PixelFormat::kRGB: return AV_PIX_FMT_RGB24;
    case PixelFormat::kRGBA: return AV_PIX_FMT_RGBA;
    case PixelFormat::kBGR: return AV_PIX_FMT_BGR24;
    case PixelFormat::kBGRA: return AV_PIX_FMT_BGRA;
    default: assert(false && "Invalid PixelFormat value");
  }
}

std::array<const byte*, 4> GetPixFmtPlanes(PixelFormat pixfmt,
                                           const byte* data) {
  switch (pixfmt) {
    case PixelFormat::kRGB:
    case PixelFormat::kRGBA:
    case PixelFormat::kBGR:
    case PixelFormat::kBGRA:
      return {data, NULL, NULL, NULL};
    default:
      assert(false && "Invalid PixelFormat value");
  }
}
std::array<int, 4> GetPixFmtStrides(PixelFormat pixfmt, u16 width,
                                    u16 height) {
  switch (pixfmt) {
    case PixelFormat::kRGB:
    case PixelFormat::kBGR:
      return {width * 3, 0, 0, 0};
    case PixelFormat::kRGBA:
    case PixelFormat::kBGRA:
      return {width * 4, 0, 0, 0};
    default:
      assert(false && "Invalid PixelFormat value");
  }
}

std::array<int, 4> GetPixFmtHeights(PixelFormat pixfmt, u16 height) {
  switch (pixfmt) {
    case PixelFormat::kRGB:
    case PixelFormat::kBGR:
    case PixelFormat::kRGBA:
    case PixelFormat::kBGRA:
      return {height, 0, 0, 0};
    default:
      assert(false && "Invalid PixelFormat value");
  }
}

}  // namespace

VideoConverter::VideoConverter() {
}

VideoConverter::~VideoConverter() {
  Stop();

  // Free any allocated SwsContext.
  for (const auto& it : ctxs_) {
    sws_freeContext(it.second);
  }
}

bool VideoConverter::Start() {
  return true;
}

void VideoConverter::Stop() {
  if (!current_conv_.valid()) {
    return;
  }

  current_conv_.wait();
}

void VideoConverter::PushFrame(std::vector<byte>& frame,
                               const VideoConverterParams& params) {
  if (current_conv_.valid()) {
    current_conv_.wait();
  }

  // Capture the current frame before spawning the conversion thread: we do not
  // know when the thread will be scheduled, and the user might have destroyed
  // the vector containing the frame before that.
  current_frame_ = std::move(frame);
  current_params_ = params;

  current_conv_ = std::async(std::launch::async,
      [=]() { DoConversion(); });
}

SwsContext* VideoConverter::GetContextForParams(
    const VideoConverterParams& params) {
  u16 width, height;
  PixelFormat pixfmt;
  bool flipv;

  std::tie(width, height, pixfmt, flipv) = params;

  SwsContext* ctx = sws_getCachedContext(ctxs_[params],
      width, height, ConvertPixFmt(pixfmt),
      kScreenWidth, kScreenHeight, PIX_FMT_YUV420P,
      SWS_FAST_BILINEAR, NULL, NULL, NULL);

  ctxs_[params] = ctx;
  return ctx;
}

void VideoConverter::DoConversion() {
  int y_size = kScreenWidth * kScreenHeight;
  int u_size = y_size / 4;
  int v_size = y_size / 4;
  std::vector<byte> converted(y_size + u_size + v_size);

  SwsContext* ctx = GetContextForParams(current_params_);

  u16 width, height;
  PixelFormat pixfmt;
  bool flipv;
  std::tie(width, height, pixfmt, flipv) = current_params_;

  byte* converted_planes[4] = {
      converted.data(),
      converted.data() + y_size,
      converted.data() + y_size + u_size,
      NULL
  };
  int converted_strides[4] = {
      kScreenWidth,
      kScreenWidth / 2,
      kScreenWidth / 2,
      0
  };
  std::array<const byte*, 4> planes =
      GetPixFmtPlanes(pixfmt, current_frame_.data());
  std::array<int, 4> strides = GetPixFmtStrides(pixfmt, width, height);

  if (flipv) {
    std::array<int, 4> heights = GetPixFmtHeights(pixfmt, height);
    for (int i = 0; i < 4; ++i) {
      int plane_size = strides[i] * heights[i];
      planes[i] += plane_size - strides[i];
      strides[i] = -strides[i];
    }
  }

  int res = sws_scale(ctx, planes.data(), strides.data(), 0, height,
                      converted_planes, converted_strides);

  done_cb_(converted);
}

}  // namespace drc
