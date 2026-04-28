# PCB Projects

Each folder in this directory is one KiCad PCB project.

## Required PCB folder contents

- `<board>.kicad_pro` — KiCad project file.
- `<board>.kicad_sch` — schematic.
- `<board>.kicad_pcb` — layout.
- `symbols/` — project-local KiCad symbol libraries.
- `footprints/` — project-local KiCad footprint libraries.
- `3dmodels/` — project-local 3D models used by footprints.
- `production/` — generated manufacturing outputs such as Gerbers, BOM, and pick-and-place files.

Avoid relying on contributor-specific global KiCad libraries for project components. If a component is not from KiCad's standard libraries, include it locally in the PCB folder.
