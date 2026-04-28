# CBC_PCB — Caribou Battery Connector PCB

Caribou battery connector PCB project.

- GitHub issue: [#6 — Design BC-PCB for Caribou: 18S, 100A nominal, 200A spike](https://github.com/Arrow-air/project-caribou/issues/6)
- Starting point: Project Quiver BC-PCB KiCad design.
- Scope: 18S Tattu smart battery compatibility, rectangular PCB shape, 100A nominal current, 200A short-duration spike current, and regulated 12V/5V outputs wired into the Caribou main PCB.

## Folder layout

- `symbols/` — project-local symbols.
- `footprints/` — project-local footprints.
- `3dmodels/` — project-local 3D models.
- `production/gerbers/` — generated Gerber outputs.
- `production/bom/` — generated BOM outputs.
- `production/pick-place/` — generated pick-and-place outputs.

Add the KiCad project files directly in this folder when design work starts.
