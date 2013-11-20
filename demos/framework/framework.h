#pragma once

#include <drc/types.h>
#include <vector>

namespace drc {
class InputReceiver;
class Streamer;
}

namespace demo {

enum DemoMode {
  kStreamerGLDemo,
  kStreamerSDLDemo,
  kReceiverDemo,
};

void Init(const char* name, DemoMode mode);
void Quit();

drc::InputReceiver* GetInputReceiver();
drc::Streamer* GetStreamer();

bool HandleEvents();

std::vector<drc::u8> TryReadbackFromGL();
void TryPushingGLFrame();

void SwapBuffers(int fps_limit = 60);

}  // namespace demo
