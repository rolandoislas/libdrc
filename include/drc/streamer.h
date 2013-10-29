#pragma once

#include <drc/types.h>
#include <vector>

namespace drc {

class Streamer {
 public:
  static const u16 kDefaultVideoPort = 50120;
  static const u16 kDefaultAudioPort = 50120;
  static const u16 kDefaultMsgPort = 50010;

  Streamer(u16 vid_port = kDefaultVideoPort,
           u16 aud_port = kDefaultAudioPort,
           u16 msg_port = kDefaultMsgPort);
  virtual ~Streamer();

  bool Start();
  void Stop();

  enum PixelFormat {
    kRGB,
    kRGBA,
    kBGR,
    kBGRA,
    kYUV420P,
  };
  void PushFrame(std::vector<u8>& frame, u16 width, u16 height,
                 PixelFormat pixfmt);
};

}  // namespace drc
