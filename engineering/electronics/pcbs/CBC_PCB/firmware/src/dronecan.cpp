#include "dronecan.h"
#include <string.h>
#include <esp_system.h>
#if __has_include(<esp_random.h>)
#include <esp_random.h> // esp_random() lives here on ESP-IDF 5.x / Arduino core 3.x
#endif

// ---------- helpers ----------

float float16ToFloat(uint16_t h) {
  uint32_t sign = (uint32_t)(h >> 15) & 1U;
  uint32_t exp = (uint32_t)(h >> 10) & 0x1FU;
  uint32_t mant = (uint32_t)h & 0x3FFU;
  uint32_t f;
  if (exp == 0) {
    if (mant == 0) {
      f = sign << 31; // +/- 0
    } else {
      // subnormal -> normalize
      exp = 127 - 15 + 1;
      while ((mant & 0x400U) == 0) {
        mant <<= 1;
        exp--;
      }
      mant &= 0x3FFU;
      f = (sign << 31) | (exp << 23) | (mant << 13);
    }
  } else if (exp == 0x1F) {
    f = (sign << 31) | (0xFFU << 23) | (mant << 13); // inf/nan
  } else {
    f = (sign << 31) | ((exp - 15 + 127) << 23) | (mant << 13);
  }
  float out;
  memcpy(&out, &f, 4);
  return out;
}

uint16_t floatToFloat16(float f) {
  uint32_t x;
  memcpy(&x, &f, 4);
  uint16_t sign = (uint16_t)((x >> 16) & 0x8000U);
  int32_t exp = (int32_t)((x >> 23) & 0xFFU) - 127 + 15;
  uint32_t mant = x & 0x7FFFFFU;
  if (exp <= 0) return sign; // flush subnormals/underflow to +/- 0
  if (exp >= 0x1F) return (uint16_t)(sign | 0x7C00U); // inf/overflow
  return (uint16_t)(sign | ((uint32_t)exp << 10) | (mant >> 13));
}

// CRC-16-CCITT-FALSE (poly 0x1021, init 0xFFFF) as used for UAVCAN v0
// transfer CRCs. The CRC is seeded with the 64-bit data type signature
// (little-endian byte order), then run over the full transfer payload.
static uint16_t crc16Add(uint16_t crc, uint8_t b) {
  crc ^= (uint16_t)b << 8;
  for (int i = 0; i < 8; i++)
    crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
  return crc;
}

static uint16_t transferCrc(uint64_t signature, const uint8_t *payload, uint16_t len) {
  uint16_t crc = 0xFFFFU;
  for (int i = 0; i < 8; i++)
    crc = crc16Add(crc, (uint8_t)(signature >> (8 * i)));
  for (uint16_t i = 0; i < len; i++)
    crc = crc16Add(crc, payload[i]);
  return crc;
}

// --- v0 scalar bit (de)serialization, ported from libcanard ---
// Semantics: a scalar is laid out as its little-endian byte array; if the bit
// length is not byte-aligned, the final partial byte is shifted to the MSB
// side; then bits are streamed sequentially, MSB-first within each byte.
// (This is what canardEncodeScalar/canardDecodeScalar do — NOT a plain
// MSB-first dump of the value, which differs for multi-byte unaligned fields
// like BatteryInfo's uint11 status_flags.)

static void copyBitArray(const uint8_t *src, uint32_t src_offset,
                         uint32_t src_len, uint8_t *dst, uint32_t dst_offset) {
  src += src_offset / 8U;
  dst += dst_offset / 8U;
  src_offset %= 8U;
  dst_offset %= 8U;
  const uint32_t last_bit = src_offset + src_len;
  while (last_bit - src_offset) {
    const uint8_t sb = (uint8_t)(src_offset % 8U);
    const uint8_t db = (uint8_t)(dst_offset % 8U);
    const uint8_t maxo = sb > db ? sb : db;
    const uint32_t rem = last_bit - src_offset;
    const uint32_t copy_bits = rem < (8U - maxo) ? rem : (8U - maxo);
    const uint8_t write_mask = (uint8_t)((uint8_t)(0xFF00U >> copy_bits) >> db);
    const uint8_t src_data = (uint8_t)(((uint32_t)src[src_offset / 8U] << sb) >> db);
    dst[dst_offset / 8U] = (uint8_t)(((uint32_t)dst[dst_offset / 8U] &
                                      (uint32_t)~write_mask) |
                                     (uint32_t)(src_data & write_mask));
    src_offset += copy_bits;
    dst_offset += copy_bits;
  }
}

