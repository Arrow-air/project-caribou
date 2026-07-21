---
title: System Architecture
sidebar_label: Architecture
sidebar_position: 2
description: How Project Caribou is put together — airframe, propulsion, power, electronics, and avionics
---

# System Architecture

Caribou is a ~200 kg MTOW hexacopter organized around a simple structural idea: **six independent motor arms around a central core**. Each arm is a self-contained power unit — battery, power electronics, motor — and the core carries the avionics, low-voltage distribution, and payload.

:::note
This page covers the parts of the architecture that are documented in the repository today. Some areas (notably the structure and detailed avionics configuration) are still being written up as PT1 progresses — see the [PT1 status](./index.md#development-status) on the intro page.
:::

## Architecture at a Glance

```text
CENTRAL CORE
  - Flight controller (ArduPilot)
  - CMAIN PCB (LV distribution + CAN hub)
  - Payload interfaces
      |
      |  harness per arm: isolated 12V,
      |  DroneCAN, kill/control signals
      |
MOTOR ARM (x6)
  Tattu 4.0 18S battery
   -> C90D adapter PCB
   -> CBC PCB (fuse, precharge, kill, regulation, dual CAN)
   -> Hobbywing X15 motor + ESC with 63" propeller
```

## Propulsion

Six Hobbywing XRotor X15 integrated propulsion units (motor + ESC) with 63" (1600 mm) propellers, rated by Hobbywing at 32.5–37.5 kg working thrust and up to 72 kg maximum thrust per rotor. Hover sits around 50% throttle (~45 A per motor); full throttle draws close to 200 A per motor, which drives the current ratings throughout the power path.

ESCs take CAN and PWM signal inputs, connected to CMAIN via sealed automotive-grade connectors.

## Power

**Per-arm, no shared HV bus.** Each arm has its own 18S Tattu 4.0 smart battery (64.8 V nominal, 75.6 V max) feeding only its own motor through its own [CBC board](./electronics/cbc.md). PT1 flies at 18S; the platform is specified for an 18–24S architecture.

Key properties of the power design:

- **Fused and switched per arm** — 250 A ceramic fuse, MOSFET precharge and main-enable stages on every CBC
- **Hardware kill chain** — an external HV kill switch feeds every CBC through the harness and cuts motor power at the MOSFET stage, independent of firmware
- **Isolated low-voltage system** — each CBC generates a galvanically isolated 12 V / 10 A output; these feed the aircraft LV system through CMAIN, so aircraft-side faults cannot back-feed into any battery power path
- **Smart battery telemetry** — each CBC reads its pack over the battery's CAN protocol and republishes telemetry on DroneCAN, so the autopilot monitors all six packs with standard battery monitors

## Electronics

Three custom KiCad-designed boards — CBC (×6), C90D (×6), and CMAIN (×1) — covered in detail in the [Electronics section](./electronics/index.md).

## Avionics

- **ArduPilot** flight stack, compatible with QGroundControl and Mission Planner
- **Triple-redundant IMU, GPS, and compass**
- **DroneCAN** as the aircraft-wide bus for battery telemetry and ESC communication, routed through CMAIN

## Structure

The airframe uses a welded steel/aluminum core structure with detachable motor arms (carbon fiber or aluminum) for transport and field service. The PT1 test frame is built; structural documentation (materials, dimensions, analysis) is part of the in-progress PT1 engineering report.

## Design Lineage

Caribou deliberately reuses proven patterns from earlier Arrow aircraft: the CBC evolved from Quiver's battery connector PCB, CMAIN merges the Feather PDB with Quiver's main PCB, and the smart-battery + DroneCAN approach extends what Quiver established. Where Caribou differs is scale — per-arm batteries instead of a central pack, an order of magnitude more thrust per rotor, and high-voltage safety hardware (kill chain, precharge, isolation) sized accordingly.
