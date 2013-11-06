#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <drc/internal/h264-encoder.h>
#include <drc/internal/udp.h>
#include <drc/internal/video-streamer.h>
#include <drc/internal/vstrm-packet.h>
#include <mutex>
#include <poll.h>
#include <string>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace drc {

namespace {

// TODO: that's not very clean.
u32 get_timestamp() {
  static int fd = -1;
  if (fd == -1) {
    fd = open("/sys/class/net/wlan1/device/tsf", O_RDONLY);
  }
  u64 ts = 0;
  read(fd, &ts, sizeof (ts));
  lseek(fd, 0, SEEK_SET);
  return static_cast<u32>(ts);
}

void GenerateVstrmPackets(std::vector<VstrmPacket>* vstrm_packets,
                          const H264ChunkArray& chunks, u32 timestamp,
                          bool* vstrm_inited, u16* vstrm_seqid) {

  // Set the init flag on the first frame ever sent.
  bool init_flag = !*vstrm_inited;
  if (init_flag) {
    *vstrm_inited = true;
  }

  for (size_t i = 0; i < chunks.size(); ++i) {
    bool first_chunk = (i == 0);
    bool last_chunk = (i == chunks.size() - 1);

    const byte* chunk_data;
    size_t chunk_size;
    std::tie(chunk_data, chunk_size) = chunks[i];

    bool first_packet = true;
    do {
      const byte* pkt_data = chunk_data;
      size_t pkt_size = std::min(kMaxVstrmPayloadSize, chunk_size);

      chunk_data += pkt_size;
      chunk_size -= pkt_size;

      bool last_packet = (chunk_size == 0);

      vstrm_packets->resize(vstrm_packets->size() + 1);
      VstrmPacket* pkt = &vstrm_packets->at(vstrm_packets->size() - 1);

      u16 seqid = (*vstrm_seqid)++;
      if (*vstrm_seqid >= 1024) {
        *vstrm_seqid = 0;
      }
      pkt->SetSeqId(seqid);

      pkt->SetPayload(pkt_data, pkt_size);
      pkt->SetTimestamp(timestamp);

      pkt->SetInitFlag(init_flag);
      pkt->SetFrameBeginFlag(first_chunk && first_packet);
      pkt->SetChunkEndFlag(last_packet);
      pkt->SetFrameEndFlag(last_chunk && last_packet);

      pkt->SetIdrFlag(true); // TODO
      pkt->SetFrameRate(VstrmFrameRate::k59_94Hz); // TODO

      first_packet = false;
    } while (chunk_size >= kMaxVstrmPayloadSize);
  }
}

}  // namespace

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
  pollfd events[] = {
    { stop_event_fd_, POLLIN, 0 },
    // TODO: enable when resyncs are actually being handled
    //{ resync_event_fd_, POLLIN, 0 },
  };
  size_t nfds = sizeof (events) / sizeof (events[0]);

  std::vector<byte> encoding_frame;
  timespec sleep_time = { 0, 0 };

  bool vstrm_inited = false;
  u16 vstrm_seqid = 0;
  std::vector<VstrmPacket> vstrm_packets;
  while (true) {
    if (ppoll(events, nfds, &sleep_time, NULL) == -1) {
      break;
    }

    bool stop_requested = false;
    for (size_t i = 0; i < nfds; ++i) {
      if (events[i].fd == stop_event_fd_ && events[i].revents) {
        stop_requested = true;
      }
    }
    if (stop_requested) {
      break;
    }

    for (const auto& pkt : vstrm_packets) {
      vstrm_client_->Send(pkt.GetBytes(), pkt.GetSize());
    }
    vstrm_packets.clear();

    LatchOnCurrentFrame(encoding_frame);
    if (encoding_frame.size() == 0) {
      continue;
    }

    const H264ChunkArray& chunks = encoder_->Encode(encoding_frame, true);
    GenerateVstrmPackets(&vstrm_packets, chunks, get_timestamp(),
                         &vstrm_inited, &vstrm_seqid);

    sleep_time.tv_nsec = 16666667;
  }
}

void VideoStreamer::LatchOnCurrentFrame(std::vector<byte>& latched_frame) {
  std::lock_guard<std::mutex> lk(frame_mutex_);
  if (frame_.size() != 0) {
    latched_frame = std::move(frame_);
  }
}

}  // namespace drc
