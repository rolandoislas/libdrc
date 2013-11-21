#pragma once

#include <deque>
#include <drc/types.h>
#include <mutex>
#include <string>
#include <thread>

namespace drc {

class UdpClient;

class AudioStreamer {
 public:
  AudioStreamer(const std::string& dst);
  virtual ~AudioStreamer();

  bool Start();
  void Stop();

  // Expects to get 48KHz audio data.
  void PushSamples(const std::vector<s16>& samples);

 private:
  void ThreadLoop();
  void PopSamples(std::vector<s16>& samples, u32 count);

  std::unique_ptr<UdpClient> astrm_client_;

  std::mutex samples_mutex_;
  std::deque<s16> samples_;
  std::thread streaming_thread_;

  int stop_event_fd_;
};

}
