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
#include <drc/internal/cmd-protocol.h>
#include <drc/internal/input-receiver.h>
#include <drc/internal/udp.h>
#include <drc/internal/video-converter.h>
#include <drc/internal/video-streamer.h>
#include <drc/streamer.h>
#include <vector>
#include <string>

namespace drc {

namespace {

bool RunGenericCmd(CmdClient* cli, int service, int method,
                   const std::vector<byte>& msg, std::vector<byte>* repl) {
  GenericCmdPacket pkt;
  pkt.SetFlags(GenericCmdPacket::kQueryFlag);
  pkt.SetServiceId(service);
  pkt.SetMethodId(method);
  pkt.SetPayload(msg.data(), msg.size());

  return cli->Query(CmdQueryType::kGenericCommand, pkt.GetBytes(),
                      pkt.GetSize(), repl);
}

void RunGenericAsyncCmd(CmdClient* cli, int service, int method,
                   const std::vector<byte>& msg, CmdState::ReplyCallback cb) {
  GenericCmdPacket pkt;
  pkt.SetFlags(GenericCmdPacket::kQueryFlag);
  pkt.SetServiceId(service);
  pkt.SetMethodId(method);
  pkt.SetPayload(msg.data(), msg.size());

  if (cb == nullptr) {
    cb = [](bool succ, const std::vector<byte>& data) {
      // empty callback
    };
  }

  cli->AsyncQuery(CmdQueryType::kGenericCommand, pkt.GetBytes(),
                      pkt.GetSize(), cb);
}

}  // namespace

Streamer::Streamer(const std::string& vid_dst,
                   const std::string& aud_dst,
                   const std::string& cmd_dst,
                   const std::string& msg_bind,
                   const std::string& input_bind,
                   const std::string& cmd_bind)
    : msg_server_(new UdpServer(msg_bind)),
      aud_streamer_(new AudioStreamer(aud_dst)),
      cmd_client_(new CmdClient(cmd_dst, cmd_bind)),
      vid_converter_(new VideoConverter()),
      vid_streamer_(new VideoStreamer(vid_dst, aud_dst)),
      input_receiver_(new InputReceiver(input_bind)) {
}

Streamer::~Streamer() {
}

bool Streamer::Start() {
  msg_server_->SetReceiveCallback(
      [=](const std::vector<byte>& msg) {
        if (msg.size() == 4 && !memcmp(msg.data(), "\1\0\0\0", 4)) {
          vid_streamer_->ResyncStream();
        }
      });

  vid_converter_->SetDoneCallback(
      [=](std::vector<byte>& yuv_frame) {
        PushNativeVidFrame(yuv_frame);
      });

  if (!msg_server_->Start() ||
      !aud_streamer_->Start() ||
      !cmd_client_->Start() ||
      !vid_converter_->Start() ||
      !vid_streamer_->Start() ||
      !input_receiver_->Start()) {
    Stop();
    return false;
  }
  return true;
}

void Streamer::Stop() {
  msg_server_->Stop();
  aud_streamer_->Stop();
  cmd_client_->Stop();
  vid_converter_->Stop();
  vid_streamer_->Stop();
  input_receiver_->Stop();
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

void Streamer::PollInput(InputData& data) {
  input_receiver_->Poll(data);
}

bool Streamer::SetLcdBacklight(int level, bool wait) {
  std::vector<byte> payload;
  bool rv = true;
  payload.push_back((level + 1) & 0xFF);
  if (wait) {
    rv = RunGenericCmd(cmd_client_.get(), 5, 0x14, payload, nullptr);
  } else {
    RunGenericAsyncCmd(cmd_client_.get(), 5, 0x14, payload, nullptr);
  }
  return rv;
}

bool Streamer::GetUICConfig(std::vector<byte> *config,
                            CmdState::ReplyCallback cb) {
  bool rv = false;
  std::vector<byte> reply;
  std::vector<byte> payload;
  if (cb == nullptr) {
    rv = RunGenericCmd(cmd_client_.get(), 5, 0x06, payload, &reply);
    if (rv) {
      if (reply.size() == 0x310) {
        if (config) {
          config->assign(reply.data()+0x10, reply.data()+0x310);
          rv = true;
        }
      }
    }
  } else {
    RunGenericAsyncCmd(cmd_client_.get(), 5, 0x06, payload, cb);
    rv = true;
  }
  return rv;
}

}  // namespace drc
