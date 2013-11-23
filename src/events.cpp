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

#include <array>
#include <cassert>
#include <drc/internal/events.h>
#include <drc/types.h>
#include <fcntl.h>
#include <functional>
#include <map>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <vector>

namespace drc {

void TriggerableEvent::Trigger() {
  u64 val = 1;
  write(fd, &val, sizeof (val));
}

EventMachine::EventMachine() {
  epoll_fd_ = epoll_create(64);
  stop_evt_ = NewTriggerableEvent([&](Event*) {
    running_ = false;
    return true;
  });
}

EventMachine::~EventMachine() {
  close(epoll_fd_);
}

TriggerableEvent* EventMachine::NewTriggerableEvent(Event::CallbackType cb) {
  Event* evt = NewEvent(eventfd(0, EFD_NONBLOCK), sizeof (u64), false, cb);

  // XXX: Hacky.
  return reinterpret_cast<TriggerableEvent*>(evt);
}

Event* EventMachine::NewTimerEvent(u64 nanoseconds, Event::CallbackType cb) {
  struct itimerspec its = {
    { 0, 0 },
    { 0, (s64)nanoseconds }
  };

  int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  timerfd_settime(fd, 0, &its, NULL);
  return NewEvent(fd, sizeof (u64), true, cb);
}

Event* EventMachine::NewRepeatedTimerEvent(u64 nanoseconds,
                                           Event::CallbackType cb) {
  struct itimerspec its = {
    { 0, (s64)nanoseconds },
    { 0, 1 }
  };

  int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  timerfd_settime(fd, 0, &its, NULL);
  return NewEvent(fd, sizeof (u64), false, cb);
}

Event* EventMachine::NewSocketEvent(int fd, Event::CallbackType cb) {
  return NewEvent(fd, 0, false, cb);
}

void EventMachine::Start() {
  std::array<struct epoll_event, 64> triggered;

  running_ = true;
  while (running_) {
    int nfds = epoll_wait(epoll_fd_, triggered.data(), triggered.size(), -1);
    assert(nfds != -1);

    for (int i = 0; i < nfds; ++i) {
      auto it = events_.find(triggered[i].data.fd);
      assert(it != events_.end());

      Event* evt = it->second.get();
      bool keep = evt->callback(evt);
      if (evt->oneshot || !keep) {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, triggered[i].data.fd,
                  &triggered[i]);
        close(evt->fd);
        events_.erase(it);
      } else if (evt->read_size) {
        std::vector<byte> buffer(evt->read_size);
        read(evt->fd, buffer.data(), evt->read_size);
      }
    }
  }
}

Event* EventMachine::NewEvent(int fd, int read_size, bool oneshot,
                              Event::CallbackType cb) {
  assert(fd >= 0);

  Event* evt = new Event();
  evt->fd = fd;
  evt->read_size = read_size;
  evt->oneshot = oneshot;
  evt->callback = cb;

  struct epoll_event epoll_evt;
  epoll_evt.events = EPOLLIN;
  epoll_evt.data.fd = fd;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &epoll_evt);

  events_[fd].reset(evt);
  return evt;
}

}  // namespace drc
