// CBC_PCB firmware v0.3.0
// ESP32-S3-WROOM-1-N16R8 — Caribou Battery Connector board
//
// Functions:
//  - Main power switch control: precharge -> latch arm sequence (see README:
//    firmware can ARM the latch but can NOT disarm it — hardware property).
//  - Kill trigger monitoring (12V_TRIGGER_SIG via Schmitt buffer on IO5).
//  - CAN1 (battery bus, MCP2515 U402 @16MHz): decodes the Tattu 18S
//    "E-UAVCAN" broadcast (protocol confirmed on hardware, Jul 2026 —
//    voltage/current/SOC/SOH/cycles/18 cell voltages/capacity/errors/serial).
//  - CAN2 (drone bus, MCP2515 U401 @16MHz, isolated): DroneCAN node with
//    dynamic node allocation, NodeStatus @1Hz, GetNodeInfo, and a battery
//    bridge that republishes Tattu telemetry as standard
//    uavcan.equipment.power.BatteryInfo @2Hz so the FC sees the pack.
//  - 2x DS18B20 board temperature sensors.
//  - USB serial console (115200 baud via CP2102N) for status + commands.
//  - Config over CAN: NODE_ID / BATT_ID exposed via the standard DroneCAN
//    parameter services (GetSet / SAVE / RestartNode), persisted in ESP32
//    flash (NVS). Editable from the DroneCAN GUI tool — no USB needed.

#include <Arduino.h>
#include <SPI.h>
#include <mcp2515.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

#include "pins.h"
#include "dronecan.h"

#define FW_VERSION "0.3.1"

// ---------------- configuration ----------------

// Arm the main switch automatically after boot (field operation without USB).
// Set to 0 to require the "arm" serial command instead.
#define AUTO_ARM_ON_BOOT 1
#define AUTO_ARM_DELAY_MS 3000

// Precharge sequence (docs/power-architecture.md: precharge on, wait
// 500ms..2s, main switch on, precharge off shortly after).
#define PRECHARGE_TIME_MS 1000
#define PRECHARGE_OVERLAP_MS 250
#define LATCH_PULSE_MS 50

// DroneCAN node ID on the drone bus. This is only the factory default for
// the NODE_ID parameter — the live value is stored in NVS and adjustable
// over CAN via the DroneCAN GUI tool (parameters NODE_ID / BATT_ID).
// 0 = request an ID from the FC via dynamic node allocation (the standard
// DroneCAN "handshake" — ArduPilot runs the allocation server by default),
// which is what you want with 6 CBCs on one bus. 1..125 pins a static ID.
#define CBC_DEFAULT_NODE_ID 0

// Default bitrates. DroneCAN convention is 1 Mbps; if the Tattu speaks its
// proprietary protocol it may use something else — use "scan" to find out.
#define DEFAULT_BAT_BITRATE CAN_1000KBPS
#define DEFAULT_DRONE_BITRATE CAN_1000KBPS

#define STATUS_PERIOD_MS 2000
#define RAW_DUMP_MAX_PER_SEC 20

// ---------------- globals ----------------

// NOTE: constructed in setup(), NOT as globals. The library's default
// constructor calls SPI.begin() at static-init time (before the Arduino core
// is initialized) — on core 3.x that leaves the SPI bus broken and the first
// transfer hangs forever. Passing &SPI explicitly skips that begin() call.
static MCP2515 *canBat = nullptr;
static MCP2515 *canDrone = nullptr;

OneWire owTemp1(PIN_TEMP1);
OneWire owTemp2(PIN_TEMP2);
DallasTemperature temp1(&owTemp1);
DallasTemperature temp2(&owTemp2);

DroneCanRx batRx;   // battery bus decoder
DroneCanRx droneRx; // drone bus decoder (see who else is talking)
DroneCanNode droneNode; // our node on the drone bus (allocation + NodeStatus)

// ---- config parameters (DroneCAN GetSet + NVS persistence) ----
// NODE_ID: 0 = dynamic allocation from the FC, 1..125 = static node ID.
//          Takes effect after save + restart (GUI tool: Fetch/Store/Restart).
// BATT_ID: battery_id field in the bridged BatteryInfo (per-pack tag so the
//          FC can tell the 6 CBCs apart; matches ArduPilot BATTx_SERIAL_NUM).
Preferences prefs;
static DroneCanParam cbcParams[] = {
        {"NODE_ID", CBC_DEFAULT_NODE_ID, CBC_DEFAULT_NODE_ID, 0, 125},
        {"BATT_ID", 0, 0, 0, 255},
};

