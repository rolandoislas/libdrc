#pragma once

#include <drc/types.h>
#include <mutex>

namespace drc {

struct InputData {
  InputData() {
    valid = false;
  }

  bool valid;

  enum Button {
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

    kBtnTV = 0x20000,
    kBtnR3 = 0x40000,
    kBtnL3 = 0x80000,
  } buttons;

  s16 left_stick_x;
  s16 left_stick_y;
  s16 right_stick_x;
  s16 right_stick_y;

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
  static const u16 kDefaultPort = 50022;

  InputReceiver(u16 recv_port = kDefaultPort);
  virtual ~InputReceiver();

  bool Start();
  void Stop();

  void Poll(InputData& data);

 private:
  InputData current_;
  std::mutex current_mutex_;
};

}  // namespace drc
