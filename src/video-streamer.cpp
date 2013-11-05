#include <cstdio>
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
      stop_event_fd_(-1),
      resync_event_fd_(-1) {
}

VideoStreamer::~VideoStreamer() {
}

bool VideoStreamer::Start() {
  if (!astrm_client_->Start() || !vstrm_client_->Start()) {
    Stop();
    return false;
  }

  stop_event_fd_ = eventfd(0, EFD_NONBLOCK);
  resync_event_fd_ = eventfd(0, EFD_NONBLOCK);
  if (stop_event_fd_ == -1 || resync_event_fd_ == -1) {
    Stop();
    return false;
  }

  streaming_thread_ = std::thread(&VideoStreamer::ThreadLoop, this);
  return true;
}

void VideoStreamer::Stop() {
  if (stop_event_fd_ != -1) {
    u64 val = 1;
    write(stop_event_fd_, &val, sizeof (val));
    streaming_thread_.join();

    close(stop_event_fd_);
  }

  if (resync_event_fd_ != -1) {
    close(resync_event_fd_);
  }

  astrm_client_->Stop();
  vstrm_client_->Stop();
}

void VideoStreamer::PushFrame(std::vector<byte>& frame) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  frame_ = std::move(frame);
}

void VideoStreamer::ResyncStream() {
  u64 val = 1;
  write(resync_event_fd_, &val, sizeof (val));
}

void VideoStreamer::ThreadLoop() {
  bool cont = true;
  pollfd events[] = {
    { stop_event_fd_, POLLIN, 0 },
    // TODO: enable when resyncs are actually being handled
    //{ resync_event_fd_, POLLIN, 0 },
  };
  size_t nfds = sizeof (events) / sizeof (events[0]);

  std::vector<byte> encoding_frame;
  timespec sleep_time = { 0, 0 };
  while (cont) {
    if (ppoll(events, nfds, &sleep_time, NULL) == -1) {
      cont = false;
    }

    for (size_t i = 0; i < nfds; ++i) {
      if (events[i].fd == stop_event_fd_ && events[i].revents) {
        cont = false;
      }
    }

    // If an error occurred or we were asked to stop, no need to continue
    // sending and/or encoding this frame.
    if (cont == false) {
      break;
    }

    LatchOnCurrentFrame(encoding_frame);
    if (encoding_frame.size() == 0) {
      continue;
    }

    const H264ChunkArray& chunks = encoder_->Encode(encoding_frame, true);
    size_t total_size = 0;
    for (auto& ch : chunks) { total_size += std::get<1>(ch); }
    printf("Frame size: %zd\n", total_size);
  }
}

void VideoStreamer::LatchOnCurrentFrame(std::vector<byte>& latched_frame) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  if (frame_.size() != 0) {
    latched_frame = std::move(frame_);
  }
}

}  // namespace drc
