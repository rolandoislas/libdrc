// Copyright (c) 2013, Mema Hacking, All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <arpa/inet.h>
#include <cstring>
#include <drc/internal/udp.h>
#include <drc/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

namespace drc {

namespace {

// Pre-swaps the port to convert it to network format.
bool ParseBindAddress(const std::string& bind_addr, in_addr* addr, u16* port) {
  size_t pos = bind_addr.find_last_of(':');
  if (pos == std::string::npos) {
    return false;
  }

  std::string ip_str = bind_addr.substr(0, pos);
  if (!inet_aton(ip_str.c_str(), addr)) {
    return false;
  }

  std::string port_str = bind_addr.substr(pos + 1);
  *port = htons(static_cast<u16>(strtol(port_str.c_str(), NULL, 10)));
  return true;
}

}  // namespace

UdpClient::UdpClient(const std::string& dst_addr)
    : sock_fd_(-1),
      dst_addr_(dst_addr) {
}

UdpClient::~UdpClient() {
  Stop();
}

bool UdpClient::Start() {
  memset(&dst_addr_parsed_, 0, sizeof (dst_addr_parsed_));
  dst_addr_parsed_.sin_family = AF_INET;
  if (!ParseBindAddress(dst_addr_, &dst_addr_parsed_.sin_addr,
                        &dst_addr_parsed_.sin_port)) {
    return false;
  }

  sock_fd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock_fd_ == -1) {
    return false;
  }

  return true;
}

void UdpClient::Stop() {
  if (sock_fd_ != -1) {
    close(sock_fd_);
  }
}

bool UdpClient::Send(const byte* data, size_t size) {
  if (sendto(sock_fd_, data, size, 0, (sockaddr*)&dst_addr_parsed_,
             sizeof (dst_addr_parsed_)) < 0) {
    return false;
  }
  return true;
}

UdpServer::UdpServer(const std::string& bind_addr)
    : sock_fd_(-1),
      event_fd_(-1),
      timeout_us_(0),
      bind_addr_(bind_addr) {
}

UdpServer::~UdpServer() {
  Stop();
}

bool UdpServer::Start() {
  sockaddr_in sin;
  memset(&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  if (!ParseBindAddress(bind_addr_, &sin.sin_addr, &sin.sin_port)) {
    return false;
  }

  sock_fd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock_fd_ == -1) {
    return false;
  }

  fcntl(sock_fd_, F_SETFL, O_NONBLOCK);
  if (bind(sock_fd_, (sockaddr*)&sin, sizeof (sin)) == -1) {
    CloseSockets();
    return false;
  }

  event_fd_ = eventfd(0, EFD_NONBLOCK);
  if (event_fd_ == -1) {
    CloseSockets();
    return false;
  }

  thread_ = std::thread(&UdpServer::ThreadLoop, this);
  return true;
}

void UdpServer::Stop() {
  if (event_fd_ == -1) {
    return;
  }

  u64 val = 1;
  write(event_fd_, &val, sizeof (val));

  thread_.join();
  CloseSockets();
}

void UdpServer::CloseSockets() {
  if (sock_fd_ != -1) {
    close(sock_fd_);
    sock_fd_ = -1;
  }

  if (event_fd_ != -1) {
    close(event_fd_);
    event_fd_ = -1;
  }
}

void UdpServer::ThreadLoop() {
  bool cont = true;

  pollfd events[] = {
    { sock_fd_, POLLIN, 0 },
    { event_fd_, POLLIN, 0 },
  };
  size_t nfds = sizeof (events) / sizeof (events[0]);
  timespec timeout = { static_cast<time_t>(timeout_us_ / 1000000),
                       static_cast<time_t>((timeout_us_ % 1000000) * 1000) };
  timespec* ptimeout = (timeout_us_ == 0) ? NULL : &timeout;

  while (cont) {
    if (ppoll(events, nfds, ptimeout, NULL) == -1) {
      cont = false;
      break;
    }

    bool timed_out = true;
    for (size_t i = 0; i < nfds; ++i) {
      if (events[i].revents) {
        timed_out = false;
      }

      if (events[i].revents & POLLIN) {
        if (events[i].fd == event_fd_) {
          cont = false;
          break;
        } else if (events[i].fd == sock_fd_) {
          sockaddr_in sender;
          socklen_t sender_len = sizeof (sender);
          int msg_max_size = 2000;
          std::vector<byte> msg(msg_max_size);

          int size = recvfrom(sock_fd_, msg.data(), msg_max_size, 0,
                              (sockaddr*)&sender, &sender_len);
          if (size < 0) {
            continue;
          }
          msg.resize(size);

          cont = recv_cb_(msg);
        }
      }
    }

    if (timed_out) {
      cont = timeout_cb_();
    }
  }

  CloseSockets();
}

}  // namespace drc
