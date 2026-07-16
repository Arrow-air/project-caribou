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

// DSDL 64-bit data type signature for uavcan.equipment.power.BatteryInfo.
// Used to seed the transfer CRC of multi-frame transfers. On CRC mismatch we
// still decode but mark crc_ok=false (verify against a real battery).
#define SIG_BATTERY_INFO 0x249C26548A711966ULL

struct BatteryTelemetry {
  bool valid = false;
  bool crc_ok = false;
  uint32_t last_update_ms = 0;
  uint8_t source_node = 0;
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
