#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

#include <drc/types.h>

#ifdef GHETTO_TEST
#include <cassert>
#include <cstdio>
#endif  // GHETTO_TEST

namespace drc {

namespace  {
int get_interface_of_ipv4(const std::string &addr, std::string *out) {
  struct ifaddrs *ifa, *p;
  char ip_buffer[16];
  int rv = EAI_FAIL;

  if (getifaddrs(&ifa) == -1) return rv;
  if (ifa == NULL) return rv;

  p = ifa;
  while (p) {
    if (p->ifa_addr != NULL) {
      if (p->ifa_addr->sa_family == AF_INET) {
        rv = getnameinfo(p->ifa_addr, sizeof(struct sockaddr_in), ip_buffer,
                         sizeof(ip_buffer), NULL, 0, NI_NUMERICHOST);
        if (rv == 0) {
          if (addr == ip_buffer)
            break;
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


int get_tsf(u64 *tsf) {
  static int fd = -1;
  if (fd == -1) {
    std::string drc_if;
    if (get_interface_of_ipv4("192.168.1.10", &drc_if) == 0) {
      std::string tsf_path = std::string("/sys/class/net/") + drc_if + "/tsf";
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



#ifdef GHETTO_TEST

int main() {
  std::string iface;
  int rv;
  drc::u64 tsf;

  rv = drc::get_interface_of_ipv4("127.0.0.1", &iface);
  assert(rv == 0);
  assert(iface == "lo");

  rv = drc::get_interface_of_ipv4("0.0.0.0", &iface);
  assert(rv != 0);

  rv = drc::get_interface_of_ipv4("192.168.1.10", &iface);
  if (rv != 0) {
    std::printf("could not find local ip 192.168.1.10; is this machine "
                "connected to drc?\n");
  } else {
    rv = drc::get_tsf(&tsf);
    if (rv != 0) {
      std::printf("could not get tsf; is the kernel patch installed?\n");
    }
  }
  return 0;
}


#endif  // GHETTO_TEST

