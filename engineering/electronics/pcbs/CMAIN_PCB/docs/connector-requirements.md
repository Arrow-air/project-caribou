# CMAIN_PCB Connector Requirements

This document tracks the initial connector requirements for the Caribou Main PCB (`CMAIN_PCB`).

## CBC_PCB Interfaces

The aircraft has six motor-arm battery connector boards (`CBC_PCB`), one per arm. `CMAIN_PCB` should provide one interface for each CBC board.

| Quantity | Connector | Purpose | Notes |
|---:|---|---|---|
| 6 | `AT13-12PB-BM03` | CBC_PCB connection | One 12-pin connector per CBC_PCB. Carries the CBC/CMAIN low-voltage, CAN, and signal interface. Exact pinout should follow the CBC harness definition. |

## ESC Interfaces

The aircraft has six Hobbywing XRotor X15 propulsion units / ESCs. `CMAIN_PCB` should provide one ESC signal connector per motor arm.

| Quantity | Connector | Purpose | Pinout |
|---:|---|---|---|
| 6 | `AT13-6P-BM01GRY` | ESC connection | `CAN H`, `CAN L`, `PWM`, `GND`, `GND`, `NC` |

The Hobbywing ESC drawing below is the current reference for the ESC-side cable/interface details. It shows the high-current 6 AWG power cable, shielded cable, PWM/GND JR 3-pin connector, and CAN H/L JR 3-pin connector.

![Hobbywing X15 ESC cable drawing](../images/hobbywing-x15-esc-cable-drawing.png)

## External Sensor / Payload Interfaces

Caribou may need additional external waterproof connectors. Available board space and final component placement still need to be checked before freezing this connector set.

Quiver used external interfaces for:

- 360° LiDAR (`S2L`)
- Bottom radar
- Front radar
- SIYI camera
- 3× attachment interfaces

## Smaller Waterproof Connector Candidates

The AT13 connectors are rugged but large. For low-current sensor interfaces, smaller waterproof circular connectors may be better if they meet the required pin count, current, field-serviceability, and harness availability.

Initial candidates to evaluate:

| Connector family | Why consider it | Tradeoffs / checks |
|---|---|---|
| M8 circular, A/B-coded | Common compact waterproof sensor connector family; available with 3, 4, 5, 6, and 8 contacts from some vendors; typically IP67+ when mated. Good candidate for CAN/PWM/sensor-level signals. | Current rating is usually low, often around 0.5–3 A depending on pin count/vendor. Verify vibration retention, keyed variants, cable availability, and PCB/panel-mount geometry. |
| M5 circular | Smaller than M8 and available as IP67/IP68 connector systems, typically 2–4 contacts. | Likely too few pins for combined CAN + power + extra signals. Better for very small/simple sensors only. |
| M12 circular | Industrial, waterproof, widely available, and robust. | Usually not meaningfully smaller than AT13 for dense packaging; better where robustness/standardization matters more than size. |
| Sealed Micro/Mini automotive connector families | Familiar rugged/waterproof harness ecosystem. | Many are cable/wire-to-wire oriented rather than clean board-edge sensor ports; board packaging may not improve enough over AT13. |

Recommendation for sensor ports: evaluate M8 first for compact waterproof signal connectors, especially for interfaces needing 4–6 contacts. Keep AT13 for interfaces where ruggedness, existing harness compatibility, or higher current/physical robustness matters more than board area.

## Open Items

- Confirm exact CBC-to-CMAIN 12-pin pinout for `AT13-12PB-BM03`.
- Confirm ESC connector orientation and final pin order for `AT13-6P-BM01GRY`.
- Decide which external sensors/attachments must be on `CMAIN_PCB` versus remote breakout/harness modules.
- Check board area and connector keepout areas before selecting additional AT13 or smaller sensor connectors.
- Select and validate smaller waterproof connector candidates with data sheets, mating cable availability, STEP models, and KiCad footprints.
