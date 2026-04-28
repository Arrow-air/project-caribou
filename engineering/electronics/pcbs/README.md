# PCB Projects

Each folder in this directory is one self-contained PCB project.

## Required PCB folder contents

```text
PCBNAME/
├── README.md
├── docs/
├── firmware/
├── images/
├── kicad/
│   └── libs/
│       ├── 3dmodels/
│       ├── PCBNAME.pretty/
│       └── PCBNAME.kicad_sym
└── manufacturing/
```

## KiCad conventions

- Put KiCad project files directly under `kicad/`.
- Put project-local symbols, footprints, and 3D models under `kicad/libs/`.
- Use `kicad/libs/PCBNAME.pretty/` for the board's local footprint library.
- Use `kicad/libs/PCBNAME.kicad_sym` for the board's local symbol library.
- Avoid relying on contributor-specific global KiCad libraries for project components. If a component is not from KiCad's standard libraries, include it locally in the PCB folder.
