# CBC PCB DRC report — 2026-06-10

Generated with `kicad-cli pcb drc --format json --severity-all --all-track-errors` after applying net classes, via normalization, existing routed-segment width normalization, and simple ERC cleanup.

- Violations: 511
- Unconnected items: 92
- Custom `GND` ↔ `GND_CD` isolation-rule clearance violations: 55

## Violation counts

- `clearance`: 387
- `shorting_items`: 48
- `solder_mask_bridge`: 22
- `silk_overlap`: 22
- `copper_edge_clearance`: 8
- `via_dangling`: 5
- `starved_thermal`: 4
- `silk_over_copper`: 4
- `lib_footprint_mismatch`: 3
- `hole_clearance`: 2
- `track_dangling`: 2
- `silk_edge_clearance`: 2
- `courtyards_overlap`: 1
- `missing_courtyard`: 1

## Notes

This report intentionally flags existing layout/routing violations after the updated net classes, via standards, isolation rules, and routed-segment width normalization. Existing trace paths were not rerouted.

The custom `GND` ↔ `GND_CD` isolation rule is active in this report. It currently includes the intentional ESD/bleed coupling area around D1/R34/C33, so review/waive that intentional coupling deliberately if keeping the 1.00 mm blanket clearance rule.
