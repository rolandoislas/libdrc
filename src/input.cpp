#include <drc/input.h>
#include <drc/internal/udp.h>
#include <functional>
#include <mutex>
#include <vector>

namespace drc {

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

  if (!server_->StartListening()) {
    return false;
  }
  return true;
}

void InputReceiver::Stop() {
  server_->StopListening();
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

  data.left_stick_x = (msg[7] << 8) | msg[6];
  data.left_stick_y = (msg[9] << 8) | msg[8];
  data.right_stick_x = (msg[11] << 8) | msg[10];
  data.right_stick_y = (msg[13] << 8) | msg[12];

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
