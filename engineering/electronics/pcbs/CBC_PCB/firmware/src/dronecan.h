// Minimal DroneCAN (UAVCAN v0) receive/transmit helpers for the CBC.
//
// Scope: enough to (a) decode uavcan.protocol.NodeStatus and
// uavcan.equipment.power.BatteryInfo broadcast by a smart battery, and
// (b) broadcast our own NodeStatus so the CBC shows up on the drone bus.
// Not a full libcanard port — no services, no dynamic node allocation.
#pragma once

#include <stdint.h>
#include <can.h> // struct can_frame (ships with autowp-mcp2515)

// DroneCAN data type IDs (message transfers)
#define DTID_NODE_STATUS 341
#define DTID_BATTERY_INFO 1092

// Tattu "E-UAVCAN" vendor broadcast (TATTU-2177 spec, confirmed against a
// real Grepow/Tattu 18S 30Ah pack on the CBC, 2026-07-21):
// ext ID 0x01109216 = priority 1, data type 0x1092 (4242), source node 0x16.
// 4 Hz, little-endian, multi-frame v0-style transfer with two quirks:
//   * transfer ID increments on EVERY frame (not constant per transfer)
//   * transfer CRC uses an unknown seed — we don't validate it
// Payload (76-byte variant; a 60-byte variant without serial alternates):
//   ofs 0  i16 manufacturer      ofs 2  i16 model
//   ofs 4  u16 voltage [10mV]    ofs 6  i16 current [10mA] (+ = charging)
//   ofs 8  i16 temperature [C]   ofs 10 u16 soc [%]
//   ofs 12 u16 cycle count       ofs 14 i16 health [%]
//   ofs 16 u16 cell_mv[18]       ofs 52 u16 design capacity [mAh]
//   ofs 54 u16 remaining [mAh]   ofs 56 u32 error bitfield
//   ofs 60 char serial[16]       (76-byte variant only)
#define DTID_TATTU_BATTERY 0x1092

// DSDL 64-bit data type signature for uavcan.equipment.power.BatteryInfo.
// Used to seed the transfer CRC of multi-frame transfers. On CRC mismatch we
// still decode but mark crc_ok=false (verify against a real battery).
#define SIG_BATTERY_INFO 0x249C26548A711966ULL

struct BatteryTelemetry {
  bool valid = false;
  bool crc_ok = false;
  uint32_t last_update_ms = 0;
  uint8_t source_node = 0;
  uint8_t protocol = 0;      // 0 none, 1 = BatteryInfo (1092), 2 = Tattu vendor (0x1092)
  uint16_t tattu_vendor = 0; // Tattu vendor field (protocol 2 only)
  // uavcan.equipment.power.BatteryInfo fields
  float temperature_k = 0;
  float voltage = 0;
  float current = 0;
  float average_power_10sec = 0;
  float remaining_capacity_wh = 0;
  float full_charge_capacity_wh = 0;
  float hours_to_full_charge = 0;
  uint16_t status_flags = 0; // uint11
  uint8_t soh_pct = 0;       // uint7
  uint8_t soc_pct = 0;       // uint7
  uint8_t soc_stdev = 0;     // uint7
  uint8_t battery_id = 0;
  uint32_t model_instance_id = 0;
  char model_name[33] = {0};
  // Tattu extras (protocol 2 only)
  uint8_t cell_count = 0;
  uint16_t cells_mv[18] = {0};
  uint16_t cycles = 0;
  uint16_t design_mah = 0;
  uint16_t remaining_mah = 0;
  uint32_t error_flags = 0; // Tattu error bitfield (see spec comment above)
  char serial[17] = {0};
};

struct NodeStatusTelemetry {
  bool valid = false;
  uint32_t last_update_ms = 0;
  uint8_t source_node = 0;
  uint32_t uptime_sec = 0;
  uint8_t health = 0; // 0 OK, 1 WARNING, 2 ERROR, 3 CRITICAL
  uint8_t mode = 0;   // 0 OPERATIONAL ... 7 OFFLINE
  uint8_t sub_mode = 0;
  uint16_t vendor_code = 0;
};

class DroneCanRx {
public:
  // Feed every received extended frame from one bus.
  // Returns true if the frame parsed as a (fragment of a) DroneCAN transfer.
  bool handleFrame(const struct can_frame &f, uint32_t now_ms);

  BatteryTelemetry battery;
  NodeStatusTelemetry nodeStatus; // most recent NodeStatus from any node

  uint32_t transfers_ok = 0;
  uint32_t crc_errors = 0;
  uint32_t frames_non_dronecan = 0;

private:
  struct {
    bool active = false;
    uint8_t src = 0, tid = 0, toggle = 0;
    uint16_t dtid = 0;
    uint16_t len = 0;
    uint8_t buf[128];
  } reasm;

  void completeTransfer(uint8_t src, uint16_t dtid, const uint8_t *payload,
                        uint16_t len, bool crc_ok, uint32_t now_ms);
};

// Build a NodeStatus broadcast frame (single-frame transfer, 7-byte payload).
// tid is incremented internally on each call.
void dronecanMakeNodeStatus(struct can_frame &f, uint8_t node_id,
                            uint32_t uptime_sec, uint8_t health, uint8_t mode);

float float16ToFloat(uint16_t h);
uint16_t floatToFloat16(float f);

