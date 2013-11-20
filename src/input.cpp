#include <drc/input.h>
#include <drc/internal/udp.h>
#include <functional>
#include <mutex>
#include <vector>

namespace drc {

namespace {

// Range of the directional sticks. This is kind of approximate but should
// still be accurate enough. Some sticks might report values lower than the
// minimum or higher than the maximum - just clamp in that case.
const s16 kDrcStickMin = 900;
const s16 kDrcStickMax = 3200;

// Dead zone: values that are < to the dead zone will be converted to 0 instead
// to avoid noise.
const float kStickDeadZone = 0.1;

}  // namespace

InputReceiver::InputReceiver(const std::string& hid_bind)
    : server_(new UdpServer(hid_bind)) {
}

InputReceiver::~InputReceiver() {
  Stop();
}

bool InputReceiver::Start() {
  // Packets are usually received at 180Hz. If nothing was received after 1/60s
  // (3 packets missed), timeout.
  server_->SetTimeout(1000000L / 60);

  server_->SetReceiveCallback(
      [=](const std::vector<byte>& msg) {
        return ProcessInputMessage(msg);
      });
  server_->SetTimeoutCallback(
      std::bind(&InputReceiver::ProcessInputTimeout, this));

  if (!server_->Start()) {
    return false;
  }
  return true;
}

void InputReceiver::Stop() {
  server_->Stop();
}

void InputReceiver::Poll(InputData& data) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  data = current_;
}

void InputReceiver::SetCurrent(const InputData& new_current) {
  std::lock_guard<std::mutex> lk(current_mutex_);
  current_ = new_current;
}

bool InputReceiver::ProcessInputMessage(const std::vector<byte>& msg) {
  // HID packets should always be 128 bytes.
  if (msg.size() != 128) {
    return false;
  }

  InputData data;

  int buttons = (msg[80] << 16) | (msg[2] << 8) | msg[3];
  data.buttons = static_cast<InputData::ButtonMask>(buttons);

  // Convert the integer stick values to floating point -1..1 range.
  float* sticks[] = { &data.left_stick_x, &data.left_stick_y,
                      &data.right_stick_x, &data.right_stick_y };
  for (size_t i = 0; i < sizeof (sticks) / sizeof (sticks[0]); ++i) {
    s16 val_int = (msg[7 + 2*i] << 8) | msg[6 + 2*i];

    // Clamp to min/max range.
    val_int = std::max(kDrcStickMin, std::min(kDrcStickMax, val_int));

    // Transform to -MID, +MID.
    s16 mid = (kDrcStickMax - kDrcStickMin) / 2;
    val_int -= kDrcStickMin + mid;

    // Divide by MID to move to -1.0, +1.0.
    *sticks[i] = static_cast<float>(val_int) / mid;

    // Apply the dead zone.
    if (*sticks[i] > -kStickDeadZone && *sticks[i] < kStickDeadZone) {
      *sticks[i] = 0.0;
    }
  }

  // Read touchscreen points and average.
  float ts_x = 0, ts_y = 0;
  const int kTsPointsCount = 10;
  for (int i = 0; i < kTsPointsCount; ++i) {
    int base = 36 + 4 * i;

    int x = ((msg[base + 1] & 0xF) << 8) | msg[base];
    int y = ((msg[base + 3] & 0xF) << 8) | msg[base + 2];

    ts_x += x / 4096.0;
    ts_y += y / 4096.0;
  }
  data.ts_x = ts_x / kTsPointsCount;
  data.ts_y = ts_y / kTsPointsCount;

  // Read touchscreen pressure intensity.
  int ts_pressure = 0;
  ts_pressure |= ((msg[37] >> 4) & 7) << 0;
  ts_pressure |= ((msg[39] >> 4) & 7) << 3;
  ts_pressure |= ((msg[41] >> 4) & 7) << 6;
  ts_pressure |= ((msg[43] >> 4) & 7) << 9;

  // TODO: make meaningful
  data.ts_pressure = ts_pressure;
  data.ts_pressed = (ts_pressure != 0);

  data.battery_charge = msg[5];
  data.audio_volume = msg[14];
  data.power_status = static_cast<InputData::PowerStatus>(msg[4]);

  data.valid = true;
  SetCurrent(data);
  return true;
}

bool InputReceiver::ProcessInputTimeout() {
  SetCurrent(InputData());
  return true;
}

}  // namespace drc