static void encodeScalar(uint8_t *dst, uint32_t bit_ofs, uint8_t bit_len,
                         uint64_t value) {
  uint8_t bytes[8];
  for (int i = 0; i < 8; i++) bytes[i] = (uint8_t)(value >> (8 * i));
  if (bit_len % 8U)
    bytes[bit_len / 8U] = (uint8_t)(bytes[bit_len / 8U] << (8U - (bit_len % 8U)));
  copyBitArray(bytes, 0, bit_len, dst, bit_ofs);
}

static uint64_t decodeScalar(const uint8_t *src, uint32_t bit_ofs,
                             uint8_t bit_len, bool value_is_signed = false) {
  uint8_t bytes[8] = {0};
  copyBitArray(src, bit_ofs, bit_len, bytes, 0);
  if (bit_len % 8U)
    bytes[bit_len / 8U] = (uint8_t)(bytes[bit_len / 8U] >> (8U - (bit_len % 8U)));
  uint64_t v = 0;
  for (int i = 0; i < 8; i++) v |= (uint64_t)bytes[i] << (8 * i);
  if (value_is_signed && bit_len < 64 && (v & (1ULL << (bit_len - 1))))
    v |= ~((1ULL << bit_len) - 1ULL);
  return v;
}

static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// ---------- RX ----------

bool DroneCanRx::handleFrame(const struct can_frame &f, uint32_t now_ms) {
  if (!(f.can_id & CAN_EFF_FLAG) || f.can_dlc < 1) {
    frames_non_dronecan++;
    return false;
  }
  uint32_t id = f.can_id & CAN_EFF_MASK;
  if (id & 0x80U) return false; // service frame — ignored
  uint8_t src = id & 0x7FU;
  uint16_t dtid = (uint16_t)((id >> 8) & 0xFFFFU);

  uint8_t tail = f.data[f.can_dlc - 1];
  bool sot = tail & 0x80U, eot = tail & 0x40U, tog = (tail >> 5) & 1U;
  uint8_t tid = tail & 0x1FU;
  uint8_t plen = f.can_dlc - 1;

  if (sot && eot) { // single-frame transfer, no transfer CRC
    completeTransfer(src, dtid, f.data, plen, true, now_ms);
    transfers_ok++;
    return true;
  }

  if (sot) {
    if (tog) return false; // first frame must have toggle=0
    reasm.active = true;
    reasm.src = src;
    reasm.tid = tid;
    reasm.dtid = dtid;
    reasm.toggle = 0;
    reasm.len = 0;
  } else {
    if (!reasm.active || reasm.src != src || reasm.dtid != dtid ||
        tog != (reasm.toggle ^ 1U)) {
      reasm.active = false;
      return false;
    }
    if (tid != reasm.tid) {
      // Tattu quirk: the pack increments the transfer ID on EVERY frame
      // instead of keeping it constant per transfer. Accept +1 sequences
      // for the Tattu data type only.
      if (dtid == DTID_TATTU_BATTERY && tid == ((reasm.tid + 1U) & 0x1FU)) {
        reasm.tid = tid;
      } else {
        reasm.active = false;
        return false;
      }
    }
    reasm.toggle ^= 1U;
  }

  if (reasm.len + plen > sizeof(reasm.buf)) {
    reasm.active = false;
    return false;
  }
  memcpy(reasm.buf + reasm.len, f.data, plen);
  reasm.len += plen;

  if (eot) {
    reasm.active = false;
    if (reasm.len < 3) return false;
    uint16_t rxCrc = le16(reasm.buf); // first 2 bytes of a multi-frame transfer
    const uint8_t *payload = reasm.buf + 2;
    uint16_t len = reasm.len - 2;
    bool crcOk = true;
    if (dtid == DTID_BATTERY_INFO) {
      crcOk = (transferCrc(SIG_BATTERY_INFO, payload, len) == rxCrc);
      if (!crcOk) crc_errors++;
    }
    completeTransfer(src, dtid, payload, len, crcOk, now_ms);
    transfers_ok++;
  }
  return true;
}

