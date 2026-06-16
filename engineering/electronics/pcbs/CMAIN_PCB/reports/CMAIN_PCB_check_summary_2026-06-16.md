# CMAIN PCB KiCad Check Summary — 2026-06-16

Imported from Julius’s latest CMAIN KiCad update archive posted in Discord on 2026-06-16.

## Import cleanup

- Excluded local KiCad `.history/` and `CMAIN_PCB-backups/` directories from the repo import.
- Remapped undefined footprint graphics layer `Rescue` to `F.Fab` in `CMAIN_PCB.kicad_pcb` and `libs/EasyEDA_Lib.pretty/SMD_SMTSO3060CTJ.kicad_mod`.
- No `Rescue` layer references remain in the CMAIN KiCad project files.

## KiCad CLI checks

- Tool: KiCad CLI 10.0.1
- Schematic ERC: **0 violations**
- PCB DRC: **7 violations**, **0 unconnected items**

### DRC violation buckets

- lib_footprint_mismatch: 6
- text_thickness: 1

Raw JSON reports are saved next to this summary.
