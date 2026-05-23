# CBC_PCB — Schematic Review Follow-up

This page tracks the accepted follow-up items from the CBC schematic review pass.

## Documentation approach

Keep design intent in three project-local places:

1. `README.md` — stable board overview, sheet interface summary, high-level architecture.
2. `docs/connector-pinout.md` — connector and harness-facing behavior, including wiring/domain notes.
3. `docs/power-architecture.md` — power tree, protection, fusing, redundancy, and routing-critical power notes.

Short-lived implementation reminders from schematic review are tracked below until they are either implemented in KiCad or promoted into the stable docs.

## Review follow-up table

| Review item | Decision / action |
|---|---|
| 4 — Main current hardware protection | Main fuse is mounted on J4/J5 using the bolt-in AMT0650009DB0000G fuse terminal arrangement. Documented as the primary high-current short-circuit protection. |
| 5 — Kill/off fail-safe documentation | Document the hardware kill behavior, latch behavior, external `12V_TRIGGER_SIG`, and expected recovery/re-arm sequence in the power architecture docs. |
| 6 — `12V_ISOLATED` short protection | Add another `0154005.DR` fuse to the isolated 12V output path before the AT13 connector / harness output. |
| 7 — ESP GPIO latched enable safety | `ESP_ENABLE_SIGNAL` already has a 10k pulldown (`R16`). Keep this documented as the default-off state for the latch input. |
| 8 — USB back-powering `+5V` | Accepted as low risk because USB mostly powers ESP32/debug logic. No schematic change required. |
| 9 — ESP32 `GPIO0` pull-up | Add/document an external pull-up option on `GPIO0` so the ESP32 boot strap is deterministic even with USB-UART/autoprogramming circuitry attached. |
| 10 — MCP2515 `CS#1` / `CS#2` pull-ups | Add pull-ups to keep both MCP2515 chip-select lines inactive during ESP32 reset/boot. |
| 11 — MCP2515 `INT#1` / `INT#2` pull-ups | Document that the MCP2515 interrupt outputs are used with ESP32 pull-ups unless the schematic later adds external pull-ups. Confirm MCP2515 output mode during firmware bring-up. |
| 12 — USB layout | Routing task: route USB D+/D− as a short matched differential pair, avoid stubs, and keep ESD/protection close to connector if added. |
| 13 — J2 ground-domain mixing | Intentional. Document harness wiring so installers understand which ground/domain each pin belongs to. |
| 14 — CAN isolated-side reference strategy | Battery CAN exposes only `CANH_BAT`/`CANL_BAT`; `GND_CA` is not normally carried to the battery. Drone CAN `GND_CB` may be carried to CMAIN_PCB and tied with the other CBC `GND_CB` references there. |
| 15 — CAN termination | Termination is jumper-selectable and normally deactivated by default. |
| 16 — J2 / AT13 footprint pin numbering | Still requires manual review against connector datasheet and harness pinout before manufacturing release. |
| 17 — Connector docs | Connector and harness notes updated in `connector-pinout.md`. |
| 18 — Sheet interfaces | Added a top-level sheet interface table to `README.md`. |
| 19 — `DATA1` / `DATA2` naming | Renamed to `TEMP1_DQ` / `TEMP2_DQ`. |
| 20 — Label direction cleanup | Cleaned global label directions for SPI, MCP2515 interrupt/chip-select, CAN bus, and 1-Wire temperature nets. |
| 21 — Custom power symbols | Keep current custom/renamed power symbols for now. |
| 22 — PWR_FLAG / power pin modeling | ERC/power modeling reviewed after changes. Keep PWR_FLAGs only where they represent an intentional supply source/modeling boundary. |
| 23 — DS18B20 warning | Footprint/filter warning is intentionally excluded from checks. |

## Notes for next schematic/layout pass

- Add `0154005.DR` fuse on the isolated 12V output.
- Add pull-ups on `CS#1` and `CS#2`.
- Decide whether `GPIO0` gets a fitted pull-up or a DNI pull-up option.
- Keep CAN termination jumper-selectable and default-open.
- Add USB routing constraints to the PCB layout checklist.