void DroneCanRx::completeTransfer(uint8_t src, uint16_t dtid, const uint8_t *p,
                                  uint16_t len, bool crcOk, uint32_t now_ms) {
  if (dtid == DTID_NODE_STATUS && len >= 7) {
    nodeStatus.valid = true;
    nodeStatus.last_update_ms = now_ms;
    nodeStatus.source_node = src;
    nodeStatus.uptime_sec = le32(p);
    nodeStatus.health = (p[4] >> 6) & 3U;
    nodeStatus.mode = (p[4] >> 3) & 7U;
    nodeStatus.sub_mode = p[4] & 7U;
    nodeStatus.vendor_code = le16(p + 5);
  } else if (dtid == DTID_TATTU_BATTERY && len >= 60) {
    // Tattu 18S "E-UAVCAN" broadcast (TATTU-2177 spec) — see dronecan.h for
    // the full layout. Transfer CRC not validated (vendor seed unknown).
    // 60-byte variant = no serial; 76-byte variant appends char serial[16].
    BatteryTelemetry &b = battery;
    b.valid = true;
    b.crc_ok = true; // unvalidated
    b.last_update_ms = now_ms;
    b.source_node = src;
    b.protocol = 2;
    b.tattu_vendor = le16(p + 0);
    b.model_instance_id = le16(p + 2);
    b.voltage = le16(p + 4) * 0.01f;                  // 10 mV units
    b.current = (int16_t)le16(p + 6) * 0.01f;         // 10 mA, + = charging
    b.temperature_k = (int16_t)le16(p + 8) + 273.15f; // degC
    uint16_t soc = le16(p + 10);
    b.soc_pct = (uint8_t)(soc > 100 ? 100 : soc);
    b.cycles = le16(p + 12);
    int16_t health = (int16_t)le16(p + 14);
    if (health < 0) health = 0;
    if (health > 100) health = 100;
    b.soh_pct = (uint8_t)health;
    b.cell_count = 18;
    for (uint8_t i = 0; i < 18; i++) b.cells_mv[i] = le16(p + 16 + 2 * i);
    b.design_mah = le16(p + 52);
    b.remaining_mah = le16(p + 54);
    b.error_flags = le32(p + 56);
    b.remaining_capacity_wh = b.remaining_mah * 0.001f * b.voltage;
    b.full_charge_capacity_wh = b.design_mah * 0.001f * b.voltage;
    if (len >= 76) {
      memcpy(b.serial, p + 60, 16);
      b.serial[16] = 0;
    }
    strncpy(b.model_name, "Tattu 18S", sizeof(b.model_name) - 1);
    // Map Tattu error bits onto BatteryInfo STATUS_FLAG_* values.
    uint16_t sf = 1; // STATUS_FLAG_IN_USE
    if (b.current > 0.05f) sf |= 2;                    // CHARGING
    if (b.error_flags & (1UL << 0)) sf |= 16;          // TEMP_COLD
    if (b.error_flags & (1UL << 1)) sf |= 8;           // TEMP_HOT
    if (b.error_flags & ((1UL << 2) | (1UL << 3) |     // chg/dis overcurrent
                         (1UL << 9) | (1UL << 10)))    // chg/dis short
      sf |= 32;                                        // OVERLOAD
    if (b.error_flags != 0) sf |= 256;                 // BMS_ERROR
    b.status_flags = sf;
  } else if (dtid == DTID_BATTERY_INFO && len >= 23) {
    BatteryTelemetry &b = battery;
    b.valid = true;
    b.crc_ok = crcOk;
    b.last_update_ms = now_ms;
    b.source_node = src;
    b.protocol = 1;
    b.temperature_k = float16ToFloat(le16(p + 0));
    b.voltage = float16ToFloat(le16(p + 2));
    b.current = float16ToFloat(le16(p + 4));
    b.average_power_10sec = float16ToFloat(le16(p + 6));
    b.remaining_capacity_wh = float16ToFloat(le16(p + 8));
    b.full_charge_capacity_wh = float16ToFloat(le16(p + 10));
    b.hours_to_full_charge = float16ToFloat(le16(p + 12));
    // Packed tail: uint11 status_flags, uint7 soh, uint7 soc, uint7 soc_stdev
    // starting at bit 112 (byte 14), canard scalar semantics.
    b.status_flags = (uint16_t)decodeScalar(p, 112, 11);
    b.soh_pct = (uint8_t)decodeScalar(p, 123, 7);
    b.soc_pct = (uint8_t)decodeScalar(p, 130, 7);
    b.soc_stdev = (uint8_t)decodeScalar(p, 137, 7);
    b.battery_id = p[18];
    b.model_instance_id = le32(p + 19);
    uint16_t nameLen = len - 23;
    if (nameLen > 32) nameLen = 32;
    memcpy(b.model_name, p + 23, nameLen);
    b.model_name[nameLen] = 0;
  }
}

