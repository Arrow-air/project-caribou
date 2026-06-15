# CMAIN PCB routing rules

Generated for the KiCad 10 CMAIN layout cleanup on 2026-06-15, following the CBC PCB rule-normalization workflow.

## Scope

- Added project net classes and explicit net assignments for all named CMAIN board nets.
- Added standard track-width and via presets.
- Normalized existing routed segment widths and via sizes to their assigned net-class values.
- Did not intentionally move footprints, tracks, vias, zones, or board geometry.

## Net classes

- `Signal_Control`: 0.20 mm tracks, 0.60 / 0.30 mm vias, 0.20 mm clearance.
- `CAN_Bus`: 0.20 mm tracks, 0.60 / 0.30 mm vias, 0.20 mm clearance, 0.20 mm diff-pair width/gap.
- `Ethernet`: 0.20 mm tracks, 0.60 / 0.30 mm vias, 0.20 mm clearance, 0.20 mm diff-pair width/gap.
- `Power_0V5A`: 0.30 mm tracks, 0.60 / 0.30 mm vias, 0.20 mm clearance.
- `Power_1A`: 0.50 mm tracks, 0.80 / 0.40 mm vias, 0.25 mm clearance.
- `Power_5A`: 1.00 mm tracks, 1.20 / 0.60 mm vias, 0.30 mm clearance.
- `GND_System`: 0.30 mm tracks, 0.60 / 0.30 mm vias, 0.25 mm clearance.

## Assignment heuristics

- `GND` → `GND_System`.
- `CAN*` and `/CAN*` nets → `CAN_Bus`.
- `ETH*` and RJ45 `J47 P1*_P/N` pair nets → `Ethernet`.
- `12V_*`, `12VSW`, and `+12V_PL` → `Power_5A`.
- `5V_*`, `+3.3V`, `/IO_VDD_3V3`, `/VBUS`, and `*3V3*` nets → `Power_0V5A`.
- Remaining named nets → `Signal_Control`.

## Change summary

- Named nets assigned: 539.
- Routed segments seen: 1,648 total, 1,641 named.
- Segment widths normalized: 516.
- Vias seen: 203 named.
- Via sizes normalized: 14.
- Track/via geometry check: segment and via counts unchanged; start/end/layer/net/UUID content is unchanged when width, size, and drill fields are ignored.

## DRC snapshot

KiCad CLI command:

```bash
kicad-cli pcb drc engineering/electronics/pcbs/CMAIN_PCB/kicad/CMAIN_PCB.kicad_pcb --format json --output /tmp/cmain-drc-after-rules.json
```

Before rule/width normalization:

- 404 violations
- 391 unconnected items
- Top categories: 131 `silk_over_copper`, 71 `track_dangling`, 55 `silk_overlap`, 47 `via_dangling`, 29 `text_height`, 24 `copper_edge_clearance`

After rule/width normalization:

- 634 violations
- 391 unconnected items
- Top categories: 201 `clearance`, 131 `silk_over_copper`, 75 `silk_overlap`, 71 `track_dangling`, 46 `via_dangling`, 29 `text_height`, 24 `copper_edge_clearance`

The increased DRC count is expected from widening existing copper and enabling stricter clearance/silk/mask settings. No rerouting was done in this pass.