static void paramsSave() {
    prefs.putUChar("node_id", (uint8_t)cbcParams[0].value);
    prefs.putUChar("batt_id", (uint8_t)cbcParams[1].value);
    Serial.printf("[param] saved to NVS: NODE_ID=%d BATT_ID=%d (node ID change applies after restart)\n",
                                (int)cbcParams[0].value, (int)cbcParams[1].value);
}

static void paramsErase() {
    paramsSave(); // values already reset to defaults by the node
    Serial.println("[param] reset to defaults");
}

static void nodeRestart() {
    Serial.println("[dronecan] RestartNode request accepted — rebooting");
    Serial.flush();
    delay(100); // let the response frame drain out of the MCP2515
    esp_restart();
}

static bool droneSend(const struct can_frame &f) {
    // Multi-frame transfers (e.g. the 9-frame GetNodeInfo response) can outrun
    // the MCP2515's 3 TX buffers — wait briefly for a free one (~128us/frame @1M).
    for (int i = 0; i < 40; i++) {
        if (canDrone->sendMessage(&f) == MCP2515::ERROR_OK) return true;
        delayMicroseconds(50);
    }
    return false;
}

enum SwitchState { SW_OFF, SW_PRECHARGE, SW_LATCH_PULSE, SW_OVERLAP, SW_ON };
static SwitchState swState = SW_OFF;
static uint32_t swStateSince = 0;
static bool killActive = false;
static bool autoArmPending = AUTO_ARM_ON_BOOT;

static bool rawBat = false; // protocol confirmed; 'raw bat on' to dump frames
static bool rawDrone = false;
static uint32_t batFramesRx = 0, droneFramesRx = 0;
static float tempC1 = NAN, tempC2 = NAN;

static char cmdBuf[64];
static uint8_t cmdLen = 0;

// ---------------- helpers ----------------

static const char *bitrateName(CAN_SPEED s) {
    switch (s) {
        case CAN_125KBPS: return "125k";
        case CAN_250KBPS: return "250k";
        case CAN_500KBPS: return "500k";
        case CAN_1000KBPS: return "1000k";
        default: return "?";
    }
}

static CAN_SPEED batBitrate = DEFAULT_BAT_BITRATE;
static CAN_SPEED droneBitrate = DEFAULT_DRONE_BITRATE;

static void initCan(MCP2515 &dev, int rstPin, CAN_SPEED speed, bool listenOnly,
                                        const char *name) {
    Serial.printf("[can] %s init (rst pin %d)...\n", name, rstPin);
    digitalWrite(rstPin, LOW);
    delay(10);
    digitalWrite(rstPin, HIGH);
    delay(10);
    MCP2515::ERROR e1 = dev.reset();
    MCP2515::ERROR e2 = dev.setBitrate(speed, MCP_16MHZ); // X401/X402 = 16 MHz
    MCP2515::ERROR e3 = listenOnly ? dev.setListenOnlyMode() : dev.setNormalMode();
    Serial.printf("[can] %s init: reset=%d bitrate(%s)=%d mode=%d\n", name, e1,
                                bitrateName(speed), e2, e3);
}

static void printFrame(const char *bus, const struct can_frame &f) {
    Serial.printf("[%s] %s 0x%08lX dlc=%u :", bus,
                                (f.can_id & CAN_EFF_FLAG) ? "ext" : "std",
                                (unsigned long)(f.can_id & (f.can_id & CAN_EFF_FLAG ? CAN_EFF_MASK : CAN_SFF_MASK)),
                                f.can_dlc);
    for (uint8_t i = 0; i < f.can_dlc; i++) Serial.printf(" %02X", f.data[i]);
    Serial.println();
}

static const char *swStateName() {
    switch (swState) {
        case SW_OFF: return "OFF";
        case SW_PRECHARGE: return "PRECHARGE";
        case SW_LATCH_PULSE: return "ARMING";
        case SW_OVERLAP: return "ON(precharge overlap)";
        case SW_ON: return "ON(latched)";
    }
    return "?";
}

// ---------------- switch control ----------------