// ---------- DroneCanNode: allocation client + NodeStatus + GetNodeInfo ----------

void DroneCanNode::begin(SendFn send, const uint8_t unique_id[16],
                         uint8_t static_node_id) {
  send_ = send;
  memcpy(unique_id_, unique_id, 16);
  node_id_ = static_node_id & 0x7FU;
}

void DroneCanNode::poll(uint32_t now) {
  if (!send_) return;
  if (!allocated()) {
    if (next_alloc_request_ms_ == 0)
      next_alloc_request_ms_ = now + 600 + (esp_random() % 400);
    // If the server stopped echoing mid-handshake, restart from stage 1.
    if (confirmed_uid_bytes_ != 0 && now - last_server_echo_ms_ > 1000)
      confirmed_uid_bytes_ = 0;
    if ((int32_t)(now - next_alloc_request_ms_) >= 0) {
      sendAllocationRequest(now);
      next_alloc_request_ms_ = now + 600 + (esp_random() % 400);
    }
    return;
  }
  if (now - last_node_status_ms_ >= 1000) {
    last_node_status_ms_ = now;
    struct can_frame f;
    dronecanMakeNodeStatus(f, node_id_, now / 1000, health_, 0);
    send_(f);
  }
}

void DroneCanNode::sendAllocationRequest(uint32_t now) {
  static uint8_t alloc_tid = 0;
  uint8_t offset = confirmed_uid_bytes_;
  uint8_t count = (uint8_t)(16 - offset);
  if (count > 6) count = 6;

  struct can_frame f;
  // Anonymous message frame: priority | 14-bit random discriminator |
  // lower 2 bits of dtid | source node 0.
  uint32_t disc = esp_random() & 0x3FFFU;
  f.can_id = CAN_EFF_FLAG | (24UL << 24) | (disc << 10) |
             (((uint32_t)DTID_ALLOCATION & 3U) << 8);
  f.data[0] = (uint8_t)((0U << 1) | (offset == 0 ? 1U : 0U)); // any ID; first_part flag
  memcpy(f.data + 1, unique_id_ + offset, count);
  f.can_dlc = (uint8_t)(1 + count + 1);
  f.data[f.can_dlc - 1] = (uint8_t)(0xC0U | (alloc_tid & 0x1FU)); // single frame
  alloc_tid = (alloc_tid + 1) & 0x1FU;
  send_(f);
  (void)now;
}

void DroneCanNode::handleAllocationBroadcast(const uint8_t *p, uint16_t len,
                                             uint32_t now) {
  if (len < 1) return;
  uint8_t offered_node_id = p[0] >> 1;
  const uint8_t *uid = p + 1;
  uint16_t uidLen = (uint16_t)(len - 1);
  if (uidLen > 16) uidLen = 16;

  if (uidLen == 0 || memcmp(uid, unique_id_, uidLen) != 0) {
    // Someone else's allocation in progress — back off per spec.
    next_alloc_request_ms_ = now + 600 + (esp_random() % 400);
    confirmed_uid_bytes_ = 0;
    return;
  }
  last_server_echo_ms_ = now;
  if (uidLen == 16) {
    if (offered_node_id != 0) node_id_ = offered_node_id; // allocated!
    return;
  }
  confirmed_uid_bytes_ = (uint8_t)uidLen;
  next_alloc_request_ms_ = now + (esp_random() % 400); // follow-up stage
}

