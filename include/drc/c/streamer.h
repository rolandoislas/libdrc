#pragma once

/*
 * A very small C wrapper for libdrc in order to make it usable in C
 * applications. It only exposes a very small portion of the libdrc API. For
 * full functionality, please use the C++ library instead.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct drc_streamer;

enum drc_pixel_format {
  DRC_RGB,
  DRC_RGBA,
  DRC_BGR,
  DRC_BGRA,
  DRC_RGB565,
};

struct drc_streamer* drc_new_streamer();
void drc_delete_streamer(struct drc_streamer* self);
int drc_start_streamer(struct drc_streamer* self);
void drc_stop_streamer(struct drc_streamer* self);
void drc_push_vid_frame(struct drc_streamer* self, const unsigned char* buffer,
                        unsigned int size, unsigned short width,
                        unsigned short height, enum drc_pixel_format pixfmt);

#ifdef __cplusplus
}
#endif
