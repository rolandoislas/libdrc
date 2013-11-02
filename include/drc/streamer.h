#pragma once

#include <drc/pixel-format.h>
#include <drc/types.h>
#include <memory>
#include <string>
#include <vector>

namespace drc {

class VideoConverter;
class VideoStreamer;
class UdpServer;

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

  // Takes ownership of the frame.
  void PushVidFrame(std::vector<byte>& frame, u16 width, u16 height,
                    PixelFormat pixfmt);

  // Same as PushVidFrame, but the frame needs to already be in the native
  // format for encoding: YUV420P at ScreenWidth x ScreenHeight.
  //
  // Faster: PushVidFrame requires pixel format conversion before encoding.
  void PushNativeVidFrame(std::vector<u8>& frame);

 private:
  std::unique_ptr<UdpServer> msg_server_;

  std::unique_ptr<VideoConverter> vid_converter_;
  std::unique_ptr<VideoStreamer> vid_streamer_;
};

}  // namespace drc
