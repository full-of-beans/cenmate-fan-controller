# CENMATE schematic-to-PCB parity audit

**November 6, 2026 · CONDITIONAL PASS for parity · Fabrication hold**

Repository `full-of-beans/cenmate-fan-controller`, [PR #6](https://github.com/full-of-beans/cenmate-fan-controller/pull/6), branch `initial-schematic` against `main`. Starting HEAD `bc3c21993f01a6a4f2766652c139b9bf81b43472`. The final audited tree is the commit introducing this report. Its PCB blob is `aabca82088d528504e7063db59f2fa2a5302db2f`. The remaining active design inputs are identical to the starting tree. The authoritative project is `hardware/fan-controller.kicad_pro` and its sibling schematic, board, and rules. The earlier [controller schematic verification](controller-schematic-verification.md) documents a superseded design.

Validation used KiCad CLI **10.0.8** (Ubuntu package build `10.0.8~ubuntu24.04.1`) in an isolated Linux validation environment with KiCad 10 libraries and project-local `FanController` tables. The native XML export and IPC-D-356 export succeeded. The referenced local symbol definitions exist in the local symbol library. All placed custom footprints, including the four mounting holes, matched their local definitions for pad numbers, types, sizes, and drills. Native ERC/DRC reported zero symbol or footprint library findings. No physical hardware was used.

| Check | Result | Evidence |
|---|---|---|
| Component parity | Pass | 30 schematic components and 30 linked PCB footprints. References, symbol UUID links, values, manufacturer, MPN, DigiKey part number, and BOM/DNP/board population agree. All 30 are included. Four grounded mounting holes are deliberate PCB-only objects. |
| Footprint parity | Pass | All 30 footprint IDs agree. Local library assignments resolve, including the custom J2, XIAO and TO-92 footprints. |
| Pin/pad parity | Pass | 91 numbered relationships, including seven explicit NC pins. Four additional GND pads belong to the mechanical holes. |
| Net connectivity | Pass | 27 schematic net groups, including the seven singleton NC nets. Exact per-pin names and endpoint sets match the board. Native IPC-D-356 independently corroborates all 95 numbered board pads, including the mechanical pads. Native DRC reports zero opens and zero shorts with saved and freshly refilled zones. |
| Native synchronization | Pass for CLI parity. F8 preview unverified. | KiCad `pcb drc --schematic-parity` reported seven warnings initially and zero after the mechanical metadata fix. It compares the existing design. The GUI **Update PCB from Schematic** preview and its proposed action list were outside the executed checks. |
| ERC | Pass | Fresh 0 errors, 0 warnings, 0 exclusions. Agrees with the committed ERC count. |
| DRC | Pass | Fresh 0 physical violations, 0 unconnected items, 0 parity findings, 0 exclusions with project settings. The committed DRC also shows 0 physical violations and opens. Fresh checks explicitly included schematic parity. |

## Findings and disposition

The starting board generated **seven native parity warnings from four instances of one metadata issue**. The four intentional grounded M2 holes shared the placeholder reference `REF**` and lacked the PCB-only flag. KiCad reported four extra-footprint warnings and three duplicate-reference warnings. Assigned unique hidden references H1–H4 and set the board-only flag so forward annotation can recognize their purpose. The hole positions, pad numbers, GND assignments, BOM and position-file exclusions, artwork, geometry, and copper are identical. All seven warnings cleared. **Zero electrical mismatches were found or corrected.**

Native CLI parity is the closest noninteractive equivalent available here. The command has no forward-annotation proposal mode. The supplemental comparison checks symbol links, fields, and pad/net groups independently of the CLI parity results. A GUI F8 preview remains the way to inspect the actual change list for a particular set of forward-annotation options. For such a review, open an isolated project copy, leave relinking by reference off, and inspect the Changes To Be Applied pane before accepting any update. [KiCad 10 PCB Editor manual](https://docs.kicad.org/10.0/en/pcbnew/pcbnew.html#forward-and-back-annotation)

## Critical circuits

| Circuit | Verified schematic and PCB agreement |
|---|---|
| Main feed and return | J1 A (center) to `/12V_IN`, B (switched) explicit NC, C (sleeve) GND. RSH1.2 upstream, RSH1.1 downstream to J2.1 `/12V_HDD`. J2.2 GND. J2 is Phoenix Contact 5442206, with 12V/GND silkscreen. The fan feed branches at upstream `/12V_IN`. |
| TPS22810 U3 | 1 VIN `/12V_IN`, 2 GND, 3 EN `+5V`, 4 CT NC, 5 QOD NC, 6 VOUT `/12V_SWITCH` to both fan headers. C5 is the VIN bypass to GND. R7 pulls EN to GND. The earlier PCB-001 open remains corrected. U3.2 at about (55.30, 92.70) mm has a 0.50 mm F.Cu GND trace into the connected ground network, and the refill run reports zero opens. |
| INA228 U2 | Pads 1/2 and 7 GND, 3 ALERT NC, 4 SDA, 5 SCL, 6 +3.3 V, 8 downstream VBUS, 9 IN− and 10 IN+. R1/R2 pull up I²C, C1 bypasses VS, and C2 bridges the filtered inputs after R3/R4. |
| Shunt sensing | Separate R3 and R4 sense branches meet the two-terminal shunt pads from their own vias near (64.25, 82.93) and (64.26, 85.79) mm. The 4 mm B.Cu load routes approach those pads separately. Upstream pad 2 goes through R3 to IN+, downstream pad 1 through R4 to IN−. U2.8 taps downstream at the R4 branch. The sense paths share the shunt landings with load current. Landing resistance and measurement accuracy require calibration. |
| XIAO RP2350 U1 | Pads 1/2 TH1/TH2 ADC, 5/6 SDA/SCL, 8/9 fan 1 tach/PWM, 10/11 fan 2 PWM/tach, 12 3.3 V, 13 GND, 14 USB VBUS/EN. Pads 3/4/7 explicit NC. Only pads 1–14 participate. Underside contacts 15–30 are represented by fabrication markings without PCB copper pads. |
| Transistors and thermistors | Both Q1/Q2 use the project-local TO-92 footprint. Pins 1/2/3 are emitter GND, base drive and fan PWM collector. Both NTCs have pad 1 on their own ADC node and pad 2 on GND. Each ADC node has its own 33 kΩ pull-up and 100 nF capacitor. |

| Fan pin | Function | J3 PCB net | J4 PCB net |
|---|---|---|---|
| 1 | Ground | `GND` | `GND` |
| 2 | Switched 12 V | `/12V_SWITCH` | `/12V_SWITCH` |
| 3 | Tach | `/FAN1_TACH` | `/FAN2_TACH` |
| 4 | PWM at open-collector output | `Net-(Q1-C)` | `Net-(Q2-C)` |

## Ignored checks and custom rules

In an isolated copy, all configured `ignore` severities were raised to `warning`. ERC and DRC both returned zero findings with those checks enabled. DRC included schematic parity, all track errors and zone refill. The existing settings were preserved.

| Check | Disposition |
|---|---|
| Missing courtyard | No findings. Keep checking assembly clearances separately. Suppression has no demonstrated need on this revision. |
| Track endpoint not centered on via | No findings with the check enabled. Suppression has no demonstrated need on this revision. |
| Tuning profile track geometries | No findings. The design has no length-matched interface requiring a tuning profile. |
| Footprint filters | No findings, including J2's updated project-local filter. Suppression has no demonstrated need on this revision. |
| Footprint type versus pads | No findings. Suppression has no demonstrated need on this revision. |
| SPICE model issue | No ERC findings when enabled. SPICE simulation is outside this audit. |

- **INA228 rule** selects U2 and applies 0.20 mm clearance and a 0.20–0.25 mm track-width constraint where its condition matches. The clearance matches the project-wide 0.20 mm floor. Removing both custom rules in a separate copy produced no U2 findings. This reference-based selector should not be treated as a blanket width constraint on the full sense nets.
- **Q1/Q2 rule** reduces only the clearance between pads on the same transistor to 0.10 mm. Removing it exposes eight errors, the 1–2 and 2–3 pairs on both layers for both transistors, at **0.12 mm** against the default 0.20 mm. A 1.15 mm pad over a 0.75 mm drill gives a nominal 0.20 mm narrow-side annular ring. The owner-approved 2-layer, 1 oz, green-mask process has a published 0.10 mm mask-bridge minimum. At the reviewed 1:1 openings the nominal web is 0.12 mm. Final CAM acceptance and actual mask openings remain fabrication checks. [JLCPCB capabilities](https://jlcpcb.com/capabilities/Capabilities)

## Reproduction

From the repository root with KiCad 10 and its standard libraries configured:

```bash
out=$(mktemp -d)
p=hardware/fan-controller
kicad-cli version
kicad-cli sch erc --format json --severity-all --exit-code-violations -o "$out/erc.json" "$p.kicad_sch"
kicad-cli sch export netlist --format kicadxml -o "$out/schematic.xml" "$p.kicad_sch"
kicad-cli pcb drc --format json --severity-all --schematic-parity --exit-code-violations -o "$out/drc.json" "$p.kicad_pcb"
kicad-cli pcb drc --format json --severity-all --schematic-parity --refill-zones --exit-code-violations -o "$out/refill.json" "$p.kicad_pcb"
kicad-cli pcb export ipcd356 -o "$out/board.d356" "$p.kicad_pcb"
python3 scripts/check-schematic-pcb-parity.py "$out/schematic.xml" "$out/board.d356" "$p.kicad_pcb" > "$out/comparison.json"
```

The script emits deterministic JSON and exits nonzero on a mismatch. It uses exact net names for the schematic-to-board comparison and accounts for the IPC export's uppercase rendering separately. To repeat the ignored-check probe, copy the hardware project and its libraries, replace each `ignore` with `warning` in the copied project's `board.design_settings.rule_severities` and `erc.rule_severities`, then rerun ERC and DRC there with `--all-track-errors` added to DRC. To isolate the custom-rule effect, repeat DRC on a separate copy with only `fan-controller.kicad_dru` removed.

## Fabrication gates

Keep the Molex-derived footprint and model. The planned MODDIY connector's hole pattern, keyed orientation and mating remain unverified. Check a physical sample against a 1:1 board print or coupon and confirm continuity from the keyed plug to pads 1–4 before ordering. Then review the final Gerber/drill and manufacturer CAM preview, including Q1/Q2 mask webs, J2 polarity and the standoff relief. The 7 A Tensility barrel jack remains an owner-accepted prototype risk pending actual current, voltage-drop and heating measurements. See the [previous review and owner dispositions](https://github.com/full-of-beans/cenmate-fan-controller/pull/6#pullrequestreview-5802921394).