void DroneCanNode::handleFrame(const struct can_frame &f, uint32_t now) {
  if (!(f.can_id & CAN_EFF_FLAG) || f.can_dlc < 1) return;
  uint32_t id = f.can_id & CAN_EFF_MASK;
  uint8_t src = id & 0x7FU;

  if (id & 0x80U) { // service frame
    if (!allocated()) return;
    uint8_t dest = (id >> 8) & 0x7FU;
    uint8_t stype = (id >> 16) & 0xFFU;
    bool isRequest = (id >> 15) & 1U;
    if (!isRequest || dest != node_id_) return;

    uint8_t tail = f.data[f.can_dlc - 1];
    bool sot = tail & 0x80U, eot = tail & 0x40U, tog = (tail >> 5) & 1U;
    uint8_t tid = tail & 0x1FU;
    uint8_t plen = (uint8_t)(f.can_dlc - 1);

    if (sot && eot) {
      handleServiceRequest(src, stype, tid, f.data, plen);
      return;
    }
    // multi-frame request (e.g. a param set-request) — reassemble
    if (sot) {
      if (tog) return;
      srv_reasm_.active = true;
      srv_reasm_.src = src;
      srv_reasm_.stype = stype;
      srv_reasm_.tid = tid;
      srv_reasm_.toggle = 0;
      srv_reasm_.len = 0;
    } else {
      if (!srv_reasm_.active || srv_reasm_.src != src ||
          srv_reasm_.stype != stype || srv_reasm_.tid != tid ||
          tog != (srv_reasm_.toggle ^ 1U)) {
        srv_reasm_.active = false;
        return;
      }
      srv_reasm_.toggle ^= 1U;
    }
    if (srv_reasm_.len + plen > sizeof(srv_reasm_.buf)) {
      srv_reasm_.active = false;
      return;
    }
    memcpy(srv_reasm_.buf + srv_reasm_.len, f.data, plen);
    srv_reasm_.len += plen;
    if (eot) {
      srv_reasm_.active = false;
      if (srv_reasm_.len >= 3) // skip 2-byte transfer CRC (not validated)
        handleServiceRequest(src, stype, tid, srv_reasm_.buf + 2,
                             (uint16_t)(srv_reasm_.len - 2));
    }
    return;
  }

  if (src == 0) { // anonymous frame = another allocatee requesting; back off
    if (((id >> 8) & 3U) == DTID_ALLOCATION && !allocated())
      next_alloc_request_ms_ = now + 600 + (esp_random() % 400);
    return;
  }

  uint16_t dtid = (uint16_t)((id >> 8) & 0xFFFFU);
  if (dtid != DTID_ALLOCATION || allocated()) return;

  // Allocation broadcast from the server (single- or multi-frame).
  uint8_t tail = f.data[f.can_dlc - 1];
  bool sot = tail & 0x80U, eot = tail & 0x40U, tog = (tail >> 5) & 1U;
  uint8_t tid = tail & 0x1FU;
  uint8_t plen = (uint8_t)(f.can_dlc - 1);

  if (sot && eot) {
    handleAllocationBroadcast(f.data, plen, now);
    return;
  }
  if (sot) {
    if (tog) return;
    alloc_reasm_.active = true;
    alloc_reasm_.src = src;
    alloc_reasm_.tid = tid;
    alloc_reasm_.toggle = 0;
    alloc_reasm_.len = 0;
  } else {
    if (!alloc_reasm_.active || alloc_reasm_.src != src ||
        alloc_reasm_.tid != tid || tog != (alloc_reasm_.toggle ^ 1U)) {
      alloc_reasm_.active = false;
      return;
    }
    alloc_reasm_.toggle ^= 1U;
  }
  if (alloc_reasm_.len + plen > sizeof(alloc_reasm_.buf)) {
    alloc_reasm_.active = false;
    return;
  }
  memcpy(alloc_reasm_.buf + alloc_reasm_.len, f.data, plen);
  alloc_reasm_.len += plen;
  if (eot) {
    alloc_reasm_.active = false;
    if (alloc_reasm_.len >= 3) // skip 2-byte transfer CRC (not validated)
      handleAllocationBroadcast(alloc_reasm_.buf + 2,
                                (uint16_t)(alloc_reasm_.len - 2), now);
  }
}

