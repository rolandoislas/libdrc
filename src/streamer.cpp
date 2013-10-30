#include <drc/streamer.h>
#include <vector>

namespace drc {

Streamer::Streamer(const std::string& vid_dst,
                   const std::string& aud_dst,
                   const std::string& msg_bind) {
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
