# CMAIN PCB KiCad check summary — 2026-06-23

Input archive: Julius Discord upload `CMAIN_PCB.7z` from 2026-06-23.

Cleanup performed:

- Integrated the updated CMAIN KiCad files and native project-local footprints from the archive.
- Removed the `EasyEDA_Lib_CMAIN` EasyEDA/JLCEDA Pro plugin library entry and loader artifact so KiCad only needs project-local native KiCad libraries plus standard KiCad libraries.
- Moved the new TH162050CTJ standoff symbol/footprint into the project-local `CMAIN_PCB` KiCad library namespace.
- Remapped undefined `Rescue` layer references to `F.Fab` in the board/footprint source files.
- Excluded KiCad `.history` and `CMAIN_PCB-backups` archive files from import.

Verification with KiCad CLI 10.0.1:

- `kicad-cli sch erc --format json engineering/electronics/pcbs/CMAIN_PCB/kicad/CMAIN_PCB.kicad_sch`
  - 16 ERC violations remain, all `pin_to_pin` warnings from unspecified-pin imported symbols.
  - 0 `lib_symbol_issues` remain.
- `kicad-cli pcb drc --format json engineering/electronics/pcbs/CMAIN_PCB/kicad/CMAIN_PCB.kicad_pcb`
  - 96 DRC violations remain: 53 `silk_overlap`, 41 `silk_over_copper`, 1 `lib_footprint_mismatch`, 1 `text_thickness`.
  - 0 unconnected items.

No live `Rescue` or `EasyEDA_Lib_CMAIN` references remain in the KiCad project files after cleanup.
