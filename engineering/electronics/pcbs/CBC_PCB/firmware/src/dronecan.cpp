#include "dronecan.h"
#include <string.h>

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

// UAVCAN v0 packs sub-byte fields MSB-first within the bit stream;
// byte-aligned multi-byte scalars are little-endian.
static uint32_t getBitsMsbFirst(const uint8_t *buf, uint32_t bitOff, uint8_t nBits) {
  uint32_t v = 0;
  for (uint8_t i = 0; i < nBits; i++) {
    uint32_t b = bitOff + i;
    v = (v << 1) | ((buf[b >> 3] >> (7U - (b & 7U))) & 1U);
  }
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
        reasm.tid != tid || tog != (reasm.toggle ^ 1U)) {
      reasm.active = false;
      return false;
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
  } else if (dtid == DTID_TATTU_BATTERY && len >= 12) {
    // Tattu vendor broadcast — layout proven by the Quiver RPi tattu bridge.
    // Transfer CRC not validated (vendor DSDL signature unknown).
    BatteryTelemetry &b = battery;
    b.valid = true;
    b.crc_ok = true; // unvalidated
    b.last_update_ms = now_ms;
    b.source_node = src;
    b.protocol = 2;
    b.tattu_vendor = le16(p + 0);
    b.model_instance_id = le16(p + 2);
    b.voltage = le16(p + 4) / 1000.0f;                 // mV
    b.current = (int16_t)le16(p + 6) / 100.0f;         // 10 mA units
    b.temperature_k = (int16_t)le16(p + 8) + 273.15f;  // degC
    uint16_t soc = le16(p + 10);
    b.soc_pct = (uint8_t)(soc > 100 ? 100 : soc);
    strncpy(b.model_name, "Tattu(0x1092)", sizeof(b.model_name) - 1);
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
    // starting at bit 112 (byte 14). Verify against a real battery — sub-byte
    // packing order is the usual v0 MSB-first convention.
    b.status_flags = (uint16_t)getBitsMsbFirst(p, 112, 11);
    b.soh_pct = (uint8_t)getBitsMsbFirst(p, 123, 7);
    b.soc_pct = (uint8_t)getBitsMsbFirst(p, 130, 7);
    b.soc_stdev = (uint8_t)getBitsMsbFirst(p, 137, 7);
    b.battery_id = p[18];
    b.model_instance_id = le32(p + 19);
    uint16_t nameLen = len - 23;
    if (nameLen > 32) nameLen = 32;
    memcpy(b.model_name, p + 23, nameLen);
    b.model_name[nameLen] = 0;
  }
}

// ---------- TX ----------

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
