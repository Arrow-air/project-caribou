# CBC_PCB — Caribou Battery Connector PCB

The Caribou Battery Connector PCB (CBC) interfaces 18S smart battery packs with the Caribou hexacopter electrical system. Each motor arm carries one CBC board connected to one battery, providing power switching, protection, voltage regulation, and dual CAN bus communication.

- **GitHub issue:** [#6 — Design BC-PCB for Caribou: 18S, 100A nominal, 200A spike](https://github.com/Arrow-air/project-caribou/issues/6)
- **Starting point:** Project Quiver BC-PCB KiCad design.
- **EDA Tool:** KiCad 10

## Context within Project Caribou

Project Caribou is a ~200 kg MTOW heavy-lift hexacopter with ~100 kg payload capacity, 18–24S power architecture, and ArduPilot firmware. The aircraft uses 6× Hobbywing XRotor X15 propulsion units, each fed by a dedicated 18S Tattu 4.0 smart battery.

Each of the 6 motor arms has:
- 1× Tattu 4.0 18S battery pack
- **1× CBC_PCB** (this board) — battery interface, protection, regulation
- 1× Hobbywing XRotor X15 motor + ESC

The CBC board sits between the battery and the propulsion unit. It provides:
1. **Battery mating** — mates directly with the Tattu 4.0 battery connector
2. **Short-circuit protection** — 250A ceramic fuse
3. **Active power switching** — precharge, kill switch, power enable via MOSFETs
4. **Voltage regulation** — galvanically isolated 12V (GNDE) for the aircraft power distribution, plus non-isolated 5V/3.3V (GNDI) for onboard logic
5. **Dual CAN bus** — battery protocol (CAN 1) and DroneCAN (CAN 2) as bridge
6. **Wireless diagnostics** — WiFi/BLE via ESP32-C3 for configuration and monitoring

The regulated 12V output (GNDE) feeds into the Caribou Main PCB ([CMAIN_PCB](../CMAIN_PCB/)) via the AT signal connector harness.

## Concept Renderings

These early renderings show the intended CBC_PCB packaging concept and approximate component placement. They are illustrative only and may change during schematic/layout development.

![CBC_PCB concept rendering with Tattu battery](images/cbc-pcb-battery-render.png)

![CBC_PCB front concept rendering](images/cbc-pcb-front-render.png)

## Board Overview

```
Battery (18S Tattu 4.0)
        |
        v
+---------------------+
|  Battery Connector   |  ← Power (+ / −) + CAN L/H from battery
|  (Prolanv EN60A)     |
+---------------------+
|  AMXL-250 Fuse       |  ← 250A short-circuit protection
+---------------------+
|  MOSFETs             |  ← Precharge / Kill switch / Power enable
+---------------------+
|  Screw Terminals     |  ← Red (+) and Black (−) cables
|  (AMT0650009DB0000G) |     to Hobbywing X15 propulsion
+---------------------+
|                       |
|  HV Bus (54–75.6V)   |
|    |            |     |
|    v            v     |
|  [Buck]    [Push-Pull]|
|  5V/1A     12V/10A   |
|  (GNDI)    (isolated) |
|    |        (GNDE)    |
|    v            |     |
|  [LDO]         |     |
|  3.3V          |     |
|  (GNDI)        |     |
+---------------------+
|  AT Signal Connector |  ← 12V (GNDE), DroneCAN, signals
|  (AT13-12PB-BM03)   |     to aircraft harness → CMAIN_PCB
+---------------------+
```

## Specifications

| Parameter | Value |
|---|---|
| Configuration | 18S (18 series cell groups) |
| Max Voltage | 75.6V (4.2V × 18) |
| Nominal Voltage | 64.8V (3.6V × 18) |
| Continuous Current | 100A |
| Spike Current | 200A (short duration) |
| Board Shape | Rectangular |
| EDA Tool | KiCad 10 |
| Board Layers | TBD |

### Current Rating Justification

Based on the Hobbywing XRotor X15 motor data:

| Throttle | Thrust | Current per Motor |
|---|---|---|
| 51% | 27,257 g | 44.7 A |
| 72% | 47,933 g | 101.6 A |
| 100% | 72,647 g | 197.1 A |

Hover throttle is ~50%. The 100A nominal rating covers normal operations; the 200A spike rating handles full-throttle bursts.

## Connectors and Fuse

### Battery Connector

**Mating Connector (Battery Side): ACES 59604-0169D-003**

This is the connector built into the Tattu 4.0 battery pack. The Prolanv EN60A on the CBC mates with this connector.

| Position | Function | Notes |
|---|---|---|
| Left bank (red) | Anodal + (Positive) | Multiple parallel power contacts |
| Right bank (blue) | Cathode − (Negative) | Multiple parallel power contacts |
| Center top | CAN H + CAN L | Battery communication bus |
| PIN1 + PIN2 (center bottom) | Battery Present Detection | Must be shorted (bridged) for the battery to power on |

> **Important:** PIN1 and PIN2 must be short-circuited by the mating connector (Prolanv EN60A) to signal "battery present" — the battery will not power on without this bridge. The CBC must include a trace or jumper connecting these two pins on the EN60A side.

**Board-Side Connector: Prolanv EN60A**

| Parameter | Value |
|---|---|
| Current per Power Pair | 60A |
| Power Pairs | 5 positive + 5 negative |
| Total Current Capacity | 300A |
| Signal Pins | 6 |
| Contact Resistance | 0.6 mΩ |
| Mating Cycles | 10,000 |
| Temperature Range | −40°C to +125°C |
| Mounting | Right-Angle DIP (PCB mount) |

The EN60A provides 300A total capacity across 5 power pairs, giving comfortable headroom above the 200A spike requirement. From the 6 signal pins, only 4 are used.

At 200A spike across 4 power pairs (50A each): P = I² × R = 50² × 0.0006 = 1.5W per pair = 6W total — well within thermal limits.

### Propulsion Screw Terminals

**Selected: Amphenol Anytek AMT0650009DB0000G**

These are the high-current screw terminals where cables to the Hobbywing X15 propulsion system are connected (+ and −).

| Parameter | Value |
|---|---|
| Type | Screw Terminal, Power Tap |
| Screw Size | M5 |
| Pins | 6 |
| Current Rating | 180A |
| Mounting | Through-Hole DIP |
| Contact Material | Brass + Steel Nut (matte tin plated) |
| Torque | 18 lbf·in |

Two sets of AMT0650009DB0000G screw terminals are used on the board: one pair for the propulsion output (+ / −), and another pair for mounting the main fuse.

### Signal Connector (Drone Interface)

**Selected: Amphenol AT13-12PB-BM03 (or similar AT series)**

This connector interfaces the CBC with the rest of the Caribou electrical system via a wiring harness to the [CMAIN_PCB](../CMAIN_PCB/).

| Parameter | Value |
|---|---|
| Series | Amphenol AT (automotive-grade) |
| Type | Right-Angle Flange Mount PCB Receptacle |
| Positions | 12 |
| Keying | B |
| Contact Plating | Tin |
| Mating Connector | AT series cable plug (wire-to-board) |

The AT series is an automotive/industrial-grade connector family designed for harsh environments — vibration-resistant, sealed, and rated for high mating cycles. The 12 positions carry the isolated 12V output (GNDE), DroneCAN bus lines, and control signals to/from the Caribou avionics.

### Main Fuse (Short-Circuit Protection)

**Selected: Eaton AMXL-250**

The main fuse sits between the battery connector and the power output stage, protecting against short circuits. It is mounted on two AMT0650009DB0000G screw terminals (bolt-in design).

| Parameter | Value |
|---|---|
| Type | Automotive bolt-in fuse |
| Current Rating | 250A |
| Voltage Rating | 125 Vdc |
| Body | Ceramic |
| Mounting | Bolt-in (M5 terminals) on 2× AMT0650009DB0000G |

The 250A fuse rating provides short-circuit protection while allowing the full 200A spike current without nuisance blowing. The ceramic body handles the thermal demands of high-current interruption.

## MCU and Communication

### MCU: ESP32-C3-MINI-1

The ESP32-C3-MINI-1 module is a compact RISC-V based microcontroller with integrated wireless connectivity. It includes a PCB antenna — no external antenna components are required. A keepout zone must be maintained around the antenna area on the PCB layout (no copper / ground plane underneath).

| Parameter | Value |
|---|---|
| Core | 32-bit RISC-V, 160 MHz |
| Flash | 4 MB (integrated) |
| SRAM | 400 KB |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | BLE 5.0 |
| GPIOs | 22 |
| SPI | 3× (1 used for dual MCP2515) |
| ADC | 6 channels, 12-bit |
| Operating Voltage | 3.0–3.6V |
| Deep Sleep Current | 5 µA |
| Antenna | Integrated PCB antenna |

### Dual CAN Bus (2× MCP2515 + Transceiver)

Two independent CAN networks using external MCP2515 controllers on a shared SPI bus. This provides identical timing behavior on both buses and clean separation of the battery-side and drone-side CAN domains.

| CAN Bus | Purpose | Controller | Transceiver |
|---|---|---|---|
| CAN 1 — Battery | Battery protocol (smart battery communication) | MCP2515 (SPI, CS1) | SN65HVD230 (3.3V) |
| CAN 2 — Drone | DroneCAN (UAVCAN v0) interface to Caribou avionics | MCP2515 (SPI, CS2) | SN65HVD230 (3.3V) |

Each MCP2515 requires an 8 MHz crystal + 2× 22 pF load capacitors. 120 Ω termination resistors are optional (directly connected or selectable via solder jumper depending on bus topology).

The board converts between the battery protocol (CAN 1) and DroneCAN (CAN 2) using `libcanard`. The DroneCAN bus connects via the AT signal connector through the wiring harness to the [CMAIN_PCB](../CMAIN_PCB/), which serves as the central avionics hub for all 6 motor arms.

### GPIO Allocation

| Function | GPIOs | Notes |
|---|---|---|
| SPI Bus (shared) — SCK, MOSI, MISO | 3 | Directly routed to both MCP2515 |
| MCP2515 #1 — CS1, INT1 | 2 | CAN 1 (Battery) |
| MCP2515 #2 — CS2, INT2 | 2 | CAN 2 (Drone) |
| Temperature Sensors | 2 | 1-Wire (e.g. DS18B20) or analog NTC via ADC |
| Circuit Switching | 4 | MOSFET gate drive (precharge, kill switch, power enable) |
| **Total Used** | **13** | |
| **Remaining / Reserve** | **9** | Available for future expansion |

## Functional Requirements

### Power Path

- **High-side MOSFET switching** — N-channel MOSFETs with charge pump gate drive
- **Precharge circuit** — soft-start via resistor + relay/MOSFET to limit inrush current
- **Emergency kill switch** — hardware-level kill input, independent of MCU. Connects to the Caribou external HV kill switch via the AT signal connector.
- **Main fuse** — Eaton AMXL-250 (250A) bolt-in fuse for short-circuit protection

### Voltage Regulation

The power supply architecture uses two independent stages with full galvanic isolation between internal onboard logic (GNDI) and external drone systems (GNDE).

#### Internal Supply (GNDI — non-isolated)

Provides power for onboard logic (ESP32, MCP2515, CAN transceivers, gate drivers, sensors).

| Stage | Topology | IC | Input | Output | Notes |
|---|---|---|---|---|---|
| 5V Rail | Sync Buck | e.g. MP9487 or similar (100V+, integrated FETs, ≥1A) | 54–75.6V (HV Bus) | 5V / 1A | Main internal rail |
| 3.3V Rail | LDO | e.g. AMS1117-3.3 or similar | 5V | 3.3V | ESP32, MCP2515, CAN transceivers |

#### External Supply (GNDE — galvanically isolated)

Provides isolated 12V output for the Caribou aircraft power distribution via the [CMAIN_PCB](../CMAIN_PCB/).

| Stage | Topology | IC | Input | Output | Notes |
|---|---|---|---|---|---|
| 12V Rail | Isolated Push-Pull | TBD | 54–75.6V (HV Bus) | 12V / 10A (isolated, GNDE) | Powers aircraft avionics, sensors, servos via CMAIN_PCB |

The isolation barrier between GNDI and GNDE ensures that a fault on the aircraft power bus does not back-feed into the battery management logic.

## Folder Layout

```
CBC_PCB/
├── README.md               ← this file
├── docs/                   ← board-specific notes, requirements, design docs
├── firmware/               ← board-specific firmware (ESP32-C3)
├── images/                 ← board renders, screenshots, diagrams, photos
├── kicad/                  ← KiCad 10 project files
│   └── libs/               ← project-local KiCad libraries
│       ├── 3dmodels/       ← project-local 3D models
│       ├── CBC_PCB.pretty/ ← project-local footprint library
│       └── CBC_PCB.kicad_sym ← project-local symbol library
└── manufacturing/          ← Gerbers, BOM, assembly drawings, pick-and-place
```

## Related

- [CMAIN_PCB](../CMAIN_PCB/) — Caribou Main PCB (central avionics hub)
- [Project Caribou](https://github.com/Arrow-air/project-caribou) — parent project
- [Issue #6](https://github.com/Arrow-air/project-caribou/issues/6) — tracking issue for this board
- [Issue #8](https://github.com/Arrow-air/project-caribou/issues/8) — CMAIN_PCB tracking issue