void DroneCanNode::sendGetNodeInfoResponse(uint8_t dest, uint8_t tid) {
  uint8_t p[64];
  // NodeStatus (7 bytes)
  uint32_t up = last_node_status_ms_ / 1000;
  p[0] = (uint8_t)up;
  p[1] = (uint8_t)(up >> 8);
  p[2] = (uint8_t)(up >> 16);
  p[3] = (uint8_t)(up >> 24);
  p[4] = (uint8_t)((health_ & 3U) << 6); // mode OPERATIONAL, sub 0
  p[5] = 0;
  p[6] = 0;
  // SoftwareVersion: major, minor, optional_field_flags, vcs_commit, image_crc
  p[7] = 0;  // major
  p[8] = 3;  // minor
  memset(p + 9, 0, 13);
  // HardwareVersion: major, minor, unique_id[16], certificate len (0)
  p[22] = 1;
  p[23] = 0;
  memcpy(p + 24, unique_id_, 16);
  p[40] = 0;
  // name (tail array, no length prefix)
  static const char name[] = "org.arrowair.cbc";
  const uint16_t nameLen = sizeof(name) - 1;
  memcpy(p + 41, name, nameLen);
  uint16_t total = 41 + nameLen;

  // Service response frame ID: priority | service type | response | dest | 1 | src
  uint32_t id = (30UL << 24) | ((uint32_t)SRVID_GET_NODE_INFO << 16) |
                ((uint32_t)dest << 8) | 0x80U | node_id_;
  sendMultiFrame(id, p, total, SIG_GET_NODE_INFO, tid);
}

void DroneCanNode::sendMultiFrame(uint32_t id, const uint8_t *payload,
                                  uint16_t len, uint64_t signature, uint8_t tid) {
  struct can_frame f;
  f.can_id = CAN_EFF_FLAG | id;
  if (len <= 7) {
    memcpy(f.data, payload, len);
    f.data[len] = (uint8_t)(0xC0U | (tid & 0x1FU));
    f.can_dlc = (uint8_t)(len + 1);
    send_(f);
    return;
  }
  uint16_t crc = transferCrc(signature, payload, len);
  uint32_t total = (uint32_t)len + 2; // stream = crc_lo, crc_hi, payload
  uint32_t off = 0;
  uint8_t toggle = 0;
  bool first = true;
  while (off < total) {
    uint8_t chunk = (uint8_t)((total - off > 7) ? 7 : (total - off));
    for (uint8_t i = 0; i < chunk; i++) {
      uint32_t s = off + i;
      f.data[i] = (s == 0) ? (uint8_t)(crc & 0xFFU)
                : (s == 1) ? (uint8_t)(crc >> 8)
                           : payload[s - 2];
    }
    off += chunk;
    f.data[chunk] = (uint8_t)((first ? 0x80U : 0U) | (off >= total ? 0x40U : 0U) |
                              ((uint32_t)toggle << 5) | (tid & 0x1FU));
    f.can_dlc = (uint8_t)(chunk + 1);
    send_(f);
    toggle ^= 1U;
    first = false;
  }
}

bool DroneCanNode::broadcast(uint16_t dtid, uint64_t signature,
                             const uint8_t *payload, uint16_t len) {
  if (!send_ || !allocated()) return false;
  const uint32_t priority = 16; // medium
  uint32_t id = (priority << 24) | ((uint32_t)dtid << 8) | node_id_;
  sendMultiFrame(id, payload, len, signature, bcast_tid_);
  bcast_tid_ = (bcast_tid_ + 1U) & 0x1FU;
  return true;
}

