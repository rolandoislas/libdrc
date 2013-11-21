#pragma once

#include <cstring>
#include <drc/types.h>
#include <memory>
#include <mutex>
#include <vector>

namespace drc {

class UdpServer;

struct InputData {
  InputData() {
    // Warning: this will break horribly if InputData stops being a POD at some
    // point in the future.
    memset(this, 0, sizeof (*this));
  }

  bool valid;

  enum ButtonMask {
    kBtnSync = 0x1,
    kBtnHome = 0x2,
    kBtnMinus = 0x4,
    kBtnPlus = 0x8,
    kBtnR = 0x10,
    kBtnL = 0x20,
    kBtnZR = 0x40,
    kBtnZL = 0x80,
    kBtnDown = 0x100,
    kBtnUp = 0x200,
    kBtnRight = 0x400,
    kBtnLeft = 0x800,
    kBtnY = 0x1000,
    kBtnX = 0x2000,
    kBtnB = 0x4000,
    kBtnA = 0x8000,

    kBtnTV = 0x200000,
    kBtnR3 = 0x400000,
    kBtnL3 = 0x800000,
  } buttons;

  // Range: -1.0 to 1.0.
  float left_stick_x;
  float left_stick_y;
  float right_stick_x;
  float right_stick_y;

  // Touchscreen data. Range: 0.0 to 1.0.
  bool ts_pressed;
  float ts_x;
  float ts_y;
  float ts_pressure;

  u8 battery_charge;
  u8 audio_volume;

  enum PowerStatus {
    kPowerAC = 0x01,
    kPowerButtonPressed = 0x02,
    kPowerCharging = 0x40,
    kPowerUSB = 0x80,
  } power_status;
};

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
