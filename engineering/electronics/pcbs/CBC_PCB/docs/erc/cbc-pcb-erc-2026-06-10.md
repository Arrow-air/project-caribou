# CBC PCB ERC report — 2026-06-10

Generated with `kicad-cli sch erc --format json --severity-all` after simple ERC cleanup.

- ERC violations: 0

## Fixes applied

- Added missing standard footprint-library entries for `Resistor_SMD` and `Capacitor_SMD`.
- Vendored the referenced Samacsys symbol/footprint files locally and pointed the CBC library tables at them.
- Corrected imported passive-part pin metadata from `unspecified`/`input` to `passive` for affected embedded symbols.
- Added one `PWR_FLAG` to the CAN sheet `+3.3V` rail so KiCad ERC sees the rail as driven.

No component values, footprints, placements, or schematic connectivity were intentionally changed, except adding the ERC-only `PWR_FLAG` symbol.
