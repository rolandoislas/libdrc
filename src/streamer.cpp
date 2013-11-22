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

#include <cstring>
#include <drc/internal/audio-streamer.h>
#include <drc/internal/udp.h>
#include <drc/internal/video-converter.h>
#include <drc/internal/video-streamer.h>
#include <drc/streamer.h>
#include <vector>

namespace drc {

Streamer::Streamer(const std::string& vid_dst,
                   const std::string& aud_dst,
                   const std::string& msg_bind) 
    : msg_server_(new UdpServer(msg_bind)),
      aud_streamer_(new AudioStreamer(aud_dst)),
      vid_converter_(new VideoConverter()),
      vid_streamer_(new VideoStreamer(vid_dst, aud_dst)) {
}

Streamer::~Streamer() {
}

bool Streamer::Start() {
  msg_server_->SetReceiveCallback(
      [=](const std::vector<byte>& msg) {
        if (msg.size() == 4 && !memcmp(msg.data(), "\1\0\0\0", 4)) {
          vid_streamer_->ResyncStream();
        }
        return true;
      });

  vid_converter_->SetDoneCallback(
      [=](std::vector<byte>& yuv_frame) {
        PushNativeVidFrame(yuv_frame);
      });

  if (!msg_server_->Start() ||
      !aud_streamer_->Start() ||
      !vid_converter_->Start() ||
      !vid_streamer_->Start()) {
    Stop();
    return false;
  }
  return true;
}

void Streamer::Stop() {
  msg_server_->Stop();
  aud_streamer_->Stop();
  vid_converter_->Stop();
  vid_streamer_->Stop();
}

void Streamer::PushVidFrame(std::vector<byte>& frame, u16 width, u16 height,
                            PixelFormat pixfmt, FlippingMode flip) {
  bool do_flip = (flip == FlipVertically);
  vid_converter_->PushFrame(frame,
                            std::make_tuple(width, height, pixfmt, do_flip));
}

void Streamer::PushNativeVidFrame(std::vector<byte>& frame) {
  vid_streamer_->PushFrame(frame);
}

void Streamer::PushAudSamples(const std::vector<s16>& samples) {
  aud_streamer_->PushSamples(samples);
}

}  // namespace drc
