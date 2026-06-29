# CMAIN PCB KiCad check summary - 2026-06-29

Input archive: Julius Discord upload `CMAIN_PCB_2906.7z` from 2026-06-29.

Update integrated:

- Imported the latest CMAIN KiCad schematic, PCB layout, project-local symbols/footprints, 3D model updates, and STEP export.
- Added the rounded-tracks config file from the archive.
- Added prototype-ordering output under `kicad/production/`.
- Excluded KiCad `.history`, `CMAIN_PCB-backups`, lockfiles, `.bak` files, and local `jlcpcb/` cache files from the repo import.
- Left the EasyEDA/JLCEDA Pro loader artifact out of the repo; the KiCad project still uses native project-local KiCad symbol and footprint libraries.

Verification with KiCad CLI 10.0.1:

- `kicad-cli sch erc --format json engineering/electronics/pcbs/CMAIN_PCB/kicad/CMAIN_PCB.kicad_sch`
  - 0 ERC violations remain.
  - 0 `lib_symbol_issues` remain.
- `kicad-cli pcb drc --format json engineering/electronics/pcbs/CMAIN_PCB/kicad/CMAIN_PCB.kicad_pcb`
  - 2 DRC violations remain:
    - 1 `silk_edge_clearance`: the back-silkscreen warning text touches/clips the board edge clearance.
    - 1 `lib_footprint_mismatch`: `D8` footprint `SMA_L4.3-W2.6-LS5.0-FD` differs from the copy in library `EasyEDA_Lib`.
  - 0 unconnected items.

Local validation note: KiCad reported local font substitution for `Karla` during DRC (`Skia Bold` used locally).
