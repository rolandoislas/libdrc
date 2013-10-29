#include <drc/input.h>
#include <mutex>

namespace drc {

InputReceiver::InputReceiver(u16 recv_port) {
  // TODO
}

InputReceiver::~InputReceiver() {
}

bool InputReceiver::Start() {
  return true;
}

void InputReceiver::Stop() {
}

void InputReceiver::Poll(InputData& data) {
  std::lock_guard<std::mutex> lk(current_mutex_);

  data = current_;
}

}  // namespace drc
