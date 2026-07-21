# CBC_PCB Firmware

Firmware for the ESP32-S3-WROOM-1-N16R8 (U301) on the Caribou Battery Connector
board (CBC_PCB, ordered board state / PR #51).

**Status: v0.3.0 — config over CAN (2026-07-21):** `NODE_ID` and `BATT_ID`
are now DroneCAN parameters, persisted in ESP32 flash (NVS) and editable from
the DroneCAN GUI tool over the drone bus — no USB connection needed (see
*Configuration over CAN* below). v0.2.0 (same day) confirmed the battery
protocol on hardware: a real Tattu/Grepow 18S 30Ah pack was captured on CN201
and the full telemetry decode (voltage, current, temp, SOC, SOH, cycles, 18
cell voltages, capacity, error flags, serial) plus the battery→drone
`BatteryInfo` bridge are implemented. v0.1.0 bench test (2026-07-16) verified
console, both MCP2515s, both DS18B20s, and the auto-arm sequence. Still
pending on hardware: FC node-ID allocation, bridge + param services on a real
FC, kill-trigger polarity (⚠ below).

## What it does

- **Main power switch control** — runs the precharge → latch-on arm sequence
  (auto-arm 3 s after boot by default, or via the `arm` serial command).
- **Kill trigger monitoring** — watches the 12 V trigger input and reports
  state changes; aborts an in-progress arm sequence if the trigger asserts.
- **Battery CAN (CAN1)** — decodes the Tattu 18S "E-UAVCAN" broadcast
  (confirmed protocol, see below): pack voltage/current/temp/SOC/SOH/cycles,
  all 18 cell voltages, design/remaining capacity, error bitfield, serial.
  Also decodes standard DroneCAN `NodeStatus`/`BatteryInfo` if a pack speaks
  that instead.
- **Drone CAN (CAN2, isolated)** — DroneCAN node with **dynamic node ID
  allocation**: at boot the CBC requests a node ID from the FC's allocation
  server (ArduPilot runs one by default), so 6 CBCs on one bus each get a
  unique ID with no per-board configuration. Unique ID is derived from the
  ESP32 eFuse MAC. Answers `GetNodeInfo` (`org.arrowair.cbc`) and broadcasts
  `NodeStatus` at 1 Hz once allocated. The **battery→drone bridge**
  republishes the decoded pack telemetry as standard
  `uavcan.equipment.power.BatteryInfo` at 2 Hz (stops if battery telemetry
  goes stale >5 s), so ArduPilot sees it with `BATT_MONITOR=8`.
- **Config over CAN** — `NODE_ID` and `BATT_ID` parameters via the standard
  DroneCAN services (`param.GetSet`, `param.ExecuteOpcode` SAVE/ERASE,
  `RestartNode`), stored in NVS flash. Editable from the DroneCAN GUI tool.
- **Temperatures** — reads both DS18B20 sensors (U501/U502).
- **Serial console** — status output + commands at 115200 baud over USB-C.

## Pin map (from the netlist)

| GPIO | Net | Function |
|---|---|---|
| IO4 | `PRECHARGE_ESP_SIGNAL` | Precharge FET on (HIGH = on), via Q202 → U206 opto |
| IO5 | `SCHMITT_OUT` | Kill sense input. HIGH = 12 V trigger present ⚠ verify polarity |
| IO6 | `ESP_ENABLE_SIGNAL` | Latch SET (pulse HIGH to arm). 10k pulldown R201 |
| IO7 | `TEMP2_DQ` | DS18B20 U502 (1-Wire) |
| IO15 | `TEMP1_DQ` | DS18B20 U501 (1-Wire) |
| IO10 | CS | MCP2515 U402 — **battery CAN** |
| IO21 | INT | U402 interrupt (ESP32 internal pull-up used) |
| IO48 | RESET | U402 hardware reset |
| IO14 | CS | MCP2515 U401 — **drone CAN** (isolated via ADM3053) |
| IO47 | INT | U401 interrupt (internal pull-up) |
| IO9 | RESET | U401 hardware reset |
| IO11/12/13 | MOSI/SCK/MISO | Shared SPI (ESP32-S3 default FSPI pins) |
| IO0, EN, RXD0, TXD0 | — | Boot/program via CP2102N (U302) auto-program circuit |

Both MCP2515s run **16 MHz crystals** (X401/X402 per BOM — the repo README
still says 8 MHz/ESP32-C3/SN65HVD230 from an earlier revision and should be
updated).

External connector J501 (8-pin JST JWPF): 1,2 = GNDE · 3,4 = 12 V isolated out
· 5 = CANL_DRONE · 6 = CANH_DRONE · 7 = 12 V trigger (kill) · 8 = 12 V supply
in (gate-drive rail).

## Switching behavior (how the hardware actually works)

Traced from the schematics; this drives the firmware design:

1. **Power-up = safe OFF.** The POR network (D201/R205/C202) resets the
   74LVC2G02 cross-coupled NOR latch (U201), so the main FETs are open until
   firmware acts. The ESP32 is powered from the always-hot bucks.
2. **Arming.** Firmware drives precharge (IO4) for 1 s, then pulses IO6 HIGH
   ≥50 ms to SET the latch → gate driver chain closes the 8 low-side FETs →
   battery negative is connected. Precharge overlaps 250 ms, then off.
   Timing per `docs/power-architecture.md` (500 ms–2 s window).
3. **The latch survives ESP32 reboots** (that's its purpose — R201 pulls IO6
   low while the ESP restarts, latch keeps its state).
4. **⚠ Firmware cannot turn the switch OFF.** The latch has no reset input
   from the ESP32 — the only reset is the power-on POR. There is deliberately
   no `off` command.
5. **Kill trigger.** 12 V on J501.7 → Schmitt buffer (U202) → drives the
   CPC1106N SSR (U203, 1-Form-B **normally closed**) which **opens** the gate
   path in pure hardware → main FETs off, no firmware involved. IO5 lets
   firmware observe it.
6. **⚠ Kill is only active while the trigger is present.** When the 12 V
   trigger is removed, the SSR closes again and — because the latch is still
   set — **the main FETs turn back ON**. The firmware prints a warning when
   this happens. If "kill = latched off until power-cycle" is the intended
   behavior, that needs a hardware change or clarification.

## Build & flash

The project pins the **pioarduino** platform (Arduino core 3.x) in
`platformio.ini`. This is required: the official `espressif32` PlatformIO
platform is frozen on Arduino core 2.0.x, which **boot-loops on the S3 N16R8
module** (repeating `rst:0x3 (RTC_SW_SYS_RST)` with no output — seen on the
first board, fixed by the platform switch). pioarduino needs Python ≥ 3.10;
the PlatformIO VSCode extension's bundled Python is fine.

### PlatformIO — VSCode extension

1. Install the "PlatformIO IDE" extension.
2. **File → Open Folder** → open this `firmware/` directory itself (the folder
   containing `platformio.ini` — opening the repo root won't be detected).
3. First open downloads the platform + toolchain (a few minutes).
4. Bottom toolbar: ✓ = Build, → = Upload (USB-C, CP2102N auto-reset — no
   button dance), 🔌 = Serial Monitor (115200 preset). The monitor doesn't
   echo typed characters; commands still work, press Enter.

### PlatformIO — CLI

```bash
pip install platformio          # once (needs Python >= 3.10)
cd firmware
pio run                         # build
pio run -t upload               # flash via USB-C (CP2102N auto-reset/boot)
pio device monitor              # serial console (115200, DTR/RTS preset)
```

If upload doesn't autodetect the port: `pio run -t upload --upload-port
/dev/cu.usbserial-*` (macOS) or `/dev/ttyUSB0` (Linux); on Windows install the
Silicon Labs CP210x VCP driver if COMx doesn't appear. If auto-program ever
fails, hold BOOT-strapping by shorting IO0 low while pressing reset (SW301).

### esptool with a prebuilt binary

```bash
pio run                         # produces .pio/build/cbc/firmware.bin
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 write_flash \
  0x0 .pio/build/cbc/bootloader.bin \
  0x8000 .pio/build/cbc/partitions.bin \
  0x10000 .pio/build/cbc/firmware.bin
```

### Arduino IDE (alternative)

Arduino IDE 2.3+ (esp32 core 3.x). Board: *ESP32S3 Dev Module*, Flash Size
16 MB, PSRAM: **OPI PSRAM**, USB CDC On Boot: **Disabled** (console is on
UART0 through the CP2102N, not native USB). Libraries: `autowp-mcp2515`,
`OneWire`, `DallasTemperature`.

### Troubleshooting

- **Endless `rst:0x3` ROM banner, no output:** image built for the wrong
  core/memory config — make sure the pinned pioarduino platform and the
  `qio_opi` settings in `platformio.ini` are intact (N16R8 = octal PSRAM).
- **Banner prints, then silence before `[can] ... init`:** SPI initialized at
  static-init time. The MCP2515 objects must be constructed inside `setup()`
  *after* `SPI.begin()` (the library's default constructor calls
  `SPI.begin()` from the global constructor, which breaks on core 3.x) —
  don't move them back to globals.
- `[E] addApbChangeCallback(): duplicate func` at boot is a known harmless
  Arduino-core message.
- OneWire `extra tokens at end of #undef` compile warnings are cosmetic.

## Serial console

115200 baud. Status prints every 2 s. Commands:

```
status                print full status now
arm                   run precharge + latch-on sequence
precharge on|off      drive precharge FET manually (bench testing)
raw bat on|off        dump raw battery bus frames (OFF by default since v0.2.0)
raw drone on|off      dump raw drone bus frames
bitrate bat|drone 125|250|500|1000
scan                  auto-detect battery bus bitrate (3 s listen per rate)
help
```

Config defaults live at the top of `src/main.cpp`: `AUTO_ARM_ON_BOOT` (1,
confirmed by Julius), `AUTO_ARM_DELAY_MS`, precharge timing,
`CBC_DEFAULT_NODE_ID` (factory default for the `NODE_ID` parameter), default
bitrates (1 Mbps both buses). Node/battery ID are runtime parameters — see
next section.

## Configuration over CAN (v0.3.0)

No USB needed: connect the [DroneCAN GUI tool](https://dronecan.github.io/GUI_Tool/Overview/)
to the drone bus, double-click the CBC node (`org.arrowair.cbc`) → *Fetch All*:

| Parameter | Range | Default | Meaning |
|---|---|---|---|
| `NODE_ID` | 0–125 | 0 | 0 = dynamic allocation from the FC; 1–125 = static node ID. Applies after save + restart. |
| `BATT_ID` | 0–255 | 0 | `battery_id` tag in the bridged `BatteryInfo`, so the FC can tell the 6 packs apart (match to ArduPilot `BATTx_SERIAL_NUM`). |

Workflow in the GUI tool: edit the value → **Send** (applies in RAM, `BATT_ID`
takes effect immediately) → **Store All** (persists to NVS flash) → **Restart**
(needed for `NODE_ID` to take effect). *Erase All* resets both to defaults.
The GUI tool runs its own DNA allocation server, so a factory-fresh CBC
(`NODE_ID`=0) gets an ID and shows up even without an FC on the bus.

Implemented services: `uavcan.protocol.param.GetSet` (integer params),
`uavcan.protocol.param.ExecuteOpcode` (SAVE/ERASE),
`uavcan.protocol.RestartNode` (standard magic number).

## Bench check over USB

USB-C alone powers the whole logic section: VBUS → D301 → +5 V rail → LDOs →
3V3 (ESP32, both MCP2515s, both CAN transceivers, temp sensors). The battery
power path stays dead without a pack — the 12 V gate rail comes from the
battery-fed flyback or J501.8 — so the arm sequence runs harmlessly as a
logic-level test.

So: plug in USB-C, `pio device monitor -b 115200`, and check:

1. Boot banner, `[can] bat/drone init` results (0 = OK for reset/bitrate/mode
   — nonzero means SPI/MCP2515 trouble).
2. `[status]` line every 2 s with both temp sensors reading plausible values.
3. Kill input: apply/remove 12 V on J501.7 → `[kill] ...` log lines.
4. Auto-arm sequence log ~3 s after boot (`precharge → latch → ON`); verify
   IO4/IO6 with a scope if you want the timing.
5. With the drone bus wired to an FC: `[dronecan] node ID n allocated by the
   FC` and the CBC appearing in the DroneCAN/UAVCAN inspector.
6. With a battery on CN201: `[battery] tattu ...` decode lines (two per
   status: pack summary + cells/capacity/serial). `raw bat on` for frames.

## Battery protocol (Tattu 18S "E-UAVCAN") — confirmed 2026-07-21

Confirmed against a real Grepow/Tattu 18S 30Ah pack captured on the first CBC
board (Julius's frame dump). Matches the TATTU-2177 spec appendix.

- **Framing:** CAN 2.0B extended, **1 Mbps**, 4 Hz. Ext ID `0x01109216` =
  priority 1 · message type `0x1092` · source node `0x16`. UAVCAN-v0-style
  multi-frame transfer with two vendor quirks the decoder handles:
  1. the **transfer ID increments on every frame** (not constant per
     transfer — a strict v0 reassembler rejects these, which is why fw
     v0.1.0 decoded nothing);
  2. the transfer CRC seed is unknown, so it is not validated.
- **Payload** (little-endian; 76-byte variant with serial alternates with a
  60-byte variant without):
  `i16 manufacturer, i16 model, u16 voltage[10mV], i16 current[10mA,
  +=charging], i16 temp[°C], u16 soc[%], u16 cycles, i16 health[%],
  u16 cell_mv[18], u16 design[mAh], u16 remaining[mAh], u32 error_bits,
  char serial[16]`.
- The pack also broadcasts a second message type `0x17E4` (not yet mapped)
  and an ASCII `V1` version beacon on `0x001E0959`.

Standard `uavcan.equipment.power.BatteryInfo` (1092) decode is also kept in
case a pack ships with real DroneCAN firmware. Raw dump (`raw bat on`) and
`scan` remain available if a pack shows up speaking something else.

**Bridge:** the decoded telemetry is re-encoded as
`uavcan.equipment.power.BatteryInfo` and broadcast on the drone bus at 2 Hz.
Note the sign convention flip: BatteryInfo current is positive-discharging
(ArduPilot convention), Tattu is positive-charging — the encoder negates.

## Confirmed by Julius (2026-07-16)

- Latch behavior (firmware can arm, not disarm) and momentary kill are **as
  designed**.
- Auto-arm at boot is the correct default.
- Battery: Tattu **18S** variant; 6 packs/CBCs on the drone bus → dynamic
  node allocation handles the IDs.

## Open items

- ✅ ~~First bench test over USB~~ — passed 2026-07-16 (console, CAN init,
  temps, auto-arm).
- ✅ ~~Battery frames on CN201 / protocol identification~~ — confirmed
  2026-07-21, full 18S decode implemented.
- ✅ ~~CAN bridge battery→drone~~ — `BatteryInfo` republish at 2 Hz
  implemented (v0.2.0); needs verification against a real FC.
- ⚠ Confirm kill-trigger polarity on IO5 (assumed HIGH = trigger present).
- ✅ ~~Per-battery `battery_id`~~ — `BATT_ID` parameter (v0.3.0), set per
  board over CAN.
- ✅ ~~Persist config in NVS instead of compile-time defaults~~ — v0.3.0.
- ⚠ Verify allocation handshake + GetNodeInfo against a real FC (ArduPilot
  DNA server); the protocol code is written from the v0 spec, untested.
- ⚠ Verify the param services against the DroneCAN GUI tool on hardware
  (written from the DSDL definitions, untested).
- Map the second Tattu message type `0x17E4`.
