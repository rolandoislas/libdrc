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

UdpServer::UdpServer(const std::string& bind_addr)
    : sock_fd_(-1),
      event_fd_(-1),
      timeout_us_(0),
      bind_addr_(bind_addr) {
}

UdpServer::~UdpServer() {
  if (event_fd_ != -1) {
    StopListening();
  }
}

bool UdpServer::StartListening() {
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

void UdpServer::StopListening() {
  if (event_fd_ == -1) {
    return;
  }

  u64 val = 1;
  write(event_fd_, &val, sizeof (val));

  thread_.join();
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
