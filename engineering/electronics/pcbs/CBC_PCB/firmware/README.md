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
- **Drone CAN (CAN2, isolated)** — DroneCAN node skeleton broadcasting
  `NodeStatus` at 1 Hz (node ID 25) so the CBC is visible to the autopilot.
  The battery→drone telemetry **bridge is stubbed** (`bridgeProcess()` in
  `src/main.cpp`) for later.
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

Config defaults live at the top of `src/main.cpp`: `AUTO_ARM_ON_BOOT` (1),
`AUTO_ARM_DELAY_MS`, precharge timing, `CBC_NODE_ID` (25), default bitrates
(1 Mbps both buses).

## Battery protocol notes (Tattu 4.0)

Gens Ace markets the Tattu 4.0 as DroneCAN-compatible (ArduPilot/PX4), but
there are field reports (ArduPilot forum, "Tattu 4.0 Battery support over
CAN") of packs shipping with firmware that speaks a **proprietary CAN 2.0
protocol** instead, fixed only via Tattu's BD300 adapter + their config tool.
So the plan:

1. Connect the battery, watch the raw dump (`raw bat on`, default) and run
   `scan` if nothing appears at 1 Mbps.
2. If DroneCAN: the firmware already decodes `BatteryInfo` (voltage, current,
   SoC, capacity, temp) and prints it in the status line. ⚠ The packed
   SoC/SoH bitfields and the transfer-CRC signature should be sanity-checked
   against real frames once.
3. If proprietary: capture a minute of raw dump and we reverse it / or ask
   Tattu for the BD300 tool to switch the pack to DroneCAN mode.

## Open items

- ⚠ Not yet compiled against real hardware — first bench test should be done
  with a lab supply + no load before a real Tattu pack.
- ⚠ Confirm kill-trigger polarity on IO5 (assumed HIGH = trigger present).
- ⚠ Confirm the "kill releases → power returns" behavior is intended.
- Decide auto-arm policy (currently: arm 3 s after boot unless kill active).
- CAN bridge battery→drone (`BatteryInfo` republish) once protocol confirmed.
- Persist bitrate/config in NVS instead of compile-time defaults.