static void startArmSequence(const char *reason) {
    if (swState != SW_OFF) {
        Serial.printf("[sw] arm ignored (state=%s)\n", swStateName());
        return;
    }
    if (killActive) {
        Serial.println("[sw] arm blocked: kill trigger active");
        return;
    }
    Serial.printf("[sw] arm sequence started (%s): precharge %dms -> latch -> overlap %dms\n",
                                reason, PRECHARGE_TIME_MS, PRECHARGE_OVERLAP_MS);
    digitalWrite(PIN_PRECHARGE, HIGH);
    swState = SW_PRECHARGE;
    swStateSince = millis();
}

static void abortArmSequence(const char *why) {
    digitalWrite(PIN_PRECHARGE, LOW);
    digitalWrite(PIN_LATCH_SET, LOW);
    Serial.printf("[sw] arm sequence ABORTED: %s\n", why);
    swState = SW_OFF;
    swStateSince = millis();
}

static void updateSwitchStateMachine() {
    uint32_t now = millis();
    switch (swState) {
        case SW_OFF:
        case SW_ON:
            break;
        case SW_PRECHARGE:
            if (killActive) { abortArmSequence("kill trigger during precharge"); break; }
            if (now - swStateSince >= PRECHARGE_TIME_MS) {
                digitalWrite(PIN_LATCH_SET, HIGH); // set the NOR latch -> main FETs on
                swState = SW_LATCH_PULSE;
                swStateSince = now;
            }
            break;
        case SW_LATCH_PULSE:
            if (now - swStateSince >= LATCH_PULSE_MS) {
                digitalWrite(PIN_LATCH_SET, LOW); // latch holds state on its own
                swState = SW_OVERLAP;
                swStateSince = now;
            }
            break;
        case SW_OVERLAP:
            if (now - swStateSince >= PRECHARGE_OVERLAP_MS) {
                digitalWrite(PIN_PRECHARGE, LOW);
                swState = SW_ON;
                swStateSince = now;
                Serial.println("[sw] main switch LATCHED ON (only a battery power-cycle can reset it)");
            }
            break;
    }
}

static void updateKillSense() {
    bool k = digitalRead(PIN_KILL_SENSE);
    if (k != killActive) {
        killActive = k;
        if (killActive) {
            Serial.println("[kill] 12V trigger ASSERTED -> SSR opens gate path, main FETs OFF (hardware)");
            if (swState == SW_PRECHARGE || swState == SW_LATCH_PULSE || swState == SW_OVERLAP)
                abortArmSequence("kill trigger");
        } else {
            Serial.println("[kill] 12V trigger RELEASED");
            if (swState == SW_ON)
                Serial.println("[kill] WARNING: latch is still set -> main FETs are BACK ON now");
        }
    }
}

// ---------------- battery bitrate scan ----------------

static void scanBatteryBitrate() {
    const CAN_SPEED rates[] = {CAN_1000KBPS, CAN_500KBPS, CAN_250KBPS, CAN_125KBPS};
    Serial.println("[scan] listening 3s on each bitrate (listen-only)...");
    CAN_SPEED best = batBitrate;
    uint32_t bestCount = 0;
    for (auto r : rates) {
        initCan(*canBat, PIN_CAN_BAT_RST, r, true, "bat");
        uint32_t count = 0, t0 = millis();
        struct can_frame f;
        while (millis() - t0 < 3000) {
            if (canBat->readMessage(&f) == MCP2515::ERROR_OK) count++;
            yield();
        }
        Serial.printf("[scan] %s: %lu frames\n", bitrateName(r), (unsigned long)count);
        if (count > bestCount) { bestCount = count; best = r; }
    }
    batBitrate = best;
    Serial.printf("[scan] selecting %s, normal mode\n", bitrateName(best));
    initCan(*canBat, PIN_CAN_BAT_RST, batBitrate, false, "bat");
}

// ---------------- battery -> drone bus bridge ----------------

// Republish the decoded Tattu telemetry on the drone bus as standard
// uavcan.equipment.power.BatteryInfo @2Hz. ArduPilot picks this up with
// BATT_MONITOR=8 (DroneCAN). Stops if telemetry goes stale (>5s).
#define BRIDGE_PERIOD_MS 500
#define BRIDGE_STALE_MS 5000