// Encode BatteryTelemetry as uavcan.equipment.power.BatteryInfo into out
// (>= 64 bytes). Returns encoded length. Note: BatteryInfo current is
// positive-discharging (ArduPilot convention), Tattu is positive-charging;
// the encoder negates accordingly.
uint16_t dronecanEncodeBatteryInfo(uint8_t *out, const BatteryTelemetry &b);

// ---------------------------------------------------------------------------
// DroneCanNode — minimal v0 node for the drone bus:
//  * dynamic node ID allocation client (uavcan.protocol.dynamic_node_id.
//    Allocation, dtid 1) — the "handshake with the FC" that assigns each CBC
//    a unique node ID when several sit on one bus. ArduPilot runs the
//    allocation server by default (CAN_Dn_UC_OPTION / DNA server).
//  * NodeStatus broadcast @1Hz once allocated.
//  * uavcan.protocol.GetNodeInfo (service 1) responder, so the FC can
//    identify the node ("org.arrowair.cbc") and persist the allocation.
// ---------------------------------------------------------------------------

#define DTID_ALLOCATION 1
#define SRVID_GET_NODE_INFO 1
#define SIG_GET_NODE_INFO 0xEE468A8121C46A9EULL

// Standard configuration services (what the DroneCAN GUI tool's node
// properties panel speaks): parameter get/set, save-to-flash, restart.
#define SRVID_PARAM_GETSET 11
#define SIG_PARAM_GETSET 0xA7B622F939D1A4D5ULL
#define SRVID_PARAM_EXECUTEOPCODE 10
#define SIG_PARAM_EXECUTEOPCODE 0x3B131AC5EB69D2CDULL
#define SRVID_RESTART_NODE 5
#define SIG_RESTART_NODE 0x569E05394A3017F0ULL
#define RESTART_NODE_MAGIC 0xACCE551B1EULL

// A configuration parameter exposed over uavcan.protocol.param.GetSet
// (integer type only — enough for NODE_ID / BATT_ID).
struct DroneCanParam {
  const char *name;
  int64_t value;
  int64_t defval, minval, maxval;
};

class DroneCanNode {
public:
  typedef bool (*SendFn)(const struct can_frame &f);

  // unique_id: 16 bytes, must be stable per board (we derive it from the
  // ESP32 eFuse MAC). static_node_id != 0 skips allocation entirely.
  void begin(SendFn send, const uint8_t unique_id[16], uint8_t static_node_id);
  void poll(uint32_t now_ms);
  void handleFrame(const struct can_frame &f, uint32_t now_ms);

  bool allocated() const { return node_id_ != 0; }
  uint8_t nodeId() const { return node_id_; }
  void setHealth(uint8_t h) { health_ = h; }

  // Broadcast an arbitrary message transfer (single- or multi-frame) on the
  // drone bus. No-op until allocated. Returns false if not allocated.
  bool broadcast(uint16_t dtid, uint64_t signature, const uint8_t *payload,
                 uint16_t len);

  // Expose config params over uavcan.protocol.param.GetSet. onSave is called
  // for ExecuteOpcode SAVE (persist to flash), onErase for ERASE (the node
  // resets values to defaults first, then calls onErase to persist).
  // onRestart is called after a valid RestartNode request has been answered.
  void setParams(DroneCanParam *params, uint8_t count) {
    params_ = params;
    nparams_ = count;
  }
  void setConfigHandlers(void (*onSave)(), void (*onErase)(),
                         void (*onRestart)()) {
    save_cb_ = onSave;
    erase_cb_ = onErase;
    restart_cb_ = onRestart;
  }

private:
  void sendAllocationRequest(uint32_t now_ms);
  void handleAllocationBroadcast(const uint8_t *payload, uint16_t len,
                                 uint32_t now_ms);
  void sendGetNodeInfoResponse(uint8_t dest, uint8_t tid);
  void handleServiceRequest(uint8_t src, uint8_t stype, uint8_t tid,
                            const uint8_t *payload, uint16_t len);
  void handleParamGetSet(uint8_t dest, uint8_t tid, const uint8_t *payload,
                         uint16_t len);
  void sendParamGetSetResponse(uint8_t dest, uint8_t tid, int paramIdx);
  void sendMultiFrame(uint32_t can_id, const uint8_t *payload, uint16_t len,
                      uint64_t signature, uint8_t tid);

  SendFn send_ = nullptr;
  uint8_t unique_id_[16] = {0};
  uint8_t node_id_ = 0;
  uint8_t health_ = 0;
  uint8_t confirmed_uid_bytes_ = 0; // unique-id bytes echoed back by the server
  uint8_t bcast_tid_ = 0;
  uint32_t next_alloc_request_ms_ = 0;
  uint32_t last_server_echo_ms_ = 0;
  uint32_t last_node_status_ms_ = 0;

  DroneCanParam *params_ = nullptr;
  uint8_t nparams_ = 0;
  void (*save_cb_)() = nullptr;
  void (*erase_cb_)() = nullptr;
  void (*restart_cb_)() = nullptr;

  // tiny reassembler for multi-frame Allocation broadcasts from the server
  struct {
    bool active = false;
    uint8_t src = 0, tid = 0, toggle = 0;
    uint16_t len = 0;
    uint8_t buf[32];
  } alloc_reasm_;

  // reassembler for multi-frame service requests addressed to us
  // (e.g. param.GetSet set-requests, which exceed one frame)
  struct {
    bool active = false;
    uint8_t src = 0, stype = 0, tid = 0, toggle = 0;
    uint16_t len = 0;
    uint8_t buf[128];
  } srv_reasm_;
};
