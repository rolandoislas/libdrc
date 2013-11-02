#pragma once

#include <drc/types.h>
#include <memory>
#include <string>

namespace drc {

class UdpClient;

class VideoStreamer {
 public:
  VideoStreamer(const std::string& dst);
  virtual ~VideoStreamer();

  bool Start();
  void Stop();

  // Needs YUV420P at the right size (kScreenWidth x kScreenHeight).
  void PushFrame(std::vector<byte>& frame);

  // Require an IDR to be sent next.
  void ResyncStream();

 private:
  std::unique_ptr<UdpClient> udp_client_;
};

}  // namespace drc
