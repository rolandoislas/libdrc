// Copyright (c) 2013, Mema Hacking, All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <drc/internal/events.h>
#include <drc/types.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace drc {

class H264Encoder;
class UdpClient;

class VideoStreamer : public ThreadedEventMachine {
 public:
  // VideoStreamer needs to send synchronization packets to the astrm port,
  // hence the aud_dst parameter.
  VideoStreamer(const std::string& vid_dst, const std::string& aud_dst);
  virtual ~VideoStreamer();

  bool Start();
  void Stop();

  // Needs YUV420P at the right size (kScreenWidth x kScreenHeight).
  void PushFrame(std::vector<byte>* frame);

  // Require an IDR to be sent next.
  void ResyncStream();

 protected:
  virtual void InitEventsAndRun();

 private:
  void LatchOnCurrentFrame(std::vector<byte>* latched_frame);

  std::unique_ptr<UdpClient> astrm_client_;
  std::unique_ptr<UdpClient> vstrm_client_;

  std::unique_ptr<H264Encoder> encoder_;

  std::mutex frame_mutex_;
  std::vector<byte> frame_;

  TriggerableEvent* resync_evt_;
};

}  // namespace drc
