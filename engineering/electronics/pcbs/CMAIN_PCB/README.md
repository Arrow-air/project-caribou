# CMAIN_PCB — Caribou Main PCB

Caribou low-voltage main PCB project.

- GitHub issue: [#8 — Design Caribou main PCB: merge Feather PDB and Quiver main PCB](https://github.com/Arrow-air/project-caribou/issues/8)
- Starting points: Project Feather PDB and Project Quiver main PCB.
- Scope: low-voltage/signal-level avionics integration, 6-motor outputs, ArduPilot/CAN architecture, ESC signal outputs, sensor connections, and central-frame form factor. High-voltage/current handling is out of scope.

## Folder layout

- `symbols/` — project-local symbols.
- `footprints/` — project-local footprints.
- `3dmodels/` — project-local 3D models.
- `production/gerbers/` — generated Gerber outputs.
- `production/bom/` — generated BOM outputs.
- `production/pick-place/` — generated pick-and-place outputs.

Add the KiCad project files directly in this folder when design work starts.
