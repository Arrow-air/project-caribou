# CBC_PCB — Power Architecture

## Overview

The CBC board handles two distinct power domains with galvanic isolation between them:

- **GNDI** (Ground Internal) — non-isolated, powers onboard logic only
- **GNDE** (Ground External) — isolated, powers the aircraft electrical system via CMAIN_PCB

This isolation ensures that faults on one power train system cannot propagate into the aircraft LV bus.

## Power Flow Diagram

```
                    Tattu 4.0 Battery (18S, 54–75.6V)
                              |
                     [Prolanv EN60A Connector]
                              |
                     [AMXL-250 Fuse, 250A]
                              |
                     [MOSFET Power Stage]
                      /               \
                     /                 \
            [Propulsion Output]     [HV Bus Tap]
            to X15 via screw         |          \
            terminals             [Buck]    [Push-Pull]
            (100A cont /          5V/1A     12V/10A
             200A spike)          (GNDI)    (GNDE, isolated)
                                    |           |
                                  [LDO]     [AT Signal
                                  3.3V      Connector]
                                  (GNDI)    → to CMAIN_PCB
                                    |
                              [ESP32-C3]
                              [MCP2515 ×2]
                              [SN65HVD230 ×2]
                              [Gate Drivers]
                              [Sensors]
```

## MOSFET Power Stage

The power stage uses N-channel MOSFETs in a high-side configuration with charge pump gate drivers. Three switching functions are implemented:

### 1. Precharge Switch
- **Purpose:** Limits inrush current when first connecting to the ESC input capacitors
- **Method:** Series resistor (e.g. 10–47 Ω, power-rated) + MOSFET bypass
- **Sequence:** Precharge MOSFET closes first → capacitors charge through resistor → main MOSFET closes → precharge MOSFET opens
- **Duration:** Typically 500 ms – 2 s (firmware-controlled)

### 2. Main Power Enable
- **Purpose:** Connects the HV bus to the propulsion output after precharge completes
- **Method:** Low-Rdson N-channel MOSFET(s) in parallel for current sharing
- **Rating:** Must handle 200A spike with acceptable Rdson heating

### 3. Kill Switch
- **Purpose:** Emergency power cutoff, independent of MCU state
- **Method:** Hardware latch — external KILL_IN signal (from AT connector pin 7), only kills the circuit after a short delay
- **Behavior:** Opens all power MOSFETs regardless of firmware state
- **Recovery:** Requires power cycle or explicit re-arm command

## Voltage Regulation Details

### 5V Buck (GNDI)

| Parameter | Specification |
|---|---|
| Input Range | 54–75.6V |
| Output | 5.0V ± 5% |
| Output Current | ≥ 1A |
| Topology | Synchronous buck |
| Candidate IC | MP9487 or similar (100V+ Vin, integrated FETs) |
| Isolation | None (GNDI domain) |

The 5V rail is the main supply for all onboard logic. It must handle the full 18S voltage range and remain stable during transient load changes on the HV bus.

### 3.3V LDO (GNDI)

| Parameter | Specification |
|---|---|
| Input | 5.0V (from buck above) |
| Output | 3.3V ± 3% |
| Output Current | ≥ 500 mA |
| Topology | Linear regulator (LDO) |
| Candidate IC | AMS1117-3.3 or similar |
| Dropout | ≤ 1.0V |
| Power Dissipation | (5.0 − 3.3) × 0.5 = 0.85W max |

### 12V Isolated Supply (GNDE)

| Parameter | Specification |
|---|---|
| Input Range | 54–75.6V |
| Output | 12.0V ± 5% |
| Output Current | ≥ 10A |
| Topology | Isolated push-pull (or similar) |
| Isolation Voltage | ≥ 500V (functional isolation) |
| Candidate IC | TBD |

The 12V isolated output is the primary regulated power supply for the Caribou aircraft. It feeds through the AT signal connector into the wiring harness and on to the [CMAIN_PCB](../../CMAIN_PCB/), which distributes power to avionics, sensors, servos, and other subsystems.

At 12V / 10A = 120W output. With an assumed 90% converter efficiency:
- Input power = 120 / 0.90 = 133.33W
- Input current at 54V (worst case) = 133.33 / 54 = 2.4691A
- Input current at 75.6V = 133.33 / 75.6 = 1.7636A

## Thermal Considerations

### MOSFET Stage
At 200A spike through main MOSFETs with Rdson = 1 mΩ (paralleled):
- P = I² × R = 200² × 0.001 = 40W (transient, < 2 s)

At 100A continuous with Rdson = 1 mΩ:
- P = I² × R = 100² × 0.001 = 10W (continuous)

Adequate copper area and/or heatsinking is required for the MOSFET stage.

### Fuse
The AMXL-250 ceramic fuse handles its own thermal management. Mounting on screw terminals provides thermal mass and heat dissipation through the PCB copper.

### Buck Converter
At 1A output, 5V, from 75.6V input — even at 85% efficiency the dissipation is modest:
- P_in = 5.0 / 0.85 = 5.88W
- P_loss = 5.88 − 5.0 = 0.88W
