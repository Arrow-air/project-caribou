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
| 6 | JST JWPF 8-pin (B08B-JWPF-SK-R) | One per CBC — isolated 12 V in, DroneCAN, kill/control signals |
| 6 | JST JWPF 4-pin (B04B-JWPF-SK-R) | One per Hobbywing X15 ESC — CAN H, CAN L, PWM, GND |

External wire-to-board interfaces use the JST JWPF waterproof connector family (an earlier Amphenol AT13 concept was dropped in favor of JST). Smaller JST GH connectors are used for internal signal-level connections. The board also carries sensor and payload interfaces per the schematic — GNSS (including a DroneCAN RTK option), front and altimeter radar, SIYI gimbal/camera, safety switch, and a companion computer header — plus auxiliary power outputs.

## Scope

- Aggregates six isolated 12 V feeds from the CBCs into the aircraft low-voltage system
- Routes the aircraft-wide DroneCAN network between CBCs, ESCs, and the flight controller
- Provides ESC signal interfaces (CAN + PWM) for the six Hobbywing X15 propulsion units
- Central-frame form factor, mounted in the aircraft core

## Status

CMAIN boards are manufactured and were delivered in July 2026, with 3D-printed enclosures complete. Detailed docs — connector requirements, routing rules, footprint audits, and design proposals — live in the board's [`docs/`](https://github.com/Arrow-air/project-caribou/tree/main/engineering/electronics/pcbs/CMAIN_PCB/docs) folder.
