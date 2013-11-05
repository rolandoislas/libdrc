#include <drc/internal/h264-encoder.h>
#include <drc/internal/udp.h>
#include <drc/internal/video-streamer.h>
#include <mutex>
#include <poll.h>
#include <string>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace drc {

VideoStreamer::VideoStreamer(const std::string& vid_dst,
                             const std::string& aud_dst)
    : astrm_client_(new UdpClient(aud_dst)),
      vstrm_client_(new UdpClient(vid_dst)),
      encoder_(new H264Encoder()),
      event_fd_(-1) {
}

VideoStreamer::~VideoStreamer() {
}

bool VideoStreamer::Start() {
  if (!astrm_client_->Start() || !vstrm_client_->Start()) {
    Stop();
    return false;
  }

  event_fd_ = eventfd(0, EFD_NONBLOCK);
  if (event_fd_ == -1) {
    Stop();
    return false;
  }

  streaming_thread_ = std::thread(&VideoStreamer::ThreadLoop, this);
  return true;
}

void VideoStreamer::Stop() {
  if (event_fd_ != -1) {
    u64 val = 1;
    write(event_fd_, &val, sizeof (val));
    streaming_thread_.join();

    close(event_fd_);
  }

  astrm_client_->Stop();
  vstrm_client_->Stop();
}

void VideoStreamer::PushFrame(std::vector<byte>& frame) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  frame_ = std::move(frame);
}

void VideoStreamer::ResyncStream() {
  // TODO: not used yet since we always send IDRs.
}

void VideoStreamer::ThreadLoop() {
  bool cont = true;
  pollfd event = { event_fd_, POLLIN, 0 };

  std::vector<byte> encoding_frame;
  timespec sleep_time = { 0, 0 };
  while (cont) {
    if (ppoll(&event, 1, &sleep_time, NULL) == -1) {
      cont = false;
      break;
    }

    if (event.revents & POLLIN) {
      cont = false;
      break;
    }

    // TODO: if we have vstrm/astrm messages to send, send them.

    // Encode next frame in advance.
    {
      std::lock_guard<std::mutex> lk(frame_mutex_);
      if (frame_.size() != 0) {
        encoding_frame = std::move(frame_);
      }
    }

    if (encoding_frame.size() == 0) {
      continue;
    }

    const H264ChunkArray& chunks = encoder_->Encode(encoding_frame);

    // TODO: craft astrm/vstrm packets.
    // TODO: recompute sleep_time.
  }
}

}  // namespace drc
