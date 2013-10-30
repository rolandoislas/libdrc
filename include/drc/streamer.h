#pragma once

#include <drc/types.h>
#include <string>
#include <vector>

namespace drc {

class Streamer {
 public:
  static constexpr const char* kDefaultVideoDest = "192.168.1.11:50120";
  static constexpr const char* kDefaultAudioDest = "192.168.1.11:50121";
  static constexpr const char* kDefaultMsgBind = "192.168.1.10:50010";

  Streamer(const std::string& vid_dst = kDefaultVideoDest,
           const std::string& aud_dst = kDefaultAudioDest,
           const std::string& msg_bind = kDefaultMsgBind);
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
