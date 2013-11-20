#include "../framework/framework.h"

#include <drc/input.h>
#include <drc/screen.h>
#include <drc/streamer.h>
#include <SDL.h>

namespace {

void RenderFrame(SDL_Surface* surface, const drc::InputData& input_data) {
  drc::u8* pixels = static_cast<drc::u8*>(surface->pixels);

  if (!input_data.valid) {
    return;
  }

  if (input_data.buttons & drc::InputData::kBtnA) {
    memset(pixels, 0, 4 * drc::kScreenWidth * drc::kScreenHeight);
  } else if (input_data.ts_pressed) {
    int x = static_cast<int>(input_data.ts_x * drc::kScreenWidth);
    int y = static_cast<int>(input_data.ts_y * drc::kScreenHeight);
    y = drc::kScreenHeight - y;

    drc::u32* ppix =
        (drc::u32*)(pixels + 4 * (y * drc::kScreenWidth + x));
    *ppix = 0xFFFFFFFF;
  }
}

}

int main(int argc, char** argv) {
  demo::Init("tsdraw", demo::kStreamerSDLDemo);

  drc::InputData input_data;
  SDL_Surface* surface = SDL_GetVideoSurface();
  while (demo::HandleEvents()) {
    demo::GetInputReceiver()->Poll(input_data);

    SDL_LockSurface(surface);
    std::vector<drc::u8> pixels(
        (drc::u8*)surface->pixels,
        (drc::u8*)surface->pixels + drc::kScreenWidth * drc::kScreenHeight * 4);
    demo::GetStreamer()->PushVidFrame(pixels, drc::kScreenWidth,
                                      drc::kScreenHeight,
                                      drc::PixelFormat::kRGBA);
    RenderFrame(surface, input_data);
    SDL_UnlockSurface(surface);

    demo::SwapBuffers(180);
  }

  demo::Quit();
  return 0;
}
