# Project Caribou 🦌

**A heavy-lift hexacopter designed for cargo logistics and precision agriculture.**

Project Caribou is an open-source initiative to develop a heavy-lift UAV capable of carrying up to **100 kg of payload** at a maximum take-off weight of approximately **200 kg**. It builds on lessons learned from Project Feather and Project Quiver, bringing Arrow back into the heavy-lift space.

This is an industrial-grade platform — prioritizing ruggedness, repairability, and cost-effectiveness over absolute weight savings.

---

## Mission Profile

| Parameter | Target |
|---|---|
| Configuration | Hexacopter (6 rotors) |
| Max Take-Off Weight | ~200 kg |
| Payload Capacity | ~100 kg |
| Hover throttle (full payload) | 50–60% |
| Power architecture | 18–24S (60–100V) |
| Flight controller | ArduPilot (Cube/Pixhawk series) |
| Telemetry range | >5 km |

**Primary use cases:**
- Heavy cargo logistics (~100 kg payload, multi-package delivery)
- Precision agriculture (up to 80L liquid spraying capacity)

---

## Design Philosophy

Caribou is designed for the field, not the lab.

- **Hybrid materials** — steel/aluminum central structure, carbon fiber or aluminum motor arms
- **Modular arms** — detachable in Phase 1, foldable in Phase 2 (fits a standard van or trailer)
- **Less welding over time** — CF tubes with 3D-printed aluminum/stainless connectors as the design matures
- **Triple-redundant avionics** — IMU, GPS, compass
- **Hard-landing rated gear** — landing gear must absorb impacts at full MTOW

---

## Project Phases

### Phase 1 — Proof of Concept *(Months 1–4)*
Validate the structural integrity of the core frame and power architecture. Focus on electronics integration. Milestone: stable **tethered hover flight**.

### Phase 2 — Scalable Design Overhaul *(Months 5–10)*
Transform the test frame into a deployable prototype. DAO vote on best structural path forward. Explore 50 kg and 100 kg payload variants in parallel. Build a second prototype in the US. Milestone: **tethered hover flights with payload** (liquid + cargo configs).

### Phase 3 — Field Readiness *(Months 11–16)*
Refine UX, reduce weight, finalize payload operation. Milestone: **2 beta units** for partner testing in real-world scenarios.

### Phase 4 — Documentation *(Months 17–18)*
Finalize engineering report, BOM, assembly guides, flight logs, operating manual.

---

## Repository Structure

```
project-caribou/
├── src/                    # CAD models, schematics, design files
├── docs/
│   ├── ADRs/               # Architecture Decision Records
│   └── meetings/           # Meeting notes and summaries
├── CONTRIBUTING.md
└── README.md
```

---

## Governance

Project Caribou is funded and governed through the Arrow DAO.

- **Project Lead:** Julius ([@far1no](https://github.com/far1no))
- **Budget:** $30,000/month ($25k labor + $5k hardware); unspent funds do not roll over
- **Multisig:** Project Lead + GBC
- **Phase gates:** Public DAO vote required to proceed to each next phase
- **AIP:** Added to AIP-007 as an ACTIVE project upon approval

---

## Deliverables

Each phase produces:
- Engineering Reports at major milestones
- CAD models, PCB layouts, and software in this repository
- Assembly and manufacturing guides
- Meeting summaries with decisions and action items
- Structured documentation with unique identifiers for version control

---

## Regulatory Strategy

A 200 kg+ MTOW aircraft falls under:
- **EU (EASA):** Specific Category, requiring SORA + LUC for scale
- **US (FAA):** Section 44807 Exemption + Part 137 for agriculture

This project lays technical groundwork for individual flight permits. Full commercial type certification is out of scope for the current phase and will be assessed once specific partners are secured.

---

## Community

- **Discord:** [Arrow Community Server](https://discord.com/invite/arrow) — `#project-caribou-general`
- **Forum:** [Project Caribou Discussion](https://dao.arrowair.com/t/project-caribou-discussion-heavy-lift-hexacopter/150)
- **Contributing:** See [CONTRIBUTING.md](./CONTRIBUTING.md)

---

## Related Projects

| Project | Description |
|---|---|
| [Project Quiver](https://github.com/Arrow-air/project-quiver) | 25 kg open-source VTOL platform |
| [Project Quiver Mini](https://github.com/Arrow-air/project-quiver-mini) | Compact 7 kg sensor platform |
| [Project Spearhead](https://github.com/Arrow-air/project-spearhead) | Hybrid propulsion testbed |

---

*Project Caribou is an [Arrow DAO](https://arrowair.com) open-source initiative. Licensed under [CERN-OHL-S v2](https://ohwr.org/cern_ohl_s_v2.txt) (hardware) and [GPL-3.0](https://www.gnu.org/licenses/gpl-3.0.html) (software).*
