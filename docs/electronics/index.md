---
title: Electronics Overview
sidebar_label: Overview
sidebar_position: 1
description: Overview of the Caribou custom electronics — CBC, C90D, and CMAIN boards
---

# Electronics Overview

Caribou uses a small family of custom PCBs, all designed in KiCad and developed in the open in [`engineering/electronics/pcbs/`](https://github.com/Arrow-air/project-caribou/tree/main/engineering/electronics/pcbs). The architecture builds on the Feather PDB and Quiver PCB designs.

## Board Family

- **[CBC — Caribou Battery Connector PCB](./cbc.md)** (6 per aircraft, one per motor arm) — interfaces an 18S smart battery with a motor arm: power switching, protection, voltage regulation, and dual CAN communication
- **[C90D — 90° Battery Adapter PCB](./c90d.md)** (6 per aircraft) — mechanical/electrical adapter between the battery connector and the CBC assembly
- **[CMAIN — Caribou Main PCB](./cmain.md)** (1 per aircraft) — central low-voltage avionics hub connecting all six arms to the flight controller

## Per-Arm Power Architecture

Each of the six motor arms is an independent power unit:

```
Tattu 4.0 18S battery
        │
     [C90D]  90° adapter
        │
     [CBC]   fuse · precharge · kill · regulation · CAN
      │  │
      │  └── isolated 12V + DroneCAN + signals ──→ harness ──→ CMAIN
      │
   [Hobbywing X15]  motor + ESC
```

There is no shared high-voltage bus between arms — each battery feeds its own propulsion unit through its own CBC. The CBC's galvanically isolated 12V output feeds the aircraft's low-voltage system via CMAIN, so a fault on the aircraft LV bus cannot back-feed into the battery power path.

## CAN Topology

Two separate CAN domains:

- **Battery CAN (per arm)** — each CBC talks to its own Tattu 4.0 smart battery on a private CAN bus, reading pack telemetry (voltage, current, state of charge, temperatures)
- **DroneCAN (aircraft-wide)** — the CBCs, ESCs, and flight controller share the aircraft DroneCAN network, routed through CMAIN. Each CBC translates its battery's telemetry onto DroneCAN so the autopilot sees six standard battery monitors

## Ground Domains

The electronics maintain strict ground-domain separation, labeled consistently across schematics and harnesses:

- **GNDI** — internal, non-isolated logic ground on each CBC (MCU, CAN controllers)
- **GNDE** — isolated external ground for the aircraft power distribution (12V system)

Deeper design detail — connector pinouts, power architecture, requirements, and review notes — lives with each board in the repository and in the pages linked above.
