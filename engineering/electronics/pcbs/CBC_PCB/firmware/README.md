# CBC_PCB Firmware

Firmware for the ESP32-S3-WROOM-1-N16R8 (U301) on the Caribou Battery Connector
board (CBC_PCB, ordered board state / PR #51).

**Status: v0.1.0 — written against the netlist and docs, not yet tested on
hardware.** Everything marked ⚠ below needs a check on the first real board.

## What it does

- **Main power switch control** — runs the precharge → latch-on arm sequence
  (auto-arm 3 s after boot by default, or via the `arm` serial command).
- **Kill trigger monitoring** — watches the 12 V trigger input and reports
  state changes; aborts an in-progress arm sequence if the trigger asserts.
- **Battery CAN (CAN1)** — listens on the Tattu bus, attempts DroneCAN decode
  (`NodeStatus`, `BatteryInfo`) and dumps raw frames so the actual protocol can
  be identified.
- **Drone CAN (CAN2, isolated)** — DroneCAN node with **dynamic node ID
  allocation**: at boot the CBC requests a node ID from the FC's allocation
  server (ArduPilot runs one by default), so 6 CBCs on one bus each get a
  unique ID with no per-board configuration. Unique ID is derived from the
  ESP32 eFuse MAC. Answers `GetNodeInfo` (`org.arrowair.cbc`) and broadcasts
  `NodeStatus` at 1 Hz once allocated. The battery→drone telemetry **bridge
  is stubbed** (`bridgeProcess()` in `src/main.cpp`) for later.
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

### PlatformIO (recommended)

```bash
pip install platformio          # once
cd firmware
pio run                         # build
pio run -t upload               # flash via USB-C (CP2102N auto-reset/boot)
pio device monitor -b 115200    # serial console
```

If upload doesn't autodetect the port: `pio run -t upload --upload-port
/dev/cu.usbserial-*` (macOS) or `/dev/ttyUSB0` (Linux). The UMH3N auto-program
circuit means no button dance is needed; if it ever fails, hold BOOT-strapping
by shorting IO0 low while pressing reset (SW301).

### esptool with a prebuilt binary

```bash
pio run                         # produces .pio/build/cbc/firmware.bin
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 write_flash \
  0x0 .pio/build/cbc/bootloader.bin \
  0x8000 .pio/build/cbc/partitions.bin \
  0x10000 .pio/build/cbc/firmware.bin
```

### Arduino IDE (alternative)

Board: *ESP32S3 Dev Module*, Flash Size 16 MB, USB CDC On Boot: **Disabled**
(console is on UART0 through the CP2102N, not native USB). Libraries:
`autowp-mcp2515`, `OneWire`, `DallasTemperature`.

## Serial console

115200 baud. Status prints every 2 s. Commands:

```
status                print full status now
arm                   run precharge + latch-on sequence
precharge on|off      drive precharge FET manually (bench testing)
raw bat on|off        dump raw battery bus frames (ON by default)
raw drone on|off      dump raw drone bus frames
bitrate bat|drone 125|250|500|1000
scan                  auto-detect battery bus bitrate (3 s listen per rate)
help
```

Config defaults live at the top of `src/main.cpp`: `AUTO_ARM_ON_BOOT` (1,
confirmed by Julius), `AUTO_ARM_DELAY_MS`, precharge timing,
`CBC_STATIC_NODE_ID` (0 = dynamic allocation from the FC; set 1..125 to pin),
default bitrates (1 Mbps both buses).

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
6. With a battery on CN201: `[bat]` raw frames + `[battery] tattu ...` decode.

## Battery protocol notes (Tattu)

Both protocols seen from Tattu packs are supported, based on the working
Quiver RPi bridge (`project-quiver`
`docs/Operations/firmware/tattu-bridge/tattu_bridge.py`):

1. **Tattu vendor broadcast** (what Quiver packs actually send): DroneCAN v0
   multi-frame transfer on ext ID `0x01109216` — priority 1, data type
   `0x1092` (4242), source node 22, **1 Mbps**. 8-frame burst; payload:
   `u16 vendor, u16 model, u16 voltage[mV], i16 current[10mA], i16 temp[°C],
   u16 soc[%]`, remainder not yet mapped (likely per-cell voltages etc.).
   Decoded automatically (`[battery] tattu ...` in the status output). The
   transfer CRC is not validated (vendor DSDL signature unknown).
2. **Standard `uavcan.equipment.power.BatteryInfo`** (1092), in case a pack
   ships with the DroneCAN firmware variant. ⚠ Packed SoC/SoH bitfields and
   the CRC signature should be sanity-checked against real frames once.

Raw dump (`raw bat on`, default) and `scan` remain available if a pack shows
up speaking something else.

## Confirmed by Julius (2026-07-16)

- Latch behavior (firmware can arm, not disarm) and momentary kill are **as
  designed**.
- Auto-arm at boot is the correct default.
- Battery: Tattu **18S** variant; 6 packs/CBCs on the drone bus → dynamic
  node allocation handles the IDs.

## Open items

- ⚠ Not hardware-tested yet — first bench test over USB (see above), then lab
  supply + no load before a real pack.
- ⚠ Confirm kill-trigger polarity on IO5 (assumed HIGH = trigger present).
- ⚠ Verify allocation handshake + GetNodeInfo against a real FC (ArduPilot
  DNA server); the protocol code is written from the v0 spec, untested.
- CAN bridge battery→drone (`BatteryInfo` republish, per-battery `battery_id`
  so the FC can tell the 6 packs apart) once bench-verified.
- Map the remaining ~42 bytes of the Tattu 0x1092 payload (per-cell voltages?).
- Persist config in NVS instead of compile-time defaults.
