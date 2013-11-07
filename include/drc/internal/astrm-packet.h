#pragma once

#include <array>
#include <drc/types.h>
#include <vector>

namespace drc {

const size_t kAstrmHeaderSize = 8;
const size_t kMaxAstrmPayloadSize = 1700;

enum class AstrmFormat {
  kPcm24KHz = 0,
  kPcm48KHz = 1,
  kAlaw24KHz = 2,
  kAlaw48KHz = 3,
  kUlaw24KHz = 4,
  kUlaw48KHz = 5,
};

enum class AstrmPacketType {
  kAudioData = 0,
  kVideoFormat = 1,
};

class AstrmPacket {
 public:
  AstrmPacket();
  AstrmPacket(const std::vector<byte>& packet);
  virtual ~AstrmPacket();

  AstrmFormat Format() const;
  bool MonoFlag() const;
  bool VibrateFlag() const;
  AstrmPacketType PacketType() const;
  u16 SeqId() const;
  u32 Timestamp() const;
  const byte* Payload() const;
  size_t PayloadSize() const;

  void SetFormat(AstrmFormat format);
  void SetMonoFlag(bool flag);
  void SetVibrateFlag(bool flag);
  void SetPacketType(AstrmPacketType type);
  void SetSeqId(u16 seqid);
  void SetTimestamp(u32 timestamp);
  void SetPayload(const byte* payload, size_t size);

  const byte* GetBytes() const { return pkt_.data(); }
  size_t GetSize() const { return kAstrmHeaderSize + PayloadSize(); }

  // Reset the packet to use default values for everything.
  void ResetPacket();

 private:
  std::array<byte, kAstrmHeaderSize + kMaxAstrmPayloadSize> pkt_;
};

}  // namespace drc
