#pragma once

#include <array>
#include <drc/types.h>
#include <vector>

namespace drc {

const size_t kMaxVstrmHeaderSize = 16;

// Limit our payload size to around 1400 bytes to work on systems with default
// mtu configured (1500).
//
// TODO: does this make sense to do? The DRC camera and mic might be sending us
// larger frames and require the MTU change anyway.
const size_t kMaxVstrmPayloadSize = 1400;

enum class VstrmFrameRate {
  k59_94Hz = 0,
  k50Hz = 1,
  k29_97Hz = 2,
  k25Hz = 3,

  kUnknown = -1,
};

class VstrmPacket {
 public:
  VstrmPacket();
  VstrmPacket(const std::vector<byte>& packet);
  virtual ~VstrmPacket();

  u16 SeqId() const;
  u32 Timestamp() const;
  bool InitFlag() const;
  bool FrameBeginFlag() const;
  bool ChunkEndFlag() const;
  bool FrameEndFlag() const;
  bool IdrFlag() const;
  VstrmFrameRate FrameRate() const;
  const byte* Payload() const;
  size_t PayloadSize() const;

  void SetSeqId(u16 seqid);
  void SetTimestamp(u32 ts);
  void SetInitFlag(bool flag);
  void SetFrameBeginFlag(bool flag);
  void SetChunkEndFlag(bool flag);
  void SetFrameEndFlag(bool flag);
  void SetIdrFlag(bool flag);
  void SetFrameRate(VstrmFrameRate framerate);
  void SetPayload(const byte* payload, size_t size);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kMaxVstrmHeaderSize + PayloadSize(); }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  bool GetExtOption(u8 opt, u8* val = NULL) const;
  void SetExtOption(u8 opt, u8* val = NULL);
  void ClearExtOption(u8 opt, bool has_val);

  std::array<byte, kMaxVstrmHeaderSize + kMaxVstrmPayloadSize> pkt_;
};

}  // namespace drc
