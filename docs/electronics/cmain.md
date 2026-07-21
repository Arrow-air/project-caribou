---
title: CMAIN — Main PCB
sidebar_label: CMAIN (Main PCB)
sidebar_position: 4
description: The Caribou Main PCB — central low-voltage avionics hub for all six motor arms
---

# CMAIN — Caribou Main PCB

CMAIN is the central hub of the Caribou electrical system. It merges the roles of the Feather PDB and the Quiver main PCB: low-voltage power distribution, CAN routing, ESC signal outputs, and sensor connections for the whole aircraft. High-voltage/high-current handling is deliberately out of scope — that lives on the [CBC](./cbc.md) boards at each arm.

Source: [`engineering/electronics/pcbs/CMAIN_PCB/`](https://github.com/Arrow-air/project-caribou/tree/main/engineering/electronics/pcbs/CMAIN_PCB) · Tracking issue: [#8](https://github.com/Arrow-air/project-caribou/issues/8)

## Interfaces

| Quantity | Connector | Purpose |
|---|---|---|
| 6 | Amphenol AT13-12PB-BM03 | One per CBC — isolated 12 V in, DroneCAN, kill/control signals |
| 6 | Amphenol AT13-6P-BM01GRY | One per ESC — CAN H, CAN L, PWM, GND |

The AT series is an automotive-grade sealed connector family chosen for vibration resistance and field serviceability. Additional external sensor and payload connectors (following Quiver's pattern of radar, LiDAR, camera, and attachment interfaces) are still being defined — compact waterproof M8 circular connectors are the leading candidate for low-current sensor ports.

## Scope

- Aggregates six isolated 12 V feeds from the CBCs into the aircraft low-voltage system
- Routes the aircraft-wide DroneCAN network between CBCs, ESCs, and the flight controller
- Provides ESC signal interfaces (CAN + PWM) for the six Hobbywing X15 propulsion units
- Central-frame form factor, mounted in the aircraft core

## Status

CMAIN boards are manufactured and were delivered in July 2026, with 3D-printed enclosures complete. Detailed docs — connector requirements, routing rules, footprint audits, and design proposals — live in the board's [`docs/`](https://github.com/Arrow-air/project-caribou/tree/main/engineering/electronics/pcbs/CMAIN_PCB/docs) folder.