void DroneCanNode::handleServiceRequest(uint8_t src, uint8_t stype, uint8_t tid,
                                        const uint8_t *p, uint16_t len) {
  if (stype == SRVID_GET_NODE_INFO) {
    sendGetNodeInfoResponse(src, tid);
  } else if (stype == SRVID_PARAM_GETSET) {
    handleParamGetSet(src, tid, p, len);
  } else if (stype == SRVID_PARAM_EXECUTEOPCODE) {
    if (len < 1) return;
    uint8_t opcode = p[0];
    bool ok = false;
    if (opcode == 0) { // SAVE
      if (save_cb_) {
        save_cb_();
        ok = true;
      }
    } else if (opcode == 1) { // ERASE -> reset to defaults, then persist
      for (uint8_t i = 0; i < nparams_; i++)
        params_[i].value = params_[i].defval;
      if (erase_cb_) {
        erase_cb_();
        ok = true;
      }
    }
    uint8_t out[7] = {0}; // int48 argument (0) + bool ok = 49 bits
    encodeScalar(out, 48, 1, ok ? 1U : 0U);
    uint32_t id = (30UL << 24) | ((uint32_t)SRVID_PARAM_EXECUTEOPCODE << 16) |
                  ((uint32_t)src << 8) | 0x80U | node_id_;
    sendMultiFrame(id, out, 7, SIG_PARAM_EXECUTEOPCODE, tid);
  } else if (stype == SRVID_RESTART_NODE) {
    if (len < 5) return;
    uint64_t magic = decodeScalar(p, 0, 40);
    bool ok = (magic == RESTART_NODE_MAGIC) && restart_cb_ != nullptr;
    uint8_t out[1] = {0};
    encodeScalar(out, 0, 1, ok ? 1U : 0U);
    uint32_t id = (30UL << 24) | ((uint32_t)SRVID_RESTART_NODE << 16) |
                  ((uint32_t)src << 8) | 0x80U | node_id_;
    sendMultiFrame(id, out, 1, SIG_RESTART_NODE, tid);
    if (ok)
      restart_cb_(); // handler delays to let the frame flush, then restarts
  }
}

void DroneCanNode::handleParamGetSet(uint8_t dest, uint8_t tid,
                                     const uint8_t *p, uint16_t len) {
  // Request: uint13 index, param.Value (3-bit union tag), uint8[<=92] name
  // (tail array, no length prefix). Empty value = read; non-empty = write.
  if (len < 2) return;
  uint16_t index = (uint16_t)decodeScalar(p, 0, 13);
  uint8_t tag = (uint8_t)decodeScalar(p, 13, 3);
  bool hasValue = false;
  int64_t newVal = 0;
  uint32_t nameByte = 2;
  switch (tag) {
  case 0: // empty
    break;
  case 1: // integer
    if (len < 10) return;
    newVal = (int64_t)decodeScalar(p, 16, 64, true);
    hasValue = true;
    nameByte = 10;
    break;
  case 2: { // real (accept, truncate to int)
    if (len < 6) return;
    uint32_t raw = (uint32_t)decodeScalar(p, 16, 32);
    float fv;
    memcpy(&fv, &raw, 4);
    newVal = (int64_t)fv;
    hasValue = true;
    nameByte = 6;
    break;
  }
  case 3: // boolean
    if (len < 3) return;
    newVal = decodeScalar(p, 16, 8) ? 1 : 0;
    hasValue = true;
    nameByte = 3;
    break;
  default: // string / unknown — unsupported for integer params
    return;
  }
  int idx = -1;
  uint16_t nameLen = (len > nameByte) ? (uint16_t)(len - nameByte) : 0;
  if (nameLen > 0 && nameLen <= 92) {
    for (uint8_t i = 0; i < nparams_; i++) {
      if (strlen(params_[i].name) == nameLen &&
          memcmp(params_[i].name, p + nameByte, nameLen) == 0) {
        idx = i;
        break;
      }
    }
  } else if (index < nparams_) {
    idx = index;
  }
  if (idx >= 0 && hasValue) {
    int64_t v = newVal;
    if (v < params_[idx].minval) v = params_[idx].minval;
    if (v > params_[idx].maxval) v = params_[idx].maxval;
    params_[idx].value = v;
  }
  sendParamGetSetResponse(dest, tid, idx);
}

