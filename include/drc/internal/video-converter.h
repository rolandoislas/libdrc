#pragma once

#include <drc/pixel-format.h>
#include <drc/types.h>
#include <future>
#include <map>
#include <utility>

struct SwsContext;

namespace drc {

// Defined as a tuple to avoid redefining == and <.
// Fields are w/h/pixfmt/flipv.
typedef std::tuple<u16, u16, PixelFormat, bool> VideoConverterParams;

class VideoConverter {
 public:
  typedef std::function<void(std::vector<byte>&)> DoneCallback;

  VideoConverter();
  virtual ~VideoConverter();

  bool Start();
  void Stop();

  void PushFrame(std::vector<byte>& frame, const VideoConverterParams& params);

  void SetDoneCallback(DoneCallback cb) { done_cb_ = cb; }

 private:
  SwsContext* GetContextForParams(const VideoConverterParams& params);
  void DoConversion();

  DoneCallback done_cb_;
  std::map<VideoConverterParams, SwsContext*> ctxs_;

  std::future<void> current_conv_;

  std::vector<byte> current_frame_;
  VideoConverterParams current_params_;
};

}  // namespace drc
