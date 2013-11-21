#include <deque>
#include <drc/internal/astrm-packet.h>
#include <drc/internal/audio-streamer.h>
#include <drc/internal/tsf.h>
#include <drc/internal/udp.h>
#include <drc/types.h>
#include <fcntl.h>
#include <mutex>
#include <poll.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>

namespace drc {

namespace {

const u32 kSamplesPerSecond = 48000;

// Chosen to be the best compromise between packet size (must be < 1600 bytes)
// and number of packets sent per second, while keeping a round number of
// milliseconds between 2 packets.
//
// 384 samples == one 1536b packet every 8ms
const u32 kSamplesPerPacket = 384;
const u32 kPacketIntervalMs = 1000 / (kSamplesPerSecond / kSamplesPerPacket);

s32 GetTimestamp() {
  u64 tsf;
  GetTsf(&tsf);
  return static_cast<s32>(tsf & 0xFFFFFFFF);
}

}  // namespace

AudioStreamer::AudioStreamer(const std::string& dst)
    : astrm_client_(new UdpClient(dst)),
      stop_event_fd_(-1) {
}

AudioStreamer::~AudioStreamer() {
  Stop();
}

bool AudioStreamer::Start() {
  if (!astrm_client_->Start()) {
    Stop();
    return false;
  }

  stop_event_fd_ = eventfd(0, EFD_NONBLOCK);
  if (stop_event_fd_ == -1) {
    Stop();
    return false;
  }

  streaming_thread_ = std::thread(&AudioStreamer::ThreadLoop, this);
  return true;
}

void AudioStreamer::Stop() {
  if (stop_event_fd_ != -1) {
    u64 val = 1;
    write(stop_event_fd_, &val, sizeof (val));
    streaming_thread_.join();

    close(stop_event_fd_);
    stop_event_fd_ = -1;
  }

  astrm_client_->Stop();
}

void AudioStreamer::PushSamples(const std::vector<s16>& samples) {
  std::lock_guard<std::mutex> lk(samples_mutex_);
  if (samples_.size() < kSamplesPerSecond * 32) {  // limit buffer size
    samples_.insert(samples_.end(), samples.begin(), samples.end());
  }
}

void AudioStreamer::PopSamples(std::vector<s16>& samples, u32 count) {
  std::lock_guard<std::mutex> lk(samples_mutex_);
  if (samples_.size() < count * 2) {
    samples.resize(0);
    samples.resize(count);
  } else {
    samples.insert(samples.end(), samples_.begin(), samples_.begin() + count);
    samples_.erase(samples_.begin(), samples_.begin() + count);
  }
}

void AudioStreamer::ThreadLoop() {
  int timer_event_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  itimerspec timer_spec = {
    { 0, kPacketIntervalMs * 1000000 },
    { 0, 1 }
  };
  timerfd_settime(timer_event_fd, 0, &timer_spec, NULL);

  pollfd events[] = {
    { stop_event_fd_, POLLIN, 0 },
    { timer_event_fd, POLLIN, 0 },
  };
  size_t nfds = sizeof (events) / sizeof (events[0]);

  u16 seqid = 0;
  while (true) {
    if (ppoll(events, 2, NULL, NULL) == -1) {
      break;
    }

    bool stop_requested = false;
    for (size_t i = 0; i < nfds; ++i) {
      if (events[i].fd == stop_event_fd_ && events[i].revents) {
        stop_requested = true;
      } else if (events[i].fd == timer_event_fd && events[i].revents) {
        u64 val;
        read(timer_event_fd, &val, sizeof (val));
      }
    }
    if (stop_requested) {
      break;
    }

    std::vector<s16> samples;
    PopSamples(samples, kSamplesPerPacket * 2);

    AstrmPacket pkt;
    pkt.SetPacketType(AstrmPacketType::kAudioData);
    pkt.SetFormat(AstrmFormat::kPcm48KHz);
    pkt.SetMonoFlag(false);
    pkt.SetVibrateFlag(false);
    pkt.SetSeqId(seqid);
    pkt.SetTimestamp(GetTimestamp());
    pkt.SetPayload((byte*)samples.data(), samples.size() * 2);  // TODO: LE/BE

    astrm_client_->Send(pkt.GetBytes(), pkt.GetSize());
    seqid = (seqid + 1) & 0x3FF;
  }

  close(timer_event_fd);
}

}  // namespace drc
