#include "../framework/framework.h"

#include <cmath>
#include <drc/input.h>
#include <drc/streamer.h>

int main() {
  std::vector<drc::s16> samples(48000 * 2);
  for (int i = 0; i < 48000; ++i) {
    float t = i / (48000.0 - 1);
    drc::s16 samp = (drc::s16)(32000 * sinf(2 * M_PI * t * 480));
    samples[2 * i] = samp;
    samples[2 * i + 1] = samp;
  }
  demo::Init("simpleaudio", demo::kStreamerGLDemo);

  drc::InputData input_data;
  bool was_pressed = false;
  while (demo::HandleEvents()) {
    demo::TryPushingGLFrame();

    demo::GetInputReceiver()->Poll(input_data);
    if (input_data.valid && input_data.buttons & drc::InputData::kBtnA) {
      if (!was_pressed) {
        demo::GetStreamer()->PushAudSamples(samples);
        was_pressed = true;
      }
    } else {
      was_pressed = false;
    }

    demo::SwapBuffers();
  }

  demo::Quit();
}
