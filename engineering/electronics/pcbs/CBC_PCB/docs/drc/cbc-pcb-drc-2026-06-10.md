# CBC PCB DRC report — 2026-06-10

Generated with `kicad-cli pcb drc --format json --severity-all --all-track-errors`.

- Violations: 327
- Unconnected items: 91
- Custom `GND` ↔ `GND_CD` isolation-rule clearance violations: 55

## Violation counts

- `clearance`: 265
- `silk_overlap`: 22
- `copper_edge_clearance`: 8
- `lib_footprint_issues`: 7
- `starved_thermal`: 4
- `via_dangling`: 4
- `silk_over_copper`: 4
- `hole_clearance`: 3
- `shorting_items`: 2
- `track_dangling`: 2
- `silk_edge_clearance`: 2
- `courtyards_overlap`: 1
- `solder_mask_bridge`: 1
- `missing_courtyard`: 1
- `lib_footprint_mismatch`: 1

## Notes

This report intentionally flags existing layout/routing violations after the updated net classes, via standards, and isolation rules. No traces were rerouted as part of this cleanup.

The custom `GND` ↔ `GND_CD` isolation rule is active in this report. It currently includes the intentional ESD/bleed coupling area around D1/R34/C33, so review/waive that intentional coupling deliberately if keeping the 1.00 mm blanket clearance rule.
