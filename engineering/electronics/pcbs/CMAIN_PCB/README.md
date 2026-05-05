# CMAIN_PCB — Caribou Main PCB

Caribou PCB project.

- GitHub issue: [#8 — Design Caribou main PCB: merge Feather PDB and Quiver main PCB](https://github.com/Arrow-air/project-caribou/issues/8)
- Starting point: Project Feather PDB and Project Quiver main PCB.
- Scope: low-voltage/signal-level avionics integration, 6-motor outputs, ArduPilot/CAN architecture, ESC signal outputs, sensor connections, and central-frame form factor. High-voltage/current handling is out of scope.


## Connector Requirements

Initial connector requirements are tracked in [`docs/connector-requirements.md`](docs/connector-requirements.md). Current assumptions include:

- 6× CBC_PCB interfaces using `AT13-12PB-BM03` 12-pin connectors.
- 6× ESC interfaces using `AT13-6P-BM01GRY` 6-pin connectors with `CAN H`, `CAN L`, `PWM`, `GND`, `GND`, and `NC`.
- Additional external sensor / attachment connectors are still TBD pending board-space and component-placement checks.

## Folder layout

- `docs/` — board-specific notes, requirements, and design documentation.
- `firmware/` — board-specific firmware, configuration, or embedded code if needed.
- `images/` — board renders, screenshots, diagrams, and photos.
- `kicad/` — KiCad project files.
- `kicad/libs/` — project-local KiCad libraries.
- `kicad/libs/3dmodels/` — project-local 3D models.
- `kicad/libs/CMAIN_PCB.pretty/` — project-local footprint library.
- `kicad/libs/CMAIN_PCB.kicad_sym` — project-local symbol library.
- `manufacturing/` — generated manufacturing outputs such as Gerbers, BOM, assembly drawings, and pick-and-place files.

Add the KiCad project files directly under `kicad/` when design work starts. Keep project-specific components in `kicad/libs/` so the PCB can be opened without missing custom symbols, footprints, or 3D models.
