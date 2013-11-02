#include <cstring>
#include <drc/internal/udp.h>
#include <drc/internal/video-converter.h>
#include <drc/streamer.h>
#include <vector>

namespace drc {

// TODO: remove
class VideoStreamer {
 public:
  VideoStreamer(const std::string& dst) {}
  virtual ~VideoStreamer() {}

  bool Start() { return true; }
  void Stop() {}

  void PushFrame(std::vector<byte>& frame) {}
  void ResyncStream() {}
};

Streamer::Streamer(const std::string& vid_dst,
                   const std::string& aud_dst,
                   const std::string& msg_bind) 
    : msg_server_(new UdpServer(msg_bind)),
      vid_converter_(new VideoConverter()),
      vid_streamer_(new VideoStreamer(vid_dst)) {
}

Streamer::~Streamer() {
}

bool Streamer::Start() {
  msg_server_->SetReceiveCallback(
      [=](const std::vector<byte>& msg) {
        if (msg.size() == 4 && memcmp(msg.data(), "\1\0\0\0", 4)) {
          vid_streamer_->ResyncStream();
        }
        return true;
      });

  vid_converter_->SetDoneCallback(
      [=](std::vector<byte>& yuv_frame) {
        PushNativeVidFrame(yuv_frame);
      });

  if (!msg_server_->StartListening() ||
      !vid_converter_->Start() ||
      !vid_streamer_->Start()) {
    Stop();
    return false;
  }
  return true;
}

void Streamer::Stop() {
  msg_server_->StopListening();
  vid_converter_->Stop();
  vid_streamer_->Stop();
}

void Streamer::PushVidFrame(std::vector<byte>& frame, u16 width, u16 height,
                            PixelFormat pixfmt) {
  vid_converter_->PushFrame(frame, std::make_tuple(width, height, pixfmt));
}

void Streamer::PushNativeVidFrame(std::vector<byte>& frame) {
  vid_streamer_->PushFrame(frame);
}

}  // namespace drc
