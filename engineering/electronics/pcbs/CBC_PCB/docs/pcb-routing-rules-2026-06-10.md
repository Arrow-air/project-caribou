# CBC PCB routing rules update — 2026-06-10

Scope: rules/classes/via normalization only. No traces were rerouted and no placements or component values were changed. Existing routed copper that violates these rules should be fixed intentionally during routing, not by this cleanup commit.

## Assumptions

- Stackup: 6 layers, 2 oz copper.
- Trace-width estimates use IPC-2221-style current capacity as a floor, then round up to practical KiCad routing widths for margin and manufacturability.
- Unknown or ambiguous nets default to `Signal_Control`.
- `GND` and `GND_CD` are isolated domains. The only intended coupling path is D1 (`PESD1CAN-UX`), R34 (`1 MΩ`), and C33 (`100 pF`).

## Net classes

- `Signal_Control`: 0.15 mm track, 0.15 mm clearance, 0.45/0.25 mm via. I2C/SPI/UART/GPIO, feedback, compensation, bootstrap, enable, sense, USB data, and other unclear nets.
- `CAN_Battery`: 0.20 mm track, 0.20 mm clearance, 0.45/0.25 mm via. Unisolated battery CAN nets and local CAN-controller support nets.
- `CAN_Drone_Isolated`: 0.20 mm track, 0.25 mm clearance, 0.45/0.25 mm via. Isolated drone CAN nets in the `GND_CD` domain.
- `Power_0V5A`: 0.30 mm track, 0.20 mm clearance, 0.60/0.30 mm via. 12 V gate-voltage buck paths for U33/U34.
- `Power_1A`: 0.50 mm track, 0.25 mm clearance, 0.80/0.40 mm via. 5 V buck outputs U23/U35 and downstream 3.3 V LDO rails U24/U36.
- `Power_5A`: 1.50 mm track, 0.30 mm clearance, 1.20/0.60 mm via. High-current input/return and DC1961A isolated-converter power path.
- `GND_System`: 0.50 mm track, 0.25 mm clearance, 0.60/0.30 mm via. System ground domain.
- `GND_CD_Isolated`: 0.30 mm track, 0.25 mm clearance, 0.45/0.25 mm via. Isolated CAN Drone ground domain.

## Via presets

Board Setup via presets were standardized to:

- signal/CAN/stitching: 0.45 mm diameter / 0.25 mm drill
- 0.5 A power: 0.60 mm / 0.30 mm
- 1 A power: 0.80 mm / 0.40 mm
- 5 A power: 1.20 mm / 0.60 mm

Existing vias were normalized to the standard matching their assigned net class. No vias were added, removed, or moved.

## GND stitching strategy

Use 0.45/0.25 mm stitching vias for dense stitching of `GND` copper pours across all copper layers, especially near connector returns, switching regulators, board edges, and high-current loop boundaries. Keep stitching on `GND_CD` separate from `GND`; do not place shared vias or stitching that bridges the isolated domains.

## Isolation DRC

`CBC_PCB.kicad_dru` enforces 1.00 mm clearance between `GND` and `GND_CD`. Zone overlaps and shared vias between the two ground domains should be treated as DRC errors.

## Global manufacturing constraints

General DRC constraints use 0.15 mm minimum track/clearance, 0.25 mm minimum drilled hole, 0.10 mm minimum via annular ring, 0.25 mm hole-to-hole clearance, and 0.50 mm copper-to-edge clearance. The lower hole floor avoids incorrectly flagging existing connector pads while net-class via presets keep routing vias standardized.
