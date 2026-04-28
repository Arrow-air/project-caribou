# CBC_PCB — Requirements

## 1. Scope

The Caribou Battery Connector PCB (CBC) is a per-arm board in the Project Caribou hexacopter. It interfaces one 18S Tattu 4.0 smart battery with one Hobbywing XRotor X15 propulsion unit. Six identical CBC boards are used in total — one per motor arm.

## 2. Electrical Requirements

### 2.1 Power Path

| Requirement | Value | Rationale |
|---|---|---|
| Input Voltage Range | 54.0–75.6V (18S Li-Ion, 3.0–4.2V/cell) | Tattu 4.0 18S battery operating range |
| Continuous Current | ≥ 100A | Covers hover (~45A) and moderate climb (~100A) |
| Spike Current | ≥ 200A for ≥ 2 seconds | Full-throttle burst, e.g. emergency climb-out |
| Short-Circuit Protection | Fuse ≥ 200A, ≤ 300A | Must not nuisance-blow at 200A spike; must clear faults |
| Precharge | Soft-start inrush limiting | Protect ESC input capacitors from inrush damage |
| Kill Switch | Hardware-level, MCU-independent | Safety requirement — must disable power even if MCU is unresponsive |

### 2.2 Voltage Regulation

| Output | Voltage | Current | Isolation | Domain | Purpose |
|---|---|---|---|---|---|
| Internal 5V | 5.0V ± 5% | ≥ 1A | Non-isolated | GNDI | Onboard logic supply |
| Internal 3.3V | 3.3V ± 3% | ≥ 500 mA | Non-isolated (LDO from 5V) | GNDI | ESP32, MCP2515, CAN transceivers |
| External 12V | 12.0V ± 5% | ≥ 10A | Galvanically isolated | GNDE | Aircraft avionics via CMAIN_PCB |

### 2.3 Communication

| Interface | Protocol | Purpose |
|---|---|---|
| CAN 1 | Battery-proprietary (Tattu smart battery protocol) | Read battery state, SoC, cell voltages, temperature |
| CAN 2 | DroneCAN (UAVCAN v0) | Report battery status to ArduPilot via CMAIN_PCB |
| WiFi | 802.11 b/g/n (2.4 GHz) | Wireless diagnostics, configuration, firmware update |
| BLE | BLE 5.0 | Low-power wireless diagnostics |

### 2.4 Sensing

| Measurement | Method | Purpose |
|---|---|---|
| Board Temperature | 1-Wire (DS18B20) or NTC via ADC | Thermal monitoring of power path |
| HV Bus Voltage | Resistive divider → ADC | Voltage monitoring, under/over-voltage protection |

## 3. Mechanical Requirements

| Requirement | Value |
|---|---|
| Board Shape | Rectangular |
| Mounting | TBD |
| Battery Connector Orientation | Right-angle DIP (vertical mate with battery) |
| Signal Connector Orientation | Right-angle flange mount |
| Environmental | Outdoor operation, vibration environment (near motor arm) |

## 4. Connector Requirements

| Connector | Part Number | Function |
|---|---|---|
| Battery Interface | Prolanv EN60A | Mates with Tattu 4.0 (ACES 59604-0169D-003) |
| Propulsion Output | 2× Amphenol AMT0650009DB0000G | High-current screw terminals for motor cables |
| Fuse Mount | 2× Amphenol AMT0650009DB0000G | Bolt-in mount for Eaton AMXL-250 |
| Signal / Drone Interface | Amphenol AT13-12PB-BM03 | 12-pos harness to CMAIN_PCB |

## 5. Firmware Requirements

| Requirement | Details |
|---|---|
| CAN Protocol Bridge | Translate Tattu battery protocol (CAN 1) → DroneCAN battery status messages (CAN 2) |
| CAN Library | `libcanard` (lightweight UAVCAN v0 implementation) |
| Power Sequencing | Manage precharge → enable → run state machine |
| Kill Switch Response | Immediate power cutoff on hardware kill input, independent of firmware state |
| OTA Updates | Firmware update via WiFi (ESP32-C3 OTA capability) |
| Diagnostics | Expose battery state, temperatures, voltages via WiFi/BLE for ground testing |

## 6. Safety Requirements

| Requirement | Rationale |
|---|---|
| Kill switch must be hardware-level | Ensures power cutoff even if MCU hangs or firmware crashes |
| Fuse must be field-replaceable | Blown fuse = swap and fly, no soldering required |
| Isolation between GNDI and GNDE | Fault on aircraft bus must not affect battery management logic |
| No single-point power failure propagation | One CBC failure must not cascade to other motor arms |

## 7. Design Constraints

- EDA tool: **KiCad 10**
- All custom symbols, footprints, and 3D models must be stored in `kicad/libs/` (no reliance on contributor-specific global libraries)
- PCB naming convention follows Caribou project standards
- Firmware in `firmware/` directory, buildable with ESP-IDF or Arduino framework
