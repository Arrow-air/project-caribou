# Project Caribou 🦌

**An open-source heavy-lift hexacopter for cargo logistics and precision agriculture.**

Project Caribou is a ~200 kg MTOW drone with ~100 kg payload capacity, designed for ruggedness, field repairability, and cost-effective manufacturing. Built on lessons from Project Feather and Project Quiver.

Developed by [Arrow Air](https://arrowair.com) and released under the CERN Open Hardware Licence.

## Key Specs

| Parameter | Target |
|---|---|
| Configuration | Hexacopter (6 rotors) |
| Max Take-Off Weight | ~200 kg |
| Payload Capacity | ~100 kg |
| Power architecture | 18–24S (60–100V) |
| Flight controller | ArduPilot (Cube/Pixhawk series) |
| Telemetry | >5 km |

## Key Features

- **Heavy cargo & agriculture** — 80L liquid spraying capacity or multi-package cargo bay
- **Hybrid material airframe** — steel/aluminum core with CF or aluminum motor arms
- **Modular arms** — detachable for transport, foldable in later revisions
- **Redundant avionics** — triple-redundant IMU, GPS, and compass
- **ArduPilot firmware** — standard QGroundControl/Mission Planner compatibility
- **High-voltage safety** — external HV kill switch + remote electronic circuit breaker

## Repository Structure

```
project-caribou/
├── src/                    # CAD models, schematics, design files
├── docs/
│   ├── ADRs/               # Architecture Decision Records
│   └── meetings/           # Meeting notes and summaries
└── CONTRIBUTING.md
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Community

- **Discord** — [discord.gg/arrow](https://discord.gg/arrow) — `#project-caribou-general`
- **DAO Forum** — [dao.arrowair.com](https://dao.arrowair.com)

## License

[CERN Open Hardware Licence Version 2 - Strongly Reciprocal (CERN-OHL-S)](LICENSE)
