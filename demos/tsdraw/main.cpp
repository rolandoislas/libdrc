#include "../framework/framework.h"

#include <drc/input.h>
#include <drc/screen.h>
#include <cmath>
#include <cstdlib>
#include <drc/streamer.h>
#include <SDL.h>

namespace {

void PutPixel(drc::u8* pixels, int x, int y, drc::u32 col) {
  drc::u32* ppix = (drc::u32*)(pixels + 4 * (y * drc::kScreenWidth + x));
  *ppix = col;
}

void DrawLine(drc::u8* pixels, int x0, int y0, int x1, int y1, drc::u32 col) {
  int dx = abs(x1 - x0), dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    PutPixel(pixels, x0, y0, col);
    if (x0 == x1 && y0 == y1) {
      break;
    }

    int err2 = 2 * err;
    if (err2 > -dy) {
      err -= dy;
      x0 += sx;
    }

    if (x0 == x1 && y0 == y1) {
      PutPixel(pixels, x0, y0, col);
      break;
    }

    if (err2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void RenderFrame(SDL_Surface* surface, const drc::InputData& input_data) {
  static bool has_previous_point = false;
  static int previous_point_x, previous_point_y;

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

    if (has_previous_point) {
      DrawLine(pixels, x, y, previous_point_x, previous_point_y, 0xFFFFFFFF);
    } else {
      PutPixel(pixels, x, y, 0xFFFFFFFF);
    }

    has_previous_point = true;
    previous_point_x = x;
    previous_point_y = y;

  } else {
    has_previous_point = false;
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
