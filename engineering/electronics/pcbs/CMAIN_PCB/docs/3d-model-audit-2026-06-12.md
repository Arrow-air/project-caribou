# CMAIN 3D model audit — 2026-06-12

Audit scope: model references in `kicad/CMAIN_PCB.kicad_pcb` and local CMAIN footprint libraries.

## Result

- Total model references: 344
- Resolved after cleanup: 187
- Still unresolved: 157 references / 40 unique model paths

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

## Remaining unresolved model refs

These were not present in Julius's uploaded archive or the existing Caribou repo checkout used for this import.

```text
16  ${KICAD_3RD_PARTY}/Samacsys.3dshapes/APT2012LVBC_D.stp
11  ${KICAD_3RD_PARTY}/Samacsys.3dshapes/CPC1114N.stp
10  ${KIPRJMOD}/EASYEDA_MODELS/POWERPAK-SO-8_L5.9-W4.9-P1.27-LS6.2-BL.step
8   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/1778719.stp
8   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/SM04B-GHS-TB_LF__SN_.stp
8   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/0154001.DR.stp
7   ${KIPRJMOD}/EASYEDA_MODELS/CAP-TH_BD10.0-P5.00-D0.6-FD_2.step
6   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/M2x4.5.step
6   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/M3x8.step
6   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/M2.5x10.step
4   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/axk6s00547y.stp
4   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/PESD0603-240.stp
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/SLW-913535-2A-SMT.stp
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/SM05B-GHS-TB_LF__SN_.stp
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/CMT-8530S-SMT-TR.stp
3   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/GIGABLOX_NANO_SWITCH_REVA_MODULE.step
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/LSHM-120-02.5-L-DV-A-S-K-TR.stp
3   ${KIPRJMOD}/EASYEDA_MODELS/IND-SMD_CYA0650-3.3UH.step
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/SM06B-GHS-TB_LF__SN_.stp
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/1814935.stp
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/SSM-120-F-DV.stp
3   ${KICAD_3RD_PARTY}/Snapeda.3dshapes/XT30PW-F.step
3   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/1812L110_24DR.stp
3   ${KIPRJMOD}/EASYEDA_MODELS/CONN-TH_SM03B-GHS-TB-LF-SN.step
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/MCP2515-I_SO.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/TJA1049T_3J.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/TSYS01.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/USB4215-03-A.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/MMSZ5242B-E3-08.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/1778735.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/1814951.stp
2   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/1814948.stp
2   ${KIPRJMOD}/EASYEDA_MODELS/CONN-TH_5P-P2.50_ZX-XH2.54-5PZZ.step
2   ${KIPRJMOD}/EASYEDA_MODELS/CAP-SMD_L7.3-W4.3-RD_PA300LV227M0J.step
2   AT13-6P-BM03GRY.stp
1   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/REC20K-4805SZ.stp
1   0154001.DR.stp
1   ${KICAD_3RD_PARTY}/Samacsys.3dshapes/REC30K-4812SZ.stp
1   ${KIPRJMOD}/EASYEDA_MODELS/PWRM-TH_YLPTEC_URBXXXXS-6WR3.step
1   ${KIPRJMOD}/EASYEDA_MODELS/CAP-TH_D8.0-H8.0-P3.50.step
```

Next step: add the remaining vendor models from the original Samacsys/SnapEDA/EasyEDA exports, or replace those footprint model refs with equivalent KiCad built-in models where the package geometry is unambiguous.
