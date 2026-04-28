# CBC_PCB — Caribou Battery Connector PCB

Caribou PCB project.

- GitHub issue: [#6 — Design BC-PCB for Caribou: 18S, 100A nominal, 200A spike](https://github.com/Arrow-air/project-caribou/issues/6)
- Starting point: Project Quiver BC-PCB KiCad design.
- Scope: 18S Tattu smart battery compatibility, rectangular PCB shape, 100A nominal current, 200A short-duration spike current, and regulated 12V/5V outputs wired into the Caribou main PCB.

## Folder layout

- `docs/` — board-specific notes, requirements, and design documentation.
- `firmware/` — board-specific firmware, configuration, or embedded code if needed.
- `images/` — board renders, screenshots, diagrams, and photos.
- `kicad/` — KiCad project files.
- `kicad/libs/` — project-local KiCad libraries.
- `kicad/libs/3dmodels/` — project-local 3D models.
- `kicad/libs/CBC_PCB.pretty/` — project-local footprint library.
- `kicad/libs/CBC_PCB.kicad_sym` — project-local symbol library.
- `manufacturing/` — generated manufacturing outputs such as Gerbers, BOM, assembly drawings, and pick-and-place files.

Add the KiCad project files directly under `kicad/` when design work starts. Keep project-specific components in `kicad/libs/` so the PCB can be opened without missing custom symbols, footprints, or 3D models.
