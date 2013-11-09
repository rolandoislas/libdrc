#include <drc/c/streamer.h>
#include <drc/streamer.h>

extern "C" {

struct drc_streamer {
  drc::Streamer streamer;
};

drc_streamer* drc_new_streamer() {
  return new drc_streamer();
}

void drc_delete_streamer(struct drc_streamer* self) {
  delete self;
}

int drc_start_streamer(struct drc_streamer* self) {
  return self->streamer.Start();
}

void drc_stop_streamer(struct drc_streamer* self) {
  self->streamer.Stop();
}

void drc_push_vid_frame(struct drc_streamer* self, const unsigned char* buffer,
                        unsigned int size, unsigned short width,
                        unsigned short height, enum drc_pixel_format pixfmt) {
  std::vector<drc::byte> frame(buffer, buffer + size);
  self->streamer.PushVidFrame(
      frame, width, height, static_cast<drc::PixelFormat>(pixfmt));
}

}
