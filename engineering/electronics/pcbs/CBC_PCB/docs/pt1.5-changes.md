# CBC_PCB – PT1.5 schematic changes (issue #58)

Changed files: `CBC_PCB.kicad_sch`, `POWER SECTION.kicad_sch`, `POWER REGULATOR.kicad_sch`, `ESP32 SECTION.kicad_sch`, `CAN SECTION.kicad_sch`, `PERIPHERALS.kicad_sch`, `libs/CBC_PCB.kicad_sym`.
Close Eeschema before copying them over the project.

## 1. Fix the precharge circuit (ground reference)

**Problem:** `GND` was the switched ESC negative, i.e. the drain side of the low-side main FETs and of the precharge path. The 5 V bucks, ESP32, latch logic, U206/U207 LED drivers, battery CAN and the iso-12 V primary all returned to that node. Before precharge, `GND` floats toward battery+ through the ESC capacitors, so the ESP32 can never power up to switch U206. It's a deadlock. The PT1 workaround (U206 pins 5–6 bridged) leaves precharge permanently on. After a kill, the ESC then stays fed through R229 || R230 (16.5 Ω).

**Change:**
- New net `H_PWR-OUT-` = switched ESC negative. The 9 output-side `GND` power symbols were renamed:
  - Q201/Q204/Q205/Q207/Q208/Q209/Q210 drain
  - R229/R230
  - J204, D216, D217
- `NT201` (net tie, `NetTie:NetTie-2_SMD_Pad2.0mm`) ties logic `GND` to `H_PWR-IN-` at a single point.
- Result:
  - ESP32, battery CAN and the iso-12 V output run as soon as the battery is on.
  - U206/Q202 now work as drawn (`PRECHARGE_ESP_SIGNAL` high = precharge on). Remove the pin 5–6 bridge.
  - No other net changed. Verified by a full netlist diff against the original.

## 2. Precharge sensing + buzzer

| Ref | Function |
|---|---|
| R243, R244 (160 k 0805, C17485), R245 (10 k 0402, C190095), C228 (100 nF 0603) | `PRECHARGE_SENSE` = V(`H_PWR-OUT-`) vs `GND` = voltage across the main switch, 1:33 → ESP32 **IO1** (ADC1_CH0) |
| R246, R247, R248, C229 (same values) | `HV_BUS_SENSE` = battery voltage, 1:33 → ESP32 **IO2** (ADC1_CH1). Optional, can be DNP |
| BZ501 GSD9605YB-5V2800 (LCSC C530415), Q501 LMUN5235T1G, D501 1N4148WS | Active 5 V buzzer, ESP32 **IO38** (`BUZZER`, high = on) |

Scaling: V = V_adc × 33. 75.6 V → 2.29 V; use ADC_ATTEN_DB_12.

## 3. Aux input: inrush limiting and per-converter fault isolation

With logic `GND` on `H_PWR-IN-`, the 5 V bucks and the iso-12 V flyback no longer charge through the precharge path.

```
H_PWR-IN ─┬─ R208 10Ω ─ U204 (12 V gate)  ┐ OR-ed
          ├─ R209 10Ω ─ U205 (12 V gate)  ┘
          └─ RT601 NTC ─ H_PWR-IN_AUX ─┬─ R628 10Ω ─ H_PWR-IN_5V_P1 ─ U601  ┐ OR-ed
                                       ├─ R629 10Ω ─ H_PWR-IN_5V_P2 ─ U602  ┘
                                       └─ F604 3 A T ─ H_PWR-IN_ISO ─ C609/L601 ─ U604
```

- **RT601** (RUILON NTC 10D-15, LCSC C105635: 10 Ω cold, 5 A, THT P = 7.5 mm)
  - Cold: inrush ≤ 7.6 A, and the L601 filter is damped.
  - At 60 W iso-12 V output (the board's maximum): ≈1 Ω, ≈1.3 V drop, ≈1.6 W, ≈120 °C.
  - Needs ≈2 min off after heavy load to protect fully again.
- **R628/R629** (10 Ω 2512, C47019, same as R208/R209)
  - Per-buck inrush limit.
  - Sacrificial element if a buck or its input cap shorts, so the other buck keeps +5 V up.
  - R613/R615 (UVLO dividers) now sense their own buck's input.
- **F604** (Littelfuse 0154003.DRT, LCSC C206913, 3 A time-delay, 125 V AC/DC, 50 A interrupt)
  - Clears a short in the flyback input. The NTC alone would only get lower-ohmic as a fault heats it.
  - Residual: a hard short with the NTC hot (60 W) can reach ≈75 A prospective, which is above the 50 A interrupt rating at 125 V.
- The gate DCDCs keep R208/R209 on `H_PWR-IN`.

## 4. Part updates from the BOM review

- **D603** SMBJ85A → **SMBJ58A** (Littelfuse, C157526).
  - Flyback clamp, referenced to VIN, so it only ever sees the reflected voltage (1.223 V × R610/R601 ≈ 32 V) plus the turn-off spike.
  - Q601 peak drops from ≈ 200–215 V to ≈ 170 V at 76.5 V battery.
- **U605** → 120 pF 500 V NP0 0805 (C527211). **R618** → 100 Ω 2010 0.75 W (C421954). Footprints changed.
- **U609** → cjiang FAUL0630-1R0MT (C3040349, 1 µH, Isat 15 A). The body is 7 × 6.6 mm, so check the land pattern.
- **D402** → ElecSuper PESD1CAN-UX-ES (C19170883), a second source for PESD1CAN-UX.
- All other `Supplier Part` / MPN fields now match the parts used in the last JLC order. The KiCad BOM export reproduces that order, plus the PT1.5 parts.
- D610 (13 V zener across the iso output), D609 and Q601 are unchanged (reference design, no issues on PT1).

## Layout to-do
- Split the board-wide `GND` zones. The high-current area (FET tabs → J204, R229/R230, D216, D217) becomes `H_PWR-OUT-`, and logic keeps `GND`.
- Place NT201 outside the main 200 A path. It carries the bucks + iso-12 V primary return (≈1.4 A worst case, 60 W out at 54 V).
- BZ501 has no footprint assigned yet. Import C530415 (JLC loader) and check the + pad.
- RT601 has no footprint assigned yet. Import C105635 (JLC loader). Glue the disc, and keep C609, plastics and the temperature sensors clear of it (≈120 °C at 60 W).
- Place R628/R629 right at their buck's input caps, and F604 before C609.
- New footprints: U605 0805, R618 2010, U609 7 × 6.6 mm (C3040349).

## Firmware to-do
- Pins: `PIN_PRECHARGE_SENSE 1`, `PIN_HV_BUS_SENSE 2`, `PIN_BUZZER 38`.
- Arm sequence:
  1. Precharge on.
  2. Poll `PRECHARGE_SENSE` until it is below ~3 % of `HV_BUS_SENSE` (or below 2 V) within a timeout of about 1.5 s.
  3. Set the latch.
  4. Overlap.
  5. Precharge off.
  6. Confirm `PRECHARGE_SENSE` ≈ 0.
  7. Beep twice.
- On timeout: precharge off, don't set the latch, one long beep.
