# C90D_PCB — Caribou 90° Battery Adapter PCB

The Caribou 90° adapter PCB (`C90D_PCB`) sits between the main battery connector and the `CBC_PCB`. It adapts the battery connector orientation into the CBC assembly while carrying the propulsion power path for a Hobbywing XRotor X15-based motor arm.

- **GitHub issue:** [#19 — Create C90D_PCB - An 90° adapter PCB interacting with the CBC_PCB and the battery](https://github.com/Arrow-air/project-caribou/issues/19)
- **Related PCB:** [`CBC_PCB`](../CBC_PCB/)
- **EDA Tool:** KiCad 10

## Board Purpose

`C90D_PCB` is a mechanical and electrical adapter:

1. **Battery-side interface** — mates with the main battery connector.
2. **CBC-side interface** — presents a right-angle power connection into the `CBC_PCB` assembly.
3. **High-current path** — carries the Hobbywing X15 drive current.
4. **Mechanical support** — includes mounting holes so the adapter and connector assembly can be secured.

## Main Parts / Connectors

- Main battery connector: `ACES 59626`
- CAN connector: `BM02B-GHS-TBT(LF)(SN)`
- Right-angle screw terminal / power interface to the `CBC_PCB`: `OT-201-M8`

## Initial Requirements

| Parameter | Target / Notes |
|---|---|
| Function | 90° adapter between battery connector and `CBC_PCB` |
| Propulsion load | Hobbywing XRotor X15 current draw |
| Current design target | 150 A, pending detailed validation |
| Mounting | Include mounting holes for the PCB / connector assembly |
| Board constraints | Copper weight, current path geometry, connector ratings, thermal rise, and mechanical assembly must be checked against the 150 A target |

## Reference Renderings

The initial GitHub issue includes basic renderings showing the main component arrangement and intended 90° adapter concept. Treat these as layout/mechanical references for early design work; detailed placement and mounting geometry still need validation in CAD/KiCad.

- [Rendering 1](https://github.com/user-attachments/assets/765d904a-0705-4a9c-978a-a3602e707b37)
- [Rendering 2](https://github.com/user-attachments/assets/26fbdbea-c981-408a-b00c-e627d38056d7)

## Folder Layout

- `docs/` — board-specific notes, requirements, and design documentation.
- `firmware/` — board-specific firmware, configuration, or embedded code if needed.
- `images/` — board renders, screenshots, diagrams, and photos.
- `kicad/` — KiCad project files.
- `kicad/libs/` — project-local KiCad libraries.
- `kicad/libs/3dmodels/` — project-local 3D models.
- `kicad/libs/C90D_PCB.pretty/` — project-local footprint library.
- `kicad/libs/C90D_PCB.kicad_sym` — project-local symbol library.
- `manufacturing/` — generated manufacturing outputs such as Gerbers, BOM, assembly drawings, and pick-and-place files.

Add the KiCad project files directly under `kicad/` when design work starts. Keep project-specific components in `kicad/libs/` so the PCB can be opened without missing custom symbols, footprints, or 3D models.
