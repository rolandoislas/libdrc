#pragma once

#include <array>
#include <drc/types.h>
#include <vector>
#include <tuple>

extern "C" {
#include <x264.h>
}

namespace drc {

// A chunk is defined as a pointer to encoded data and the size of that data.
// There are 5 chunks in a frame.
const int kH264ChunksPerFrame = 5;
typedef std::tuple<const byte*, size_t> H264Chunk;
typedef std::array<H264Chunk, kH264ChunksPerFrame> H264ChunkArray;

class H264Encoder {
 public:
  H264Encoder();
  virtual ~H264Encoder();

  // The returned chunk array is only valid until the next call to Encode.
  const H264ChunkArray& Encode(const std::vector<byte>& frame);

 private:
  void ProcessNalUnit(x264_nal_t* nal);
  static void ProcessNalUnitTrampoline(x264_t* h, x264_nal_t* nal, void* arg);

  void CreateEncoder();

  x264_t* encoder_;
  H264ChunkArray chunks_;
  int num_chunks_encoded_;
  bool curr_frame_idr_;
};


}
