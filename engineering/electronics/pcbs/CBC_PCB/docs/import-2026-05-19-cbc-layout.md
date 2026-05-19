# CBC_PCB KiCad layout import — 2026-05-19

Imported from Julius's `18SBC_PCB_19052026` package.

## Integration notes

- Renamed the KiCad root project files from `18SBC_PCB` to `CBC_PCB` to match the repository folder/project naming.
- Placed the KiCad source under `engineering/electronics/pcbs/CBC_PCB/kicad/`.
- Consolidated schematic symbols into `kicad/libs/CBC_PCB.kicad_sym` and rewrote non-power `lib_id` references to the `CBC_PCB` project library.
- Consolidated available discrete/custom footprint files into `kicad/libs/CBC_PCB.pretty/`.
- Moved included STEP/model assets under `kicad/libs/3dmodels/` and updated board model paths accordingly.
- Copied `CBC_PCB.step` and `fabrication-toolkit-options.json` to `manufacturing/`.

## Caveat

The zip did not include an `EasyEDA_Lib.elibz` source library even though the original project tables referenced it. The imported schematic sheets and board retain embedded EasyEDA symbol/footprint definitions, and the consolidated `CBC_PCB.kicad_sym` contains the embedded schematic symbols. If we want a fully reusable EasyEDA-derived footprint library later, export those footprints explicitly from KiCad/EasyEDA and add them to `CBC_PCB.pretty`.

## Import validation

Ran with KiCad CLI 10.0.1 after integration:

- `kicad-cli sch export netlist kicad/CBC_PCB.kicad_sch -o /tmp/cbc.net` succeeded.
- `kicad-cli pcb export step kicad/CBC_PCB.kicad_pcb -o /tmp/cbc_import.step` succeeded and produced a 57 MB STEP export.
- `kicad-cli sch erc` currently reports 1491 violations on the initial design, mostly dangling wires/labels.
- `kicad-cli pcb drc` currently reports 256 DRC violations and 115 unconnected items.
- The source package references `EASYEDA_MODELS/C1206_L3.2-W1.6-H1.3.step`, but that model file was not present in the zip; KiCad can still export STEP, with warnings for the affected C1206 capacitors.
