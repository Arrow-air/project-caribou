# LTC4370 12V Redundancy Proposal

Purpose: replace the old HV-derived power-supply page with redundant 12V input combining from six Caribou CBC PCBs.

## Requirement

CMAIN receives six independent `12V / 10A` feeds from CBC PCBs. The PCB should combine them into redundant, load-share-capable 12V rails using external MOSFETs and a dedicated controller IC.

## Proposed first-revision topology

Use three identical dual-input combiner channels. Each channel uses one LTC4370, two external N-channel MOSFETs, and two sense resistors.

```text
CBC_12V_1 ─ fuse/eFuse ─ Rsense ─ N-MOSFET ┐
                                           ├─ LTC4370_CH1_OUT = 12V_CH1
CBC_12V_4 ─ fuse/eFuse ─ Rsense ─ N-MOSFET ┘

CBC_12V_2 ─ fuse/eFuse ─ Rsense ─ N-MOSFET ┐
                                           ├─ LTC4370_CH2_OUT = 12V_CH2
CBC_12V_5 ─ fuse/eFuse ─ Rsense ─ N-MOSFET ┘

CBC_12V_3 ─ fuse/eFuse ─ Rsense ─ N-MOSFET ┐
                                           ├─ LTC4370_CH3_OUT = 12V_CH3
CBC_12V_6 ─ fuse/eFuse ─ Rsense ─ N-MOSFET ┘
```

Pairing `1+4`, `2+5`, `3+6` avoids grouping all feeds from one physical side/battery/CBC cluster into a single bus, assuming the CBC numbering maps sensibly across the aircraft. If physical numbering differs, keep the topology but pair opposite/independent CBCs.

## Why dual-input channels instead of 3-input buses

The LTC4370 is a two-supply current-sharing controller. A 3-input bus would require cascading or mixing controller stages, which can create less obvious current-share behavior. Three independent dual-input channels are simpler to reason about, easier to lay out symmetrically, and still provide redundancy: each output channel survives loss of either of its two CBC feeds.

## Downstream power distribution

Recommended downstream nets:

```text
12V_CH1 ── loads group 1
12V_CH2 ── loads group 2
12V_CH3 ── loads group 3
```

For flight-controller power, add two independent 5V regulator paths so the FC can receive redundant power:

```text
12V_CH1 ─ 5V_REG_A ─ FC_5V_MAIN      / legacy +5V
12V_CH2 ─ 5V_REG_B ─ FC_5V_BACKUP    / legacy VDD5V_BRICK2
```

Keep these two 5V outputs independent if the FC has separate main/backup power inputs. If the selected FC input requires one rail, OR/power-mux them downstream rather than shorting regulator outputs directly.

For critical 12V avionics or control power, optionally add a second OR-ing stage:

```text
12V_CH1 ─ ideal diode/eFuse ┐
12V_CH2 ─ ideal diode/eFuse ├─ CRITICAL_12V
12V_CH3 ─ ideal diode/eFuse ┘
```

This keeps the LTC4370 stages focused on pairwise current sharing, while critical rails get N+ redundancy from the three paired channels.

## LTC4370 design facts relevant to this board

Source: Analog Devices LTC4370 datasheet.

- Two-supply diode-OR current-balancing controller.
- Works from `2.9V` to `18V` on `VIN1/VIN2`, so 12V feeds are in range.
- Uses one external N-channel MOSFET per input.
- Blocks reverse current and input-fault backfeed.
- Equal sense resistors produce equal current sharing between the two inputs.
- Unequal sense resistor ratio can intentionally bias sharing.
- Sharing capture range is set by `RANGE`; open gives up to about ±0.6V, while resistor-to-ground sets a smaller range.
- `RANGE` tied to `VCC` disables sharing and makes it act as dual ideal diode OR-ing only.
- `FETON1/FETON2` can be routed to test points or MCU/status headers.
- `EN1/EN2` can be pulled active by default and optionally controlled/disabled.
- Kelvin routing from sense resistors to `OUT1/OUT2` is important for current-share accuracy.

## Straw-man component values

These should be checked against final current/thermal/layout assumptions.

Per input path:

- Input connector: CBC 12V + GND
- Fuse/eFuse: 10A nominal per CBC feed, sized to CBC and harness capability
- TVS diode: 12V automotive-ish transient protection to GND, exact part TBD
- Sense resistor: `2.5 mΩ`, >= appropriate pulse/thermal rating
  - At 10A: 25mV sense drop, 0.25W dissipation
  - Use Kelvin pads/traces
- MOSFET: N-channel, low Rds(on), >=30V Vds, Vgs max compatible with LTC4370 gate drive
  - Target Rds(on) around 2–5mΩ depending thermal budget
- Local capacitance: per LTC4370 datasheet around VCC/CPO/COMP plus output bulk cap per channel

For 10A/channel-side operation, sense resistor and MOSFET heat are not trivial. Layout needs copper pours, thermal vias, and short high-current loops.

## Nets to add on CMAIN power page

Inputs:

- `CBC_12V_IN_1`
- `CBC_12V_IN_2`
- `CBC_12V_IN_3`
- `CBC_12V_IN_4`
- `CBC_12V_IN_5`
- `CBC_12V_IN_6`
- `GND`

Intermediate/output rails:

- `12V_CH1`
- `12V_CH2`
- `12V_CH3`
- optional `CRITICAL_12V`

Status/control:

- `CH1_IN_A_FETON`, `CH1_IN_B_FETON`
- `CH2_IN_A_FETON`, `CH2_IN_B_FETON`
- `CH3_IN_A_FETON`, `CH3_IN_B_FETON`
- optional `CHx_EN_A/B`
- optional current monitor test pads across each sense resistor

## Open decisions

1. Do we want three independent 12V output channels, or should they be ORed again into one/few downstream rails?
2. What are the actual CMAIN loads and max current per 12V channel?
3. Should each CBC input have a discrete fuse, resettable fuse, or active eFuse before LTC4370?
4. Should CMAIN report per-input status to a controller, or are test points/LEDs enough?
5. Confirm CBC numbering/physical pairing before locking `1+4`, `2+5`, `3+6`.

## Recommended next step

Create one reusable schematic block for `LTC4370_DUAL_12V_COMBINER` and instantiate it three times on `power_supply.kicad_sch`. Keep old HV converter circuitry disconnected/removed after the new block validates in ERC.
