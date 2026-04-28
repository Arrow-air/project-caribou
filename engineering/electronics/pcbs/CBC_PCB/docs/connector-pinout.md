# CBC_PCB — Connector Pinout Reference

## 1. Battery Connector — Prolanv EN60A ↔ ACES 59604-0169D-003

The Prolanv EN60A (board side) mates with the ACES 59604-0169D-003 built into the Tattu 4.0 battery.

### Power Contacts

| Bank | Function | Pairs | Current per Pair | Total Capacity |
|---|---|---|---|---|
| Left (red) | Positive (+) | 5 | 60A | 300A |
| Right (blue) | Negative (−) | 5 | 60A | 300A |

### Signal Contacts

| Pin | Function | Notes |
|---|---|---|
| CAN H | Battery CAN High | → MCP2515 #1 (CAN 1) via SN65HVD230 |
| CAN L | Battery CAN Low | → MCP2515 #1 (CAN 1) via SN65HVD230 |
| PIN1 | Battery Present Detection | Must be shorted to PIN2 on EN60A side |
| PIN2 | Battery Present Detection | Must be shorted to PIN1 on EN60A side |
| (unused) | — | 2 signal pins reserved |

> ⚠️ **PIN1 + PIN2 must be bridged** on the EN60A (PCB trace or jumper). Without this bridge, the Tattu 4.0 battery will not power on.

## 2. Signal Connector — Amphenol AT13-12PB-BM03

12-position automotive-grade connector carrying regulated power and signals to the Caribou main harness → [CMAIN_PCB](../../CMAIN_PCB/).

### Proposed Pin Assignment

| Pin | Signal | Domain | Notes |
|---|---|---|---|
| 1 | 12V OUT | GNDE | Isolated 12V supply to aircraft |
| 2 | 12V OUT | GNDE | Parallel pin for current sharing |
| 3 | GND_E | GNDE | Isolated ground return |
| 4 | GND_E | GNDE | Parallel pin for current sharing |
| 5 | DroneCAN H | — | CAN 2 High → CMAIN_PCB |
| 6 | DroneCAN L | — | CAN 2 Low → CMAIN_PCB |
| 7 | KILL_IN | — | Hardware kill switch input |
| 8 | Reserved | — | Future use |
| 9 | Reserved | — | Future use |
| 10 | Reserved | — | Future use |
| 11 | Reserved | — | Future use |
| 12 | Reserved | — | Future use |

> **Note:** Pin assignment is preliminary and subject to change during schematic design. Dual pins for 12V and GND_E are used to share the 10A load across two AT contacts (each AT contact is rated ~7.5A for 14 AWG).

## 3. Propulsion Screw Terminals — Amphenol AMT0650009DB0000G

Two 6-pin screw terminal blocks for high-current output to the Hobbywing XRotor X15.

| Terminal Block | Function | Cable | Termination |
|---|---|---|---|
| Block 1 | Positive (+) output | Red cable | Ring lug, M5 bolt |
| Block 2 | Negative (−) output | Black cable | Ring lug, M5 bolt |

Cable gauge: sized for 200A spike — check hobbywing wire gauge.

## 4. Fuse Mount — Eaton AMXL-250

The fuse is bolt-mounted between two additional AMT0650009DB0000G screw terminals on the board.

```
Battery (-) ──→ [Screw Terminal A] ──→ [AMXL-250 Fuse] ──→ [Screw Terminal B] ──→ MOSFET stage
```

| Parameter | Value |
|---|---|
| Fuse Rating | 250A |
| Voltage Rating | 125 Vdc |
| Bolt Size | M5 |
