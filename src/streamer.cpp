#include <drc/streamer.h>
#include <vector>

namespace drc {

Streamer::Streamer(u16 vid_port, u16 aud_port, u16 msg_port) {
  // TODO
}

Streamer::~Streamer() {
}

bool Streamer::Start() {
  return true;
}

void Streamer::Stop() {
}

void Streamer::PushFrame(std::vector<u8>& frame, u16 width, u16 height,
                         PixelFormat pixfmt) {
}

}  // namespace drc
