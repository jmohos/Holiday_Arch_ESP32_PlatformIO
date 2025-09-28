// Protocol.h
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <string.h>

namespace Proto {

// ---- Constants ----
static constexpr uint8_t  MAGIC       = 0xA5;
static constexpr uint8_t  BROADCAST   = 0xFF;     // 255 = all nodes
static constexpr uint8_t  MIN_NODE_ID = 0x00;
static constexpr uint8_t  MAX_NODE_ID = 0xFE;     // 0..254
static constexpr size_t   HDR_SIZE    = 4;
static constexpr size_t   MAX_FRAME   = 256;      // tune as needed

// ---- Command codes (grow this as you add more) ----
enum Command : uint8_t {
  CMD_PING          = 0x00,
  CMD_CHANGE_MODE   = 0x01,
  CMD_TRIGGER_ANIM  = 0x02,
};

// ---- On-the-wire header (4 bytes) ----
// We don't rely on struct packing on the wire; we explicitly write/read bytes.
struct Header {
  uint8_t magic;   // always 0xA5
  uint8_t dst;     // 0..254, or 255 for broadcast
  uint8_t src;     // 0..254
  uint8_t cmd;     // Command code
};

// ---- Parsed view of a received frame ----
struct View {
  Header       hdr{};
  const uint8_t* payload = nullptr;   // points into received buffer
  size_t       payload_len = 0;
};

// ---- Helpers ----
inline bool isBroadcast(uint8_t dst) { return dst == BROADCAST; }
inline bool isForMe(uint8_t myId, uint8_t dst) {
  return dst == myId || dst == BROADCAST;
}

// Encode just the 4B header into out[], returns bytes written (4) or 0 on error
inline size_t encodeHeader(uint8_t* out, size_t cap,
                           uint8_t dst, uint8_t src, uint8_t cmd) {
  if (!out || cap < HDR_SIZE) return 0;
  out[0] = MAGIC;
  out[1] = dst;
  out[2] = src;
  out[3] = cmd;
  return HDR_SIZE;
}

// ---- Command-specific encoders (examples) ----
// 0x00 Ping: no payload
inline size_t buildPing(uint8_t* out, size_t cap, uint8_t dst, uint8_t src,
                        int8_t rssi, uint16_t range) {
  if (cap < HDR_SIZE + 3) return 0;
  size_t n = encodeHeader(out, cap, dst, src, CMD_PING);
  out[n++] = rssi;
  out[n++] = (uint8_t)(range & 0xff);
  out[n++] = (uint8_t)((range >> 8) & 0xFF);
  return n;
}

// 0x01 Change Mode: payload[0] = mode
inline size_t buildChangeMode(uint8_t* out, size_t cap, uint8_t dst, uint8_t src,
                              uint8_t mode) {
  if (cap < HDR_SIZE + 1) return 0;
  size_t n = encodeHeader(out, cap, dst, src, CMD_CHANGE_MODE);
  out[n++] = mode;
  return n;
}

// 0x02 Trigger Animation: payload[0] = animId
inline size_t buildTriggerAnim(uint8_t* out, size_t cap, uint8_t dst, uint8_t src,
                               uint8_t animId) {
  if (cap < HDR_SIZE + 1) return 0;
  size_t n = encodeHeader(out, cap, dst, src, CMD_TRIGGER_ANIM);
  out[n++] = animId;
  return n;
}

// ---- Parser ----
// Returns true on success; fills View with header + payload view.
// Validates magic and minimum length.
inline bool parse(const uint8_t* buf, size_t len, View& out) {
  if (!buf || len < HDR_SIZE) return false;
  if (buf[0] != MAGIC) return false;

  out.hdr.magic = buf[0];
  out.hdr.dst   = buf[1];
  out.hdr.src   = buf[2];
  out.hdr.cmd   = buf[3];
  out.payload   = (len > HDR_SIZE) ? (buf + HDR_SIZE) : nullptr;
  out.payload_len = (len > HDR_SIZE) ? (len - HDR_SIZE) : 0;
  return true;
}

// ---- Tiny payload decoders (optional convenience) ----
inline bool decodeChangeMode(const View& v, uint8_t& modeOut) {
  if (v.hdr.cmd != CMD_CHANGE_MODE || v.payload_len < 1) return false;
  modeOut = v.payload[0];
  return true;
}
inline bool decodeTriggerAnim(const View& v, uint8_t& animIdOut) {
  if (v.hdr.cmd != CMD_TRIGGER_ANIM || v.payload_len < 1) return false;
  animIdOut = v.payload[0];
  return true;
}

} // namespace Proto