static void bridgeProcess() {
    static uint32_t lastBridge = 0;
    uint32_t now = millis();
    if (now - lastBridge < BRIDGE_PERIOD_MS) return;
    if (!droneNode.allocated()) return;
    BatteryTelemetry &b = batRx.battery;
    if (!b.valid || now - b.last_update_ms > BRIDGE_STALE_MS) return;
    lastBridge = now;
    BatteryTelemetry tagged = b;
    tagged.battery_id = (uint8_t)cbcParams[1].value; // BATT_ID param (per-pack)
    // Forward the hottest of the three temperatures we know — the pack's own
    // internal sensor and the two DS18B20s on the CBC board — so the FC sees
    // whichever part of the battery path is running warmest.
    float tC = (tagged.temperature_k > 0.0f) ? (tagged.temperature_k - 273.15f) : NAN;
    if (!isnan(tempC1) && (isnan(tC) || tempC1 > tC)) tC = tempC1;
    if (!isnan(tempC2) && (isnan(tC) || tempC2 > tC)) tC = tempC2;
    if (!isnan(tC)) tagged.temperature_k = tC + 273.15f;
    uint8_t buf[64];
    uint16_t len = dronecanEncodeBatteryInfo(buf, tagged);
    droneNode.broadcast(DTID_BATTERY_INFO, SIG_BATTERY_INFO, buf, len);
}

// ---------------- status / console ----------------

static void printStatus() {
    Serial.printf("[status] fw=%s up=%lus sw=%s kill=%s t1=%.1fC t2=%.1fC | bat bus %s rx=%lu dc_ok=%lu crc_err=%lu | drone bus %s rx=%lu\n",
                                FW_VERSION, (unsigned long)(millis() / 1000), swStateName(),
                                killActive ? "ACTIVE" : "clear", tempC1, tempC2,
                                bitrateName(batBitrate), (unsigned long)batFramesRx,
                                (unsigned long)batRx.transfers_ok, (unsigned long)batRx.crc_errors,
                                bitrateName(droneBitrate), (unsigned long)droneFramesRx);
    Serial.printf("[dronecan] drone bus node: %s\n",
                                droneNode.allocated()
                                        ? (String("ID ") + droneNode.nodeId()).c_str()
                                        : "waiting for allocation from FC");
    if (batRx.battery.valid) {
        BatteryTelemetry &b = batRx.battery;
        Serial.printf("[battery] %s node=%u '%s' %.2fV %.2fA soc=%u%% soh=%u%% temp=%.0fC%s (age %lus)\n",
                                    b.protocol == 2 ? "tattu" : "dronecan",
                                    b.source_node, b.model_name, b.voltage, b.current, b.soc_pct,
                                    b.soh_pct,
                                    b.temperature_k > 0 ? b.temperature_k - 273.15f : 0.0f,
                                    b.crc_ok ? "" : " CRC?",
                                    (unsigned long)((millis() - b.last_update_ms) / 1000));
        if (b.protocol == 2 && b.cell_count) {
            uint16_t cmin = 0xFFFF, cmax = 0;
            for (uint8_t i = 0; i < b.cell_count; i++) {
                if (b.cells_mv[i] < cmin) cmin = b.cells_mv[i];
                if (b.cells_mv[i] > cmax) cmax = b.cells_mv[i];
            }
            Serial.printf("[battery] cells %u: %u..%umV (spread %u) | %u/%umAh cycles=%u err=0x%08lX serial='%s'\n",
                                        b.cell_count, cmin, cmax, (unsigned)(cmax - cmin),
                                        b.remaining_mah, b.design_mah, b.cycles,
                                        (unsigned long)b.error_flags, b.serial);
        }
    } else {
        Serial.println("[battery] no battery telemetry decoded yet");
    }
    if (batRx.nodeStatus.valid) {
        NodeStatusTelemetry &n = batRx.nodeStatus;
        Serial.printf("[battery] NodeStatus: node=%u up=%lus health=%u mode=%u\n",
                                    n.source_node, (unsigned long)n.uptime_sec, n.health, n.mode);
    }
}

static void printHelp() {
    Serial.println(
            "commands:\n"
            "  status              print full status now\n"
            "  arm                 run precharge + latch-on sequence\n"
            "  precharge on|off    drive precharge FET manually\n"
            "  raw bat on|off      dump raw battery bus frames (default off)\n"
            "  raw drone on|off    dump raw drone bus frames\n"
            "  bitrate bat|drone 125|250|500|1000\n"
            "  scan                auto-detect battery bus bitrate\n"
            "  help                this text\n"
            "note: there is NO 'off' command — the hardware latch cannot be cleared\n"
            "      by firmware; power-cycle the battery or use the 12V kill trigger.");
}

