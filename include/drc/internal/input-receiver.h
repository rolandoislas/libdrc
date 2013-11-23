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

#include <drc/input.h>
#include <drc/types.h>
#include <memory>
#include <mutex>
#include <vector>

namespace drc {

class UdpServer;

class InputReceiver {
 public:
  static constexpr const char* kDefaultBind = "192.168.1.10:50022";

  InputReceiver(const std::string& hid_bind = kDefaultBind);
  virtual ~InputReceiver();

  bool Start();
  void Stop();

  void Poll(InputData& data);

  void CalibrateWithPoints(s32 raw_1_x, s32 raw_1_y, s32 raw_2_x, s32 raw_2_y,
                           s32 ref_1_x, s32 ref_1_y, s32 ref_2_x, s32 ref_2_y);

 private:
  void SetCurrent(const InputData& new_current);

  bool ProcessInputMessage(const std::vector<byte>& data);
  bool ProcessInputTimeout();

  std::unique_ptr<UdpServer> server_;

  InputData current_;
  std::mutex current_mutex_;

  // Touchscreen calibration parameters.
  float ts_ox_, ts_oy_, ts_w_, ts_h_;
};

}  // namespace drc