void DroneCanNode::sendParamGetSetResponse(uint8_t dest, uint8_t tid,
                                           int paramIdx) {
  // Response: Value value, Value default_value, NumericValue max_value,
  // NumericValue min_value, uint8[<=92] name (tail array).
  // All-empty unions (10 bits) when param not found — that's how the GUI
  // detects the end of the parameter list.
  uint8_t out[64];
  memset(out, 0, sizeof(out));
  uint32_t bo = 0;
  if (paramIdx < 0 || paramIdx >= (int)nparams_) {
    bo = 10; // 3+3+2+2 empty tags, no name
  } else {
    const DroneCanParam &pr = params_[paramIdx];
    encodeScalar(out, bo, 3, 1); // Value tag = integer
    bo += 3;
    encodeScalar(out, bo, 64, (uint64_t)pr.value);
    bo += 64;
    encodeScalar(out, bo, 3, 1);
    bo += 3;
    encodeScalar(out, bo, 64, (uint64_t)pr.defval);
    bo += 64;
    encodeScalar(out, bo, 2, 1); // NumericValue tag = integer
    bo += 2;
    encodeScalar(out, bo, 64, (uint64_t)pr.maxval);
    bo += 64;
    encodeScalar(out, bo, 2, 1);
    bo += 2;
    encodeScalar(out, bo, 64, (uint64_t)pr.minval);
    bo += 64;
    for (const char *c = pr.name; *c; c++) {
      encodeScalar(out, bo, 8, (uint8_t)*c);
      bo += 8;
    }
  }
  uint16_t total = (uint16_t)((bo + 7) / 8);
  uint32_t id = (30UL << 24) | ((uint32_t)SRVID_PARAM_GETSET << 16) |
                ((uint32_t)dest << 8) | 0x80U | node_id_;
  sendMultiFrame(id, out, total, SIG_PARAM_GETSET, tid);
}

// ---------- TX ----------

// Encode uavcan.equipment.power.BatteryInfo. Layout (byte-aligned unless
// noted): 7x float16, then uint11 status_flags / uint7 soh / uint7 soc /
// uint7 soc_stdev (MSB-first packed, bits 112..143), u8 battery_id,
// u32 model_instance_id, then model_name as tail array (no length prefix).
uint16_t dronecanEncodeBatteryInfo(uint8_t *out, const BatteryTelemetry &b) {
  // BatteryInfo current convention (ArduPilot AP_BattMonitor_DroneCAN):
  // positive = discharging. Tattu reports positive = charging, so negate.
  float current = (b.protocol == 2) ? -b.current : b.current;
  uint16_t f16[7] = {
      floatToFloat16(b.temperature_k),
      floatToFloat16(b.voltage),
      floatToFloat16(current),
      floatToFloat16(b.average_power_10sec),
      floatToFloat16(b.remaining_capacity_wh),
      floatToFloat16(b.full_charge_capacity_wh),
      floatToFloat16(b.hours_to_full_charge),
  };
  for (int i = 0; i < 7; i++) {
    out[2 * i] = (uint8_t)(f16[i] & 0xFFU);
    out[2 * i + 1] = (uint8_t)(f16[i] >> 8);
  }
  out[14] = out[15] = out[16] = out[17] = 0;
  encodeScalar(out, 112, 11, b.status_flags);
  encodeScalar(out, 123, 7, b.soh_pct);
  encodeScalar(out, 130, 7, b.soc_pct);
  encodeScalar(out, 137, 7, b.soc_stdev);
  out[18] = b.battery_id;
  out[19] = (uint8_t)(b.model_instance_id);
  out[20] = (uint8_t)(b.model_instance_id >> 8);
  out[21] = (uint8_t)(b.model_instance_id >> 16);
  out[22] = (uint8_t)(b.model_instance_id >> 24);
  uint16_t nameLen = (uint16_t)strlen(b.model_name);
  if (nameLen > 31) nameLen = 31;
  memcpy(out + 23, b.model_name, nameLen);
  return (uint16_t)(23 + nameLen);
}

void dronecanMakeNodeStatus(struct can_frame &f, uint8_t node_id,
                            uint32_t uptime_sec, uint8_t health, uint8_t mode) {
  static uint8_t tid = 0;
  const uint32_t priority = 16; // medium
  f.can_id = CAN_EFF_FLAG | (priority << 24) |
             ((uint32_t)DTID_NODE_STATUS << 8) | (node_id & 0x7FU);
  f.can_dlc = 8;
  f.data[0] = (uint8_t)(uptime_sec);
  f.data[1] = (uint8_t)(uptime_sec >> 8);
  f.data[2] = (uint8_t)(uptime_sec >> 16);
  f.data[3] = (uint8_t)(uptime_sec >> 24);
  f.data[4] = (uint8_t)(((health & 3U) << 6) | ((mode & 7U) << 3)); // sub_mode=0
  f.data[5] = 0; // vendor_specific_status_code
  f.data[6] = 0;
  f.data[7] = (uint8_t)(0xC0U | (tid & 0x1FU)); // SOT|EOT, toggle=0
  tid = (tid + 1) & 0x1FU;
}
