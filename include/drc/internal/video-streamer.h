#pragma once

#include <drc/types.h>
#include <memory>
#include <string>

namespace drc {

class H264Encoder;
class UdpClient;

class VideoStreamer {
 public:
  // VideoStreamer needs to send synchronization packets to the astrm port,
  // hence the aud_dst parameter.
  VideoStreamer(const std::string& vid_dst, const std::string& aud_dst);
  virtual ~VideoStreamer();

  bool Start();
  void Stop();

  // Needs YUV420P at the right size (kScreenWidth x kScreenHeight).
  void PushFrame(std::vector<byte>& frame);

  // Require an IDR to be sent next.
  void ResyncStream();

 private:
  std::unique_ptr<UdpClient> astrm_client_;
  std::unique_ptr<UdpClient> vstrm_client_;

  std::unique_ptr<H264Encoder> encoder_;
  std::vector<byte> frame_;
};

}  // namespace drc
