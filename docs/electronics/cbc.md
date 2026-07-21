---
title: CBC — Battery Connector PCB
sidebar_label: CBC (Battery Connector)
sidebar_position: 2
description: The Caribou Battery Connector PCB — power switching, protection, regulation, and dual CAN per motor arm
---

# CBC — Caribou Battery Connector PCB

The CBC sits between an 18S Tattu 4.0 smart battery and a Hobbywing X15 propulsion unit. Each of the six motor arms carries one CBC. It is the safety-critical board in the power path: nothing flows from a battery to a motor except through a CBC.

Source: [`engineering/electronics/pcbs/CBC_PCB/`](https://github.com/Arrow-air/project-caribou/tree/main/engineering/electronics/pcbs/CBC_PCB) · Tracking issue: [#6](https://github.com/Arrow-air/project-caribou/issues/6)

## What It Does

1. **Battery mating** — mates directly with the Tattu 4.0 battery connector (via the [C90D adapter](./c90d.md)). Battery-present detection pins must be bridged for the pack to power on
2. **Short-circuit protection** — 250 A ceramic bolt-in fuse (Eaton AMXL-250)
3. **Active power switching** — MOSFET stage with three functions:
   - *Precharge* — soft-starts the ESC input capacitors through a resistor before the main path closes, limiting inrush current
   - *Main power enable* — low-Rdson parallel MOSFETs connect the HV bus to the propulsion output
   - *Kill switch* — hardware-level cutoff driven by the aircraft's external HV kill switch, independent of the MCU
4. **Voltage regulation** — 5 V / 3.3 V rails for onboard logic (GNDI, non-isolated) and a galvanically isolated 12 V / 10 A output (GNDE) that powers the aircraft's low-voltage system through CMAIN
5. **Dual CAN** — battery protocol on one bus, DroneCAN on the other, bridged in firmware

## Key Specifications

| Parameter | Value |
|---|---|
| Battery configuration | 18S (64.8 V nominal, 75.6 V max) |
| Continuous current | 100 A |
| Spike current | 200 A (short duration) |
| Main fuse | 250 A ceramic bolt-in |
| Isolated output | 12 V / 10 A (GNDE) |
| MCU | ESP32-S3-WROOM-1 |
| CAN controllers | 2× MCP2515 on shared SPI |
| EDA tool | KiCad |

The current ratings are derived from Hobbywing X15 motor data: roughly 45 A per motor at hover throttle (~50%), ~100 A at 72% throttle, and ~197 A at full throttle.

## Dual CAN Bridge

The CBC has two independent CAN interfaces:

- **CAN 1 — Battery** — private bus to the Tattu 4.0 smart battery
- **CAN 2 — DroneCAN** — aircraft-wide bus to the flight controller via CMAIN

Firmware decodes the battery pack telemetry (voltage, current, state of charge, cell data, temperatures) and republishes it as standard DroneCAN battery messages, so the autopilot sees each pack as a normal DroneCAN battery monitor. CAN termination is jumper-selectable and open by default.

## Firmware

Firmware for the ESP32-S3 is in review ([PR #54](https://github.com/Arrow-air/project-caribou/pull/54)): switch control (precharge/enable/kill sensing), dual-CAN drivers, Tattu battery telemetry decode, and the DroneCAN bridge.

Bench validation (July 2026) on the manufactured boards: full Tattu telemetry decode on real hardware, and two CBCs connected to a flight controller successfully completed DroneCAN dynamic node allocation, with battery voltage/SoC/temperature visible in Mission Planner via standard `BATT_MONITOR` DroneCAN configuration.

## Status

Manufactured CBC boards were delivered in mid-July 2026 and are in bench bring-up, with enclosures printed. See the board's [`docs/`](https://github.com/Arrow-air/project-caribou/tree/main/engineering/electronics/pcbs/CBC_PCB/docs) folder for the power architecture, connector pinout, and schematic review notes.