static void handleCommand(char *cmd) {
    if (!strcmp(cmd, "help")) { printHelp(); return; }
    if (!strcmp(cmd, "status")) { printStatus(); return; }
    if (!strcmp(cmd, "arm")) { autoArmPending = false; startArmSequence("serial command"); return; }
    if (!strcmp(cmd, "precharge on")) { digitalWrite(PIN_PRECHARGE, HIGH); Serial.println("[sw] precharge ON (manual)"); return; }
    if (!strcmp(cmd, "precharge off")) { digitalWrite(PIN_PRECHARGE, LOW); Serial.println("[sw] precharge OFF (manual)"); return; }
    if (!strcmp(cmd, "raw bat on")) { rawBat = true; return; }
    if (!strcmp(cmd, "raw bat off")) { rawBat = false; return; }
    if (!strcmp(cmd, "raw drone on")) { rawDrone = true; return; }
    if (!strcmp(cmd, "raw drone off")) { rawDrone = false; return; }
    if (!strcmp(cmd, "scan")) { scanBatteryBitrate(); return; }
    if (!strncmp(cmd, "bitrate ", 8)) {
        char *rest = cmd + 8;
        bool isBat = !strncmp(rest, "bat ", 4);
        bool isDrone = !strncmp(rest, "drone ", 6);
        int val = atoi(strchr(rest, ' ') ? strchr(rest, ' ') + 1 : "");
        CAN_SPEED s;
        switch (val) {
            case 125: s = CAN_125KBPS; break;
            case 250: s = CAN_250KBPS; break;
            case 500: s = CAN_500KBPS; break;
            case 1000: s = CAN_1000KBPS; break;
            default: Serial.println("bad bitrate (125|250|500|1000)"); return;
        }
        if (isBat) { batBitrate = s; initCan(*canBat, PIN_CAN_BAT_RST, s, false, "bat"); }
        else if (isDrone) { droneBitrate = s; initCan(*canDrone, PIN_CAN_DRONE_RST, s, false, "drone"); }
        else Serial.println("usage: bitrate bat|drone <kbps>");
        return;
    }
    Serial.printf("unknown command '%s' — try 'help'\n", cmd);
}

static void pollConsole() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            cmdBuf[cmdLen] = 0;
            if (cmdLen) handleCommand(cmdBuf);
            cmdLen = 0;
        } else if (cmdLen < sizeof(cmdBuf) - 1) {
            cmdBuf[cmdLen++] = c;
        }
    }
}

// ---------------- setup / loop ----------------

void setup() {
    // Safe-state GPIO first: latch-set and precharge low, CS lines high so the
    // two MCP2515s don't both listen while SPI initializes.
    pinMode(PIN_LATCH_SET, OUTPUT);
    digitalWrite(PIN_LATCH_SET, LOW);
    pinMode(PIN_PRECHARGE, OUTPUT);
    digitalWrite(PIN_PRECHARGE, LOW);
    pinMode(PIN_KILL_SENSE, INPUT);
    pinMode(PIN_CAN_BAT_CS, OUTPUT);
    digitalWrite(PIN_CAN_BAT_CS, HIGH);
    pinMode(PIN_CAN_DRONE_CS, OUTPUT);
    digitalWrite(PIN_CAN_DRONE_CS, HIGH);
    pinMode(PIN_CAN_BAT_RST, OUTPUT);
    digitalWrite(PIN_CAN_BAT_RST, HIGH);
    pinMode(PIN_CAN_DRONE_RST, OUTPUT);
    digitalWrite(PIN_CAN_DRONE_RST, HIGH);
    pinMode(PIN_CAN_BAT_INT, INPUT_PULLUP);   // MCP2515 INT is open-drain
    pinMode(PIN_CAN_DRONE_INT, INPUT_PULLUP); // (per schematic-review-followup)

    Serial.begin(115200);
    delay(200);
    Serial.printf("\n=== CBC_PCB firmware v%s (ESP32-S3-WROOM-1-N16R8) ===\n", FW_VERSION);
    Serial.println("[sw] main switch state: OFF (hardware POR reset)");

    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, -1);

    // Construct AFTER SPI.begin; passing &SPI keeps the ctor from re-running
    // SPI.begin() itself (see note at the declarations above).
    canBat = new MCP2515(PIN_CAN_BAT_CS, 10000000, &SPI);
    canDrone = new MCP2515(PIN_CAN_DRONE_CS, 10000000, &SPI);

    initCan(*canBat, PIN_CAN_BAT_RST, batBitrate, false, "bat");
    initCan(*canDrone, PIN_CAN_DRONE_RST, droneBitrate, false, "drone");

    temp1.begin();
    temp2.begin();
    temp1.setWaitForConversion(false);
    temp2.setWaitForConversion(false);
    temp1.requestTemperatures();
    temp2.requestTemperatures();

    // Config parameters from NVS (set over CAN via the DroneCAN GUI tool)
    prefs.begin("cbc");
    cbcParams[0].value = prefs.getUChar("node_id", CBC_DEFAULT_NODE_ID);
    cbcParams[1].value = prefs.getUChar("batt_id", 0);
    Serial.printf("[param] NODE_ID=%d BATT_ID=%d (from NVS; edit via DroneCAN GUI tool)\n",
                                (int)cbcParams[0].value, (int)cbcParams[1].value);

    // Drone-bus DroneCAN node: unique ID from the eFuse MAC (stable per board)
    uint8_t uid[16] = {0};
    memcpy(uid, "ARW-CBC1", 8);
    uint64_t mac = ESP.getEfuseMac();
    memcpy(uid + 8, &mac, 8);
    uint8_t staticId = (uint8_t)cbcParams[0].value;
    droneNode.begin(droneSend, uid, staticId);
    droneNode.setParams(cbcParams, 2);
    droneNode.setConfigHandlers(paramsSave, paramsErase, nodeRestart);
    if (staticId == 0)
        Serial.println("[dronecan] requesting node ID from the FC (dynamic allocation)");
    else
        Serial.printf("[dronecan] static node ID %u\n", staticId);

    killActive = digitalRead(PIN_KILL_SENSE);
    if (killActive) Serial.println("[kill] trigger active at boot — auto-arm held off");

