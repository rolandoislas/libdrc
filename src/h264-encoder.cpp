// TODO: this file needs to be GPLv2+ because of x264 licensing (boo, hope
// openh264 will work for our use case).

#include <cassert>
#include <cstdio>
#include <drc/internal/h264-encoder.h>
#include <drc/screen.h>
#include <drc/types.h>
#include <vector>
#include <tuple>

extern "C" {
#include <x264.h>
}

namespace drc {

namespace {
const char* const kEncoderQuality = "slow";

void log_encoder_messages(void* data, int i_level, const char* format,
                          va_list va) {
  vfprintf(stderr, format, va);
}
}  // namespace

H264Encoder::H264Encoder() {
  CreateEncoder();
}

H264Encoder::~H264Encoder() {
}

void H264Encoder::CreateEncoder() {
  x264_param_t param;

  x264_param_default_preset(&param, kEncoderQuality, "zerolatency");
  param.i_width = kScreenWidth;
  param.i_height = kScreenHeight;

  param.analyse.inter &= ~X264_ANALYSE_PSUB16x16;

  // The following two lines make x264 output 1 IDR and then all P.
  param.i_keyint_min = param.i_keyint_max = X264_KEYINT_MAX_INFINITE;

  param.i_scenecut_threshold = -1;
  param.i_csp = X264_CSP_I420;
  param.b_cabac = 1;
  param.b_interlaced = 0;
  param.i_bframe = 0;
  param.i_bframe_pyramid = 0;
  param.i_frame_reference = 1;
  param.b_constrained_intra = 1;
  param.analyse.i_weighted_pred = 0;
  param.analyse.b_weighted_bipred = 0;
  param.analyse.b_transform_8x8 = 0;
  param.analyse.i_chroma_qp_offset = 0;

  // Set QP = 32 for all frames.
  param.rc.i_rc_method = X264_RC_CQP;
  param.rc.i_qp_constant = param.rc.i_qp_min = param.rc.i_qp_max = 32;
  param.rc.f_ip_factor = 1.0;

  // Do not output SPS/PPS/SEI/unit delimeters.
  param.b_repeat_headers = 0;
  param.b_aud = 0;

  // return macroblock rows intead of NAL units
  // XXX x264 must also be modified to not produce macroblocks utilizing
  // planar prediction. this isn't toggleable at runtime for now...
  param.b_drh_mode = 1;

  // Yield one complete frame serially.
  param.i_threads = 1;
  param.b_sliced_threads = 0;
  param.i_slice_count = 1;
  param.nalu_process = H264Encoder::ProcessNalUnitTrampoline;

  // Logging parameters.
  param.i_log_level = X264_LOG_INFO;
  param.pf_log = log_encoder_messages;

  x264_param_apply_profile(&param, "main");

  encoder_ = x264_encoder_open(&param);
  assert(x264_encoder_maximum_delayed_frames(encoder_) == 0 ||
             "Encoder parameters will cause frames to be delayed");
}

const H264ChunkArray& H264Encoder::Encode(const std::vector<byte>& frame,
                                          bool idr) {
  x264_picture_t input;
  x264_picture_init(&input);

  // x264 requires us to pass a non-const pointer even though it does not
  // modify the input data. Cheat a bit to keep a clean interface.
  byte* data = const_cast<byte*>(frame.data());

  // Used to find ourselves back from x264 callbacks.
  input.opaque = this;

  // Input image parameters.
  input.img.i_csp = X264_CSP_I420;
  input.img.i_plane = 3;

  // Plane 0: Y, offset 0, 1 byte per horizontal pixel
  input.img.i_stride[0] = kScreenWidth;
  input.img.plane[0] = data;

  // Plane 1: U, offset sizeof(Y), 0.5 byte per horizontal pixel
  input.img.i_stride[1] = kScreenWidth / 2;
  input.img.plane[1] = input.img.plane[0] + (kScreenWidth * kScreenHeight);

  // Plane 2: V, offset sizeof(Y)+sizeof(U), 0.5 byte per horizontal pixel
  input.img.i_stride[2] = kScreenWidth / 2;
  input.img.plane[2] = input.img.plane[1] + (kScreenWidth * kScreenHeight) / 4;

  if (idr) {
    input.i_type = X264_TYPE_IDR;
  } else {
    input.i_type = X264_TYPE_P;
  }

  // Since we use a nalu_process callback, these will/might not contain correct
  // data (especially since we do not behave well and NAL encode the NAL units
  // sent to us by x264).
  x264_nal_t* nals;
  int nals_count;
  x264_picture_t output;

  // Reinitialize frame-related state.
  num_chunks_encoded_ = 0;
  curr_frame_idr_ = false;

  // Encode!
  x264_encoder_encode(encoder_, &nals, &nals_count, &input, &output);

  return chunks_;
}

void H264Encoder::ProcessNalUnit(x264_nal_t* nal) {
  int mb_per_frame = ((kScreenWidth + 15) / 16) * ((kScreenHeight + 15) / 16);
  int mb_per_chunk = mb_per_frame / kH264ChunksPerFrame;
  int chunk_idx = nal->i_first_mb / mb_per_chunk;
  chunks_[chunk_idx] = std::make_tuple(nal->p_payload, nal->i_payload);

  num_chunks_encoded_++;
  assert(num_chunks_encoded_ <= 5);
  if (num_chunks_encoded_ == 5) {
    curr_frame_idr_ = nal->i_ref_idc != NAL_PRIORITY_DISPOSABLE &&
                      nal->i_type == NAL_SLICE_IDR;
  }
}

void H264Encoder::ProcessNalUnitTrampoline(x264_t* h, x264_nal_t* nal,
                                           void* opaque) {
  H264Encoder* enc = static_cast<H264Encoder*>(opaque);
  enc->ProcessNalUnit(nal);
}

}  // namespace drc
