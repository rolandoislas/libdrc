#include <fcntl.h>
#include <drc/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

namespace drc {

namespace {

int GetInterfaceOfIpv4(const std::string& addr, std::string* out) {
  struct ifaddrs* ifa;
  struct ifaddrs* p;
  char ip_buffer[16];
  int rv = EAI_FAIL;

  if (getifaddrs(&ifa) == -1 || ifa == NULL) {
    return rv;
  }

  p = ifa;
  while (p) {
    if (p->ifa_addr != NULL) {
      if (p->ifa_addr->sa_family == AF_INET) {
        rv = getnameinfo(p->ifa_addr, sizeof(struct sockaddr_in), ip_buffer,
                         sizeof(ip_buffer), NULL, 0, NI_NUMERICHOST);
        if (rv == 0) {
          if (addr == ip_buffer) {
            break;
          }
        }
      }
    }
    p = p->ifa_next;
  }

  if (p) {
    *out = p->ifa_name;
  } else {
    rv = EAI_FAIL;
  }

  freeifaddrs(ifa);
  return rv;
}

}  // namespace

int GetTsf(u64 *tsf) {
  static int fd = -1;

  if (fd == -1) {
    std::string drc_if;

    if (GetInterfaceOfIpv4("192.168.1.10", &drc_if) == 0) {
      std::string tsf_path = std::string("/sys/class/net/") + drc_if +
                             "/device/tsf";
      fd = open(tsf_path.c_str(), O_RDONLY);
    }

    if (fd == -1) {
      return -1;
    }
  }
  pread(fd, tsf, sizeof(tsf), 0);
  return 0;
}

}  // namespace drc
