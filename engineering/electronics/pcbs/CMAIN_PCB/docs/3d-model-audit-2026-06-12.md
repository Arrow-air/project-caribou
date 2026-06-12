# CMAIN 3D model audit — 2026-06-12

Audit scope: model references in `kicad/CMAIN_PCB.kicad_pcb` and local CMAIN footprint libraries.

## Result

- Total model references: 344
- Resolved after cleanup: 307
- Still unresolved: 37 references / 13 unique model paths

## Fixed in this pass

- Repointed stale KiCad 8 `.wrl` passive-component refs to KiCad 10 `.step` models:
  - `R_0805_2012Metric`
  - `C_0805_2012Metric`
  - `C_0603_1608Metric`
- Repointed the JLCPCB `D_SMB.step` ref to KiCad's built-in `Diode_SMD.3dshapes/D_SMB.step`.
- Repointed relative `NX3225GB-16M-STD-CRA-2.stp` refs to KiCad's built-in `Crystal_SMD_3225-4Pin_3.2x2.5mm.step`.
- Added reusable models already present elsewhere in the Caribou repo:
  - `EASYEDA_MODELS/C1206_L3.2-W1.6-H1.3.step`
  - `EASYEDA_MODELS/SMB_L4.6-W3.6-LS5.3-RD.step`
  - `EASYEDA_MODELS/SMC_L7.1-W6.2-LS8.1-RD.step`
  - `ALTIUM_EMBEDDED_MODELS/AT13-08PA-BM01.stp`
- Tried `dsa-t/jlc-kicad-lib-loader` against the CMAIN LCSC codes. The plugin is runnable, but EasyEDA's `searchByCodes` endpoint returned CloudFront `403 Request blocked` from this host.
- Used `JLC2KiCadLib` as a fallback against the same LCSC/JLC codes and copied the downloaded STEP models into `EASYEDA_MODELS/`.
- Replaced obvious standard-package missing refs with KiCad built-in equivalents:
  - 0805 LEDs
  - 0603/SOD-123 diodes
  - SOIC/SOP/QFN packages
  - JST-GH 4/5/6 pin connectors
  - generic screw/standoff bodies
  - Phoenix 2.5 mm 4/5/6-position terminal/header bodies
  - 8.5-9 mm magnetic buzzer body

## Local EasyEDA/JLC STEP models now present

- `C1206_L3.2-W1.6-H1.3.step`
- `CAP-SMD_L7.3-W4.3-RD_PA300LV227M0J.step`
- `CAP-TH_BD10.0-P5.00-D0.6-FD_2.step`
- `CAP-TH_D8.0-H8.0-P3.50.step`
- `CONN-SMD_GH1.25-LS-4P.step`
- `CONN-TH_5P-P2.50_ZX-XH2.54-5PZZ.step`
- `CONN-TH_SM03B-GHS-TB-LF-SN.step`
- `IND-SMD_CYA0650-3.3UH.step`
- `IND-SMD_L11.6-W10.1-H4.0.step`
- `POWERPAK-SO-8_L5.9-W4.9-P1.27-LS6.2-BL.step`
- `PWRM-TH_YLPTEC_URBXXXXS-6WR3.step`
- `R0402_L1.0-W0.5-H0.4.step`
- `SCREW-SMD_BD4.4-D2.8-H4.5.step`
- `SMA_L4.3-W2.6-LS5.0-FD.step`
- `SMB_L4.6-W3.6-LS5.3-RD.step`
- `SMC_L7.1-W6.2-LS8.1-RD.step`
- `SMD_BD5.6-L5.6-W5.6-D4.1.step`
- `SMD_SMTSO3060CTJ.step`
- `SOT-23-6_L2.9-W1.6-H1.5-LS2.8-P0.95.step`
- `USB-C-SMD_TYPEC-303-ACP16.step`

## Remaining unresolved model refs

These are the refs that still do not have a safe local/generic substitute. They are mostly connector/fuse/module parts where wrong geometry could hurt enclosure work more than no model.

```text
8   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/0154001.DR.stp
4   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/axk6s00547y.stp
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/SLW-913535-2A-SMT.stp
3   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/GIGABLOX_NANO_SWITCH_REVA_MODULE.step
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/LSHM-120-02.5-L-DV-A-S-K-TR.stp
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/SSM-120-F-DV.stp
3   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/XT30PW-F.step
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/1812L110_24DR.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/USB4215-03-A.stp
2   AT13-6P-BM03GRY.stp
1   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/REC20K-4805SZ.stp
1   0154001.DR.stp
1   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/REC30K-4812SZ.stp
```

Next step: add exact vendor STEP models for these remaining parts from SnapEDA/Samacsys/vendor downloads, or confirm an acceptable generic substitute for each connector/module.
