#pragma once

#include <drc/types.h>
#include <functional>
#include <string>
#include <thread>

namespace drc {

class UdpServer {
 public:
  typedef std::function<bool(const std::vector<byte>&)> ReceiveCallback;
  typedef std::function<bool(void)> TimeoutCallback;

  UdpServer(const std::string& bind_addr);
  virtual ~UdpServer();

  bool StartListening();
  void StopListening();

  void SetTimeout(u64 us) { timeout_us_ = us; }

  void SetReceiveCallback(ReceiveCallback cb) { recv_cb_ = cb; }
  void SetTimeoutCallback(TimeoutCallback cb) { timeout_cb_ = cb; }

 private:
  void CloseSockets();
  void ThreadLoop();

  int sock_fd_;
  int event_fd_;
  u64 timeout_us_;

  std::string bind_addr_;

  std::thread thread_;

  ReceiveCallback recv_cb_;
  TimeoutCallback timeout_cb_;
};

}  // namespace drc
