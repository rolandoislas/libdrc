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

#include <drc/pixel-format.h>
#include <drc/types.h>
#include <memory>
#include <string>
#include <vector>

namespace drc {

class AudioStreamer;
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
  enum FlippingMode {
    NoFlip,
    FlipVertically
  };
  void PushVidFrame(std::vector<byte>& frame, u16 width, u16 height,
                    PixelFormat pixfmt, FlippingMode flip = NoFlip);

  // Same as PushVidFrame, but the frame needs to already be in the native
  // format for encoding: YUV420P at ScreenWidth x ScreenHeight.
  //
  // Faster: PushVidFrame requires pixel format conversion before encoding.
  void PushNativeVidFrame(std::vector<u8>& frame);

  // Expects 48KHz samples.
  void PushAudSamples(const std::vector<s16>& samples);

 private:
  std::unique_ptr<UdpServer> msg_server_;

  std::unique_ptr<AudioStreamer> aud_streamer_;
  std::unique_ptr<VideoConverter> vid_converter_;
  std::unique_ptr<VideoStreamer> vid_streamer_;
};

}  // namespace drc