#if AUTO_ARM_ON_BOOT
    Serial.printf("[sw] auto-arm in %dms (send any command to inspect first)\n", AUTO_ARM_DELAY_MS);
#else
    Serial.println("[sw] auto-arm disabled — send 'arm' to close the main switch");
#endif
    printHelp();
}

void loop() {
    uint32_t now = millis();

    updateKillSense();
    updateSwitchStateMachine();

    if (autoArmPending && now >= AUTO_ARM_DELAY_MS && !killActive && swState == SW_OFF) {
        autoArmPending = false;
        startArmSequence("auto-arm at boot");
    }

    // --- battery bus RX ---
    static uint32_t rawWindowStart = 0;
    static uint8_t rawCount = 0;
    struct can_frame f;
    while (canBat->readMessage(&f) == MCP2515::ERROR_OK) {
        batFramesRx++;
        batRx.handleFrame(f, now);
        if (rawBat) {
            if (now - rawWindowStart >= 1000) { rawWindowStart = now; rawCount = 0; }
            if (rawCount < RAW_DUMP_MAX_PER_SEC) { printFrame("bat", f); rawCount++; }
        }
    }

    // --- drone bus RX ---
    while (canDrone->readMessage(&f) == MCP2515::ERROR_OK) {
        droneFramesRx++;
        droneRx.handleFrame(f, now);
        droneNode.handleFrame(f, now); // allocation echoes, GetNodeInfo requests
        if (rawDrone) printFrame("drone", f);
    }

    // --- drone bus node housekeeping (allocation handshake / NodeStatus @1Hz) ---
    static bool wasAllocated = false;
    droneNode.setHealth(killActive ? 1 /*WARNING*/ : 0 /*OK*/);
    droneNode.poll(now);
    if (!wasAllocated && droneNode.allocated()) {
        wasAllocated = true;
        Serial.printf("[dronecan] node ID %u allocated by the FC\n", droneNode.nodeId());
    }

    bridgeProcess();

    // --- temperature, ~every 2s ---
    static uint32_t lastTempReq = 0;
    if (now - lastTempReq >= 2000) {
        lastTempReq = now;
        float t = temp1.getTempCByIndex(0);
        tempC1 = (t > DEVICE_DISCONNECTED_C) ? t : NAN;
        t = temp2.getTempCByIndex(0);
        tempC2 = (t > DEVICE_DISCONNECTED_C) ? t : NAN;
        temp1.requestTemperatures();
        temp2.requestTemperatures();
    }

    // --- periodic status ---
    static uint32_t lastStatus = 0;
    if (now - lastStatus >= STATUS_PERIOD_MS) {
        lastStatus = now;
        printStatus();
    }

    pollConsole();
}
