# Controller schematic verification

Review date: **2026-10-23 (America/New_York)**. Verdict: **do not freeze the controller schematic yet**. The eight-sensor interface is defined well enough to begin the separate strip schematic, subject to the sensor footprint and excitation decisions below. No hardware files were changed by this review.

## Revision, authority, and method

- Repository: `full-of-beans/cenmate-fan-controller`; existing [PR #3](https://github.com/full-of-beans/cenmate-fan-controller/pull/3), `initial-schematic`.
- Reviewed hardware HEAD: `4e11ff811b3e1c956a6e93991b7f690b1e0788ef`.
- Refreshed `main`: `0547721e74e47c7454f8622b46102caedfa93470`.
- Schematic: [`controller.kicad_sch`](../hardware/controller/controller.kicad_sch), Git blob `f9159b28d068a0c50c1f24a80bb9ab4622480034`, SHA-256 `bf67a6716ebc1300f01249a031c463ee02dcd0e819cff6cc365332b2d0739ea6`.
- No `AGENTS.md` exists in the repository or applicable workspace ancestors. README, native project files, all 17 PR commits, the complete changed-file inventory/diff, and GitHub review/comment/thread history were inspected. The three review-history collections were empty at review time. The large library addition includes unused XIAO variants; only the selected symbols/footprints govern this circuit.
- Earlier design specification and BOM are in **open, unmerged [PR #2](https://github.com/full-of-beans/cenmate-fan-controller/pull/2)**, branch `docs/initial-electronics-spec-bom`, HEAD `8b0578ea2d25c28e74fddcfbf3a2d628fc8d7e7f`. They are recovered design intent, not accepted `main` content or proof of electrical correctness. Do not merge that branch merely to recover context. Its reference designators and input-connector plan are stale relative to this schematic.
- Read the single-sheet native S-expression schematic, its embedded placed symbols, project ERC settings, five project-local symbol/footprint library mappings, selected local footprints, and the committed ERC report. Independently reconstructed connectivity with a temporary Python union/find graph over transformed symbol pin coordinates, wires, junctions, and identically named labels/power symbols. Selected the placed body style: RSH1 uses style 1; its unused alternate style must not contribute pins.
- Inspection covered **29 physical components, 89 wire segments, 18 junctions, 28 local labels, and 19 explicit no-connect markers**. There are no buses or hierarchical sheets. All 19 no-connect markers correspond to the unused pins listed below. The reconstructed graph has 49 connected/single-pin groups including unused pins; this is an inspection result, not a KiCad-exported netlist.
- **KiCad CLI is unavailable.** Available OS packages offer KiCad 7, while this project is KiCad 10; an older engine was not used to reinterpret it. No fresh ERC or KiCad netlist export is claimed. The committed ERC evidence and independent graph were checked together.
- Standard footprints were inspected from official **KiCad GitLab `10.0.7`** sources (not the older GitHub mirror). These checks establish compatibility of the named library definitions, not that an arbitrary user's installed library version is identical. Local footprint pad geometry was inspected directly. There are no footprints, tracks, or board outline in the controller PCB; its default thickness is 1.6 mm.
- No live enclosure, purchased parts, PSU label, physical harness, or prototype was available. Manufacturer documents below establish specifications, not verified deployed state or fabrication approval.

## Requirements and recovered decisions

The functional target is a roughly 30 × 100 mm XIAO RP2350 controller: external 12 V at J1; HDD backplane at J2; two independently controlled/monitored P8 Max fans; HDD-only high-side current sensing; two local and six remote NTCs. Strip target is roughly 160 × 8 × 0.6 mm with limited clearance, six bay-aligned sensors, seven wires, insulation and mechanical retention. These dimensions are **requirements, not implemented geometry**.

The implemented power branching follows the intended architecture. Differences and historical decisions:

- **U1 is the TRACO; U2 is the XIAO**, unlike the earlier BOM. J1 is now the barrel input; J2 is the HDD screw terminal. Neither is the old proposed direct-solder switched input.
- CSS2H-3920R-L200F is a **two-terminal** resistor. The requested description “four-terminal shunt” does not match this MPN or its two-pad footprint. Four-wire measurement can be obtained using dedicated Kelvin PCB takeoffs; there are no separate manufacturer-numbered sense terminals. Keeping this MPN does not require changing schematic wiring.
- Historical rocker test (`8b0578e`, PR #2): bridging the two-terminal `Switch` harness with an EX330 on its 10 A range started the backplane fans, with `0.00 A` displayed and no HDDs attached. This supports a low-current control interface, not a high-current power source. Do not revive the rejected switched-12-V-feed assumption. The native controller has no rocker interface; the original switch must remain associated with the backplane until its integration is resolved.
- Prior **10 A continuous path target** is retained as a review criterion pending explicit reconciliation with the selected 7 A input jack. It is not a measurement of actual six-drive consumption.
- Commits `2533ec8` and `8a5ff6a` corrected C3/wiring; `ea6d857`/`e2393ac` synchronized connector pin types; `e3369a3` corrected the used RSH1 pin types; `4e11ff8` restored ERC warning checks. Present C1/C3 are across 3.3 V/GND, and the used connector/shunt pins are passive. These resolved ERC issues are not being reopened.
- Strip uses six individual signals and common GND, no shared strip pull-up. Retention should avoid glue; exact insulation and bracket design remain mechanical work.

## Verification matrix

Each row is one counted finding. **PASS** means the stated scope is evidenced; **PASS WITH LAYOUT REQUIREMENT** preserves an explicit implementation constraint; **OPEN** means evidence/decision is missing; **FAIL** means a confirmed conflict with a stated requirement. OPEN does not mean a proven broken circuit. Priorities: P1 blocks controller freeze; P2 blocks layout/fabrication or commissioning as specified.

| ID | Reviewed item / references | Classification | Evidence and consequence / disposition |
|---|---|---|---|
| A1 | Barrel polarity and switched contact, J1 | PASS | A → `12V_IN`, C → GND, B has explicit NC. Tensility drawing identifies A as center, C sleeve and B switched contact. Center-positive intended; leaving B unused is correct. Actual PSU polarity remains A5. |
| A2 | Full input current rating, J1 | FAIL | Tensility 54-00286 is 48 V, **7 A**, below recovered 10 A continuous HDD-path target even before fans/controller. P1; resolve target or select a higher-rated input system. Component/footprint change if 10 A retained; see findings. |
| A3 | HDD-only current branch, RSH1/J2/J3/J4/U1 | PASS | `12V_IN` supplies RSH1.2, both fan pin 2s and U1.1. `12V_HDD` is RSH1.1, J2.1 and U3.8/R4.1. No wire/net bypass around RSH1. Fan/controller consumption is excluded. |
| A4 | Shared ground and power copper | PASS WITH LAYOUT REQUIREMENT | J1.C, J2.2, both fan pin 1s and all functional IC grounds share GND. Provide full-current return to J1; do not use chassis/USB shield as the load return. Copper, vias and terminations require current/thermal design. |
| A5 | PSU/backplane/switch integration, J1/J2 | OPEN | No measured PSU polarity/rating, startup current or completed physical power interception evidence. P1; verify the real DC12V input and switch operation; sole positive feed must run through J2/RSH1. Harness/system integration; not presently a confirmed schematic miswire. |
| A6 | Reverse polarity, overcurrent, transients, J1/J2/J3/J4/U1 | OPEN | No fuse, reverse-polarity device, TVS or 12 V bulk capacitor exists among the 29 components. D1 is on 5 V and protects none of the 12 V loads. P1; complete protection selection/coordination before freeze; schematic additions likely. |
| B1 | INA supply/address/I²C, U3/R1/R2/C1 | PASS | VS6=3.3 V, GND7=GND; A1/A0 pins 1/2 grounded → 0x40. SDA4/SCL5 reach U2 D4/D5; R1/R2 4.7k pull to 3.3 V; C1=100nF. ALERT3 unused/NC is permitted. |
| B2 | INA polarity/filter/VBUS, U3/R3/R4/C2 | PASS | R3=100Ω from upstream to IN+10; R4=100Ω from downstream to IN−9; C2=100nF across IC-side inputs. VBUS8 is downstream. Correct forward-current sign; RC topology matches TI §8.1.4. |
| B3 | Shunt construction and Kelvin implementation, RSH1 | PASS WITH LAYOUT REQUIREMENT | Bourns two-terminal 0.2mΩ, ±1%, 12 W part; local footprint pads 1/2 match. Use separate sense traces at the manufacturer-recommended terminal sense locations; no load copper shared in the sense route. Four-terminal-component premise is incorrect. |
| B4 | Current range, dissipation and operating voltage | PASS | At 10 A: 2 mV / 20 mW. INA sense full scales correspond to ±204.8 A / ±819.2 A; this does **not** rate connectors for those currents. 12 V common mode and 3.3 V supply are within TI limits. Quantization/calibration limits below. |
| C1 | TRACO pins and basic capacitor requirement, U1 | PASS | Pins 1/2/3 = input/GND/output; 12 V → 5 V, 0.5 A. Manufacturer 2025 document has internal input capacitance, no minimum load and no universal external output-cap minimum; 22µF requirement is for input >28 V, not nominal 12 V. Cable transients/EMC still A6; load/budget C4. |
| C2 | Schottky direction/rating, D1 | PASS | U1.3 → D1 anode2; cathode1 → +5V/U2.14. STPS1L40ZF direction permits external supply and blocks USB feeding back into the TRACO output (apart from leakage). 1 A / 40 V nominal device limits exceed this 0.5 A branch. This does not establish host isolation. |
| C3 | USB/external supply isolation, D1/U2 | FAIL | U2.14 is **VBUS**, directly connected to USB-C VBUS in Seeed v1.0 schematic. D1 is upstream of that node: external power can energize the USB cable/host VBUS. P1; redesign isolation or enforce a documented VBUS-disconnected USB connection; see ordered actions. |
| C4 | Guaranteed 5 V / 3.3 V load and thermal budget, U1/U2 | OPEN | RT9013 on Seeed board is nominally 500 mA total, not 500 mA spare. MCU/LED/flash, peripherals, transients and LDO heat must fit both regulators and USB-only supply. P1; allocate/measure worst case and document margin. Power budget/bench evidence; change components only if insufficient. |
| D1 | Fan pinout/power, J3/J4 | PASS | 1 GND, 2 +12 V, 3 tach, 4 PWM matches ARCTIC. P8 Max ACFAN00286A is 12 V / 0.19 A specified current each (not a measured startup peak). Both supplies are upstream of shunt. |
| D2 | Open-collector drivers, Q1/Q2/R5/R6/R8/R9 | PASS | Q pins 1 E→GND, 2 B→1k series and 100k pulldown, 3 C→fan PWM4. About 2.6 mA base drive at 3.3 V is ample for a standard 5 mA PWM sink. Inversion required: GPIO high drives fan PWM low. |
| D3 | Independent tach/control GPIO routing, R7/R10/U2 | PASS | Tach lines each have 10k to 3.3 V; fan1 PWM/tach=D7/D8 and fan2=D9/D10. No shared control/tach nets; the PWM GPIOs are distinct hardware PWM slices. Exact GPIO table below. |
| D4 | Actual fan electrical/reset/unpowered behavior | OPEN | Open-collector tach and fan-internal PWM pull-up follow Intel reference behavior, but ARCTIC retrieved sheet does not guarantee all electrical limits or unpowered leakage. P2 commissioning gate: verify actual fan pins, full-speed default, tach high voltage, edge timing and MCU-unpowered state; no speculative wiring correction. |
| E1 | All eight sensor channels, U4/TH1/TH2/J5 | PASS | Channels 0/1 are local TH1/TH2; 2–7 go to J5.1–6. Every NTC return is GND/J5.7. No duplicated signal channels; see strip contract. |
| E2 | Analog divider, select/enable, U4/R11/C3/U2 | PASS | VCC16=3.3 V, VEE7/GND8/enable6=GND; common3 → D1 ADC and 10k R11 →3.3 V. S0/1/2 pins11/10/9 → D2/D3/D6. C3=100nF across supply. HC 3.3 V operation supported; do not substitute HCT without review. |
| E3 | Temperature accuracy, excitation and fault behavior | OPEN | 10k/3380 K NTCs confirmed; divider has mux resistance and self-heating. At 25°C selected current ≈165µA exceeds Murata's 100µA reference for 0.1°C self-heating. P2 before temperature-performance signoff: define scan duty, error budget and R–T conversion; see analysis. Not evidence of thermal destruction. |
| E4 | Imported NTC land pattern, TH1/TH2 and future six | OPEN | Local pads 1.00 ×0.97 mm, center spacing 1.47 mm, inner gap 0.47 mm differ from Murata reflow recommendation (gap 0.6–1.2, pad length 0.6–0.7, width 0.6–0.8 mm). P2 before layout/strip footprint reuse: approve a manufacturer-compatible footprint/stencil; footprint change, no net change. |
| F1 | J1 pad/contact mapping | PASS WITH LAYOUT REQUIREMENT | Local A/B/C slots and offsets match Tensility layout; retain all three mechanical terminals, B electrically NC. Respect jack axis/height (14.2 mm property), mating-plug length and panel load. Rating conflict remains A2. |
| F2 | J2 exact purchased screw terminal | OPEN | Assigned Phoenix MKDS-3-2-5.08 footprint has 5.08mm pitch, 1.3mm holes. Matching manufacturer part is MKDS 3/2-5,08 **1711725** (24 A nominal); no MPN is placed on J2. P1 selection gate: commit to this exact part or validate the purchased substitute. Footprint/property selection; generic 5.08mm pitch alone is insufficient. |
| F3 | INA228AIDGSR footprint | PASS WITH LAYOUT REQUIREMENT | Assigned TSSOP-10 3×3mm /0.5mm footprint is compatible with TI DGS VSSOP-10 despite differing package names. KiCad 10 pads 1.45×0.30mm at ±2.15mm; TI example lands 1.45×0.30mm, row-center distance 4.4mm. Check pin-1 orientation/assembly and installed library before release. |
| F4 | XIAO module symbol/footprint | PASS WITH LAYOUT REQUIREMENT | Selected local RP2350 SMD footprint has numbered pads 1–30 matching symbol identifiers; used 1–14 mapping matches Seeed header drawing. Side-pad pitch 2.54mm. Reserve USB-C, underside module parts and solder/mask clearances; prevent paste/trace shorts on unused underside pads. |
| F5 | U1/D1/Q1/Q2 packages | PASS WITH LAYOUT REQUIREMENT | SIP3 U1 pitch 2.54mm and pin order match TRACO; SOD-123F D1 matches ST ZF variant and cathode pad1; TO-92 inline 1/2/3=E/B/C matches 2N3904BU. Confirm exact lead form, polarity marks and assembly orientation. |
| F6 | Fan-header mechanical drawing/mating, J3/J4 | OPEN | Local four-pad footprint is 2.54mm pitch plus locator at (−5.08,+2.16)mm. Molex 470531000 is 4 A/contact, vertical, PCB locator, 1.6mm PCB recommendation. P2 before layout release: compare locator/lock and pin-1 side against authoritative drawing and actual fan plug; dimensions alone do not prove mating orientation. |
| F7 | Mux/JST package compatibility, U4/J5 | PASS WITH LAYOUT REQUIREMENT | MC74HC4051ADR2G is narrow SOIC-16 matching assigned 3.9×9.9mm/1.27mm. J5 footprint is B7B-XH-A 2.50mm, seven pins/15mm span; JST drawing agrees. Use XHP-7 housing and compatible crimps; do not use 2.54mm lookalikes. Retain keyed pin-1 orientation. |
| F8 | Passive procurement and MPN completeness, R1–R11/C1–C3 | OPEN | Native schematic specifies values/0805 footprints but no purchased MPN/tolerance/voltage. Earlier BOM proposes Yageo 1% resistors and Samsung CL21B104KACNFNC 100nF/25V X7R. P2 fabrication gate: approve actual parts and their ratings; filter capacitor may see fault differential, not merely 3.3 V. BOM/properties, no present proof of a bad capacitor. |
| G1 | Committed ERC and configuration | PASS | Report at reviewed HEAD: 0 errors/0 warnings, no exclusions, only SPICE ignored. ERC settings agree; see dedicated section. This is report verification, not an independent rerun. |
| G2 | Mechanical envelope and strip clearance | OPEN | PCB has no outline/placements; strip files remain placeholders. P2 fabrication gate: verify 30×100 and 160×8×0.6 targets, heights, bay spacing, insulation and bracket/cable retention in physical envelope. PCB/mechanical work. |
| G3 | Routed-board DRC and manufactured-board test | NOT APPLICABLE | No routed layout or physical prototype exists in this revision. Required in the later PCB/commissioning phase; no present DRC/manufacturing pass is claimed. |
| G4 | Prior ERC corrections retained | PASS | Used RSH1 style-1 pins and connectors are passive; C1/C3 rail wiring and all NC markers inspected. No outstanding issue is inferred merely from an older ERC report. |

Matrix totals: **14 PASS, 7 PASS WITH LAYOUT REQUIREMENT, 10 OPEN, 2 FAIL, 1 NOT APPLICABLE** (34 rows).

## Detailed findings and disposition

### A2 — Input rating conflict (P1)

The **entire** input current crosses J1, including HDDs plus fans plus converter. At the recovered 10 A HDD continuous criterion, total input is greater than 10 A; a 7 A jack cannot satisfy it. This is a confirmed requirement conflict, **not** proof the actual enclosure draws 10 A or that the original PSU exceeds 7 A. Decide the current requirement from the PSU and all-drive spin-up/steady-state evidence. If retaining 10 A, change the jack and mating cable/plug to a suitably rated system and validate its footprint, derating, temperature rise and service clearance. If reducing the requirement, record an explicit justified new continuous/inrush limit and protection coordination. A thicker trace cannot cure an underrated connector.

### C3 — External power can reach USB VBUS (P1)

Seeed v1.0 sheet 3/5 connects header pin 14 to USB-C VBUS; sheet 4/5 puts the module's D1 **after VBUS** toward `VBAT`, not between USB connector and header. Thus the controller's D1 blocks the reverse path to the TRACO, but does not block forward external power reaching the host cable. Whether current actually flows into a powered host depends on voltages; the unwanted conductive path itself is confirmed. Never treat “there is a diode” as proof of two-source isolation.

Two reviewable directions: a properly designed power mux/OR-ing scheme with access to separately isolated USB and external source paths (may require module/interface modification), or a specified USB data connection that positively disconnects cable VBUS while preserving data/GND and is verified with the RP2350 USB firmware. Do not blindly move the supply to VBAT: this board also has battery charging circuitry. Select the architecture first; an upstream diode reversal or a second series diode at the same header node does not solve host isolation. Ordinary USB plus external power is not approved by this schematic.

### A5/A6 — Physical power and protection decisions (P1)

The circuit gives a valid **schematic** series path; the actual installation must remove any parallel original positive feed or it will bypass current measurement. Backplane current also includes whatever electronics are powered from its DC12V connector; it is not strictly motor-only HDD current. Verify separately whether the backplane internally switches drive rails. The presumed enable rocker is not a substitute power terminal.

No existing protection is evidenced on this controller. Recommended engineering disposition:

1. Specify the center-positive regulated PSU, cable/plug, maximum continuous and spin-up current, and short-circuit behavior.
2. Provide coordinated overcurrent protection at the source/input that protects the weakest conductor, connector and PCB section; select time/current behavior against drive startup. Supply current limiting alone is acceptable only with documented fault-energy coordination.
3. Restore the earlier intended separate fan-branch protection or explicitly justify a shared arrangement that protects its thinner copper/wires. The present fan branch is unfused. The TRACO's internal short-circuit protection does not protect the HDD/fan branches.
4. For an externally accessible, non-keyed barrel interface, provide reverse-polarity protection or document and accept a controlled-polarity installation with an adequate alternative. The current board has none. A low-loss MOSFET arrangement is preferable to casually adding a high-loss high-current diode, but no specific circuit is approved here.
5. Determine input bulk capacitance, hot-plug behavior and whether a TVS/EMI filter is needed from source/cable transient limits. Do not claim these universally mandatory solely because they are absent. TRACO input tolerance is the more restrictive limit than INA common-mode tolerance. Choose actual TVS clamping and capacitor ratings against the protected devices and fuse behavior.

These are design decisions, not authorization to alter the enclosure or schematic. Hardware additions require owner approval and owner KiCad work.

### C4 — Supply budget (P1)

The external branch is 500 mA at nominal 5 V before D1; the module's RT9013 is 500 mA **total** 3.3 V capacity, with thermal restrictions. External peripheral allowance should initially reserve at least 15 mA: base drive ~5.2 mA when both sinks on, I²C pull-ups ~1.4 mA when low, tachs ~0.66 mA, one selected divider ≤0.33 mA, INA up to ~1.1 mA, plus mux dynamic current/margin. This is an allocation, not measured consumption. Add module MCU/flash/LED demand, any onboard charger load and firmware modes; do not count 500 mA again as spare capacity.

Check `I_module + I_peripherals < 500 mA` for both relevant regulators, USB-only source constraints, startup peaks and actual thermal margin. Approximate module LDO dissipation is `(V_LDO_input − 3.3 V) × I_3V3`; D1 loss is `Vf × I`. TRACO set tolerance and diode drop must still leave sufficient LDO headroom. No large external 3.3 V load is currently proposed, but neither peak current nor enclosure temperature has been measured, so a guaranteed budget is OPEN.

### E3/E4 — Sensor accuracy and footprint (P2)

The DC expression is `V_ADC = V_3V3 × (R_NTC + R_ON) / (R11 + R_NTC + R_ON)`, neglecting leakage and cable resistance. ADC and divider use the same 3.3 V rail: ratiometric conversion is possible. Ideal R_ON=0 at 25°C gives 1.65 V (~2048 of 4096 ADC levels). Signals remain between GND and 3.3 V. A rough constant-3380 K model gives ~2.44 V at 0°C and ~0.77 V at 60°C; these are estimates, not the final calibration curve. Open sensor tends toward full scale; shorted sensor approaches the mux's residual resistance divider, not exactly zero. Implement fault bands rather than an exact-zero test.

At 25°C continuously selected: `I ≈ 3.3/(10k+10k)=165µA`, `P≈0.272mW`. Murata specifies 100µA for its 0.1°C self-heating criterion, with typical dissipation ~1mW/°C. The inferred continuous self-heating is ~0.27°C under the reference thermal condition, not a damage conclusion. Equal eight-channel scanning reduces average excitation per NTC; reset or stalled firmware leaves one channel continuously selected because enable is grounded. Define acceptable temperature error, scan timing, mux R_ON/leakage correction, settling/ADC acquisition and R–T table/calibration. Increasing R11 or switching excitation is an option if the error criterion requires it; neither is a confirmed mandatory rewiring fix yet.

Local imported NTC footprint has pads larger and closer together than Murata's reflow recommendation. Body size fits 1608, but package-size matching alone does not validate solder fillets/stress. Review and select an appropriate two-pad land/stencil pattern **before reusing it on the thin strip**. This is a footprint decision, not a sensor net/pin-polarity change. No imported footprint was silently edited.

### F2/F6/F8/G2 — Release evidence still missing (P1/P2)

J2 needs a purchased MPN; if choosing Phoenix 1711725, use the assigned footprint and its manufacturer's wire/torque conditions (24 A is a component rating, not board approval). Otherwise compare body, pin pitch, pin size, holes, current and wire entry with the actual substitute. J2.1 must be marked `+12V_HDD`; J2.2 GND. Screw terminals are not intrinsically protected against reversing the two wires.

For J3/J4, check the actual keyed fan receptacle against the latch/locator and manufacturer pin-number view before rotating placement. Record an actual plug-fit check. Fan behavior tests must include 12 V present with MCU unpowered, USB-only controller power with fans unpowered, reset/default state, both independent channels, PWM high/low extremes, frequency and tach voltage. Intel reference expectations are not an ARCTIC-specific guarantee.

Passive MPNs are not encoded in this native schematic. Confirm tolerance/power and capacitor voltage against fault conditions, temperature and DC bias. Earlier BOM's 25 V capacitors are suitable candidates, not verified purchased parts. Native footprint assignment completeness is not procurement completeness.

## Verified GPIO and interface contract

These names are taken from the actual U2 symbol and manufacturer mapping; D1 here is the **module pin label**, distinct from diode reference D1.

| Function | U2 physical pad / XIAO label | RP2350 GPIO | Other endpoint |
|---|---|---|---|
| NTC ADC | 2 / D1 | 27 (ADC1) | U4.3, R11.2 |
| Mux S0 / S1 / S2 | 3 / D2; 4 / D3; 7 / D6 | 28; 5; 0 | U4.11 / .10 / .9 |
| I²C SDA / SCL | 5 / D4; 6 / D5 | 6; 7 | U3.4 / .5 |
| Fan 1 PWM / tach | 8 / D7; 9 / D8 | 1; 2 | R5.1 / J3.3 |
| Fan 2 PWM / tach | 10 / D9; 11 / D10 | 4; 3 | R8.1 / J4.3 |
| 3.3 V / GND / VBUS | 12 / 3V3; 13 / GND; 14 / VBUS | — | Low-voltage rail / common return / +5V |

Unused/NC: U2 pads 1,15–30, J1.B and U3.3. The unused extra module GND/3V3 pads are internally connected on the module; NC markers mean no carrier trace is intended, not internal isolation. Do not put unintended carrier copper/paste under module circuitry.

### Future remote strip specification

| Sensor / selector S2 S1 S0 | U4 channel / pin | Controller J5 pin | Strip connection |
|---|---|---|---|
| Onboard TH1 / 000 | 0 / 13 | — | Not on strip |
| Onboard TH2 / 001 | 1 / 14 | — | Not on strip |
| Remote 1 / 010 | 2 / 15 | 1 | Individual NTC end |
| Remote 2 / 011 | 3 / 12 | 2 | Individual NTC end |
| Remote 3 / 100 | 4 / 1 | 3 | Individual NTC end |
| Remote 4 / 101 | 5 / 5 | 4 | Individual NTC end |
| Remote 5 / 110 | 6 / 2 | 5 | Individual NTC end |
| Remote 6 / 111 | 7 / 4 | 6 | Individual NTC end |
| Common return | GND | 7 | Other ends of all six NTCs |

- Six Murata NCU18XH103F60RB 10kΩ ±1% NTCs, B25/50=3380 K ±1%; no polarity. Use actual manufacturer R–T data, not a generic beta assumed accurate over the entire range.
- Seven conductors only: six signals and common return, **no +12 V, +5 V or separate 3.3 V feed**. One pull-up R11 is on the controller's selected common node. Do not place six additional strip pull-ups.
- Controller header: JST B7B-XH-A(LF)(SN), 2.50mm pitch. Cable housing XHP-7 with wire-size-compatible XH terminals. Specify pin numbers from the connector manufacturer's mating view; verify cable continuity to avoid mirroring the order.
- Physical left-to-right drive bay numbering and actual sensor center positions remain unverified. Map Remote 1–6 to drive bays explicitly after measuring; do not infer enclosure bay orientation from this electrical table.
- Strip target 160×8×0.6mm, component-height envelope approximately ≤2mm including board, solder and insulation. A 0.95mm maximum body plus 0.6mm board leaves little clearance. Keep exposed copper from the SATA backplane using rated dielectric insulation; solder mask alone is not a mechanical isolation guarantee. Provide strain relief, maintain removable retention, and avoid flexing the narrow board at ceramic NTCs.
- Schematic capture can start using this contract. **Do not freeze its footprint/layout until E3/E4 and actual mechanical spacing/retention are resolved.** Controller USB/current-path fixes do not change this seven-wire contract unless a later approved design decision explicitly does so.

## Measurement and PCB requirements

The 0.2mΩ shunt is deliberately low loss. At 12 V, ideal sensitive-range current quantum is ~0.391mA (~4.69mW); normal range is ~1.563mA (~18.75mW). These are quantization, not accuracy. Shunt tolerance, input-filter loading, offset, copper pickup and calibration dominate useful accuracy. Symmetric R3/R4 and C2 form ~7.96kHz differential pole (200Ω×100nF); account for input impedance loading and calibrate. Firmware must program shunt calibration and ADC range/averaging; raw full-scale math is not a safe current-path rating.

Layout obligations for all rows marked PASS WITH LAYOUT REQUIREMENT:

1. Engineer input/HDD supply **and return** copper, pads, vias and solder joints for the resolved continuous/inrush/fault current; verify temperature rise and voltage drop. No fixed trace width is approved without copper thickness, stack-up and thermal environment. Avoid narrow thermal-relief spokes on power joints.
2. Take two independent Kelvin routes from RSH1 manufacturer-recommended terminal sense locations to R3/R4; keep them paired, short and clear of converter switching nodes and fan-current loops. Put R3/R4/C2 near U3; VBUS tracks the load side. No high-current copper or parallel enclosure feed may bypass the resistor.
3. Place C1 close to U3 VS/GND and C3 close to U4 VCC/GND; quiet ADC ground/reference routing must not share substantial load-current voltage drop. Place any approved input bulk/protection at the input with a short return loop.
4. Preserve pin-1/polarity marks at U3/U4, D1 cathode, Q emitter, J2 positive and fan/JST headers. Keep JST cable away from 12 V wiring and noisy PWM/converter nodes; implement strain relief and reasonable ESD handling for accessible sensor leads.
5. Reserve XIAO underside/component and USB-C access/overhang clearance, jack mating axis, screw access and cable bend space. Verify actual footprints in the user's KiCad installation and the 3D mechanical assembly. Do not mirror top-view versus bottom-view manufacturer drawings.
6. Check mask/paste, solder fillets and assembly methods, particularly NTCs and module underside pads. RSH1 footprint's central copper keepout must be respected. The imported shunt pads (~2.718×5.512mm at ±4.0005mm) are broadly consistent with the two-terminal body/landing geometry; dedicated sense takeoffs remain essential.
7. Run routed-board DRC with appropriate constraints and review the intentionally ignored PCB DRC rules separately. ERC's sole SPICE ignore does **not** describe PCB DRC: project currently also ignores some footprint/courtyard/other board checks. No DRC result is inferred from ERC.

## ERC evidence and reproducibility

[`ERC.rpt`](../hardware/controller/ERC.rpt) is timestamped `2026-10-23T11:55:40` (report contains no timezone). It includes errors, warnings and exclusions, reports **0 / 0 / 0 messages**, and names only **SPICE model issue** as ignored. `controller.kicad_pro` has empty `erc_exclusions`; its only ERC severity `ignore` is `simulation_model_issue`. Four-way junction, similar labels/power, single-global-label and library/pin checks are enabled. This is consistent with the user's latest clean report and commit `4e11ff8`.

Embedded module pins are passive, so ERC cannot by itself diagnose power direction, USB backfeed, GPIO function, connector current rating or physical fit. The unused alternate RSH1 body retains unspecified pin types; it is not the placed style and does not invalidate the used style's correction.

For an independent native rerun later, use the same installed KiCad 10/library environment, export a netlist to a temporary directory, run ERC including warnings/exclusions, and compare these connections without saving incidental hardware changes:

```sh
kicad-cli sch export netlist --format kicadxml -o /tmp/controller.xml hardware/controller/controller.kicad_sch
kicad-cli sch erc --severity-all -o /tmp/controller-erc.rpt hardware/controller/controller.kicad_sch
```

A later hardware correction invalidates the relevant rows, not every resolved design decision: update the reviewed SHA, regenerate ERC/netlist, recheck affected nets and preserve unaffected evidence. Do not use this report as a perpetual pass after hardware changes.

## Manufacturer and library references

Accessed 2026-10-23. Manufacturer-authored PDFs hosted by distributors are identified as mirrors. A mirror is not represented as a newly downloaded current manufacturer release.

| Key | Source / scope |
|---|---|
| Tensility | [54-00286 product](https://www.tensility.com/products/54-00286), [drawing rev A, 2021-05-13](https://tensility.s3.us-west-2.amazonaws.com/imports/product_spec_sheets/54-00286.pdf): contacts, PCB slots, 48V/7A and temperature derating. |
| Seeed | [XIAO RP2350 v1.0, 5-sheet schematic](https://files.seeedstudio.com/wiki/XIAO-RP2350/res/Seeed-Studio-XIAO-RP2350-v1.0.pdf), sheets 3/4/5: header, VBUS/battery/LDO and GPIO paths. [Manufacturer hardware repository](https://github.com/Seeed-Studio/OSHW-XIAO-Series) for module assets. |
| Richtek | [RT9013](https://www.richtek.com/assets/product_file/RT9013/DS9013-10.pdf): 500mA LDO; thermal/load restrictions remain system-specific. |
| TI | [INA228 SLYS021A](https://www.ti.com/lit/ds/symlink/ina228.pdf), pin table, electrical characteristics, §8.1.4, §9/10, DGS land pattern: supply/address/filter/ranges/package. |
| Bourns | [CSS2H-3920, rev 01/26](https://www.bourns.com/docs/product-datasheets/css2h-3920.pdf), pp1–2: L200F resistance/power, two-terminal geometry and Kelvin PCB sensing. |
| TRACO | [Current manufacturer landing document](https://www.tracopower.com/tsr0-5-datasheet) (full fetch blocked by HTTP403; indexed July 2, 2026 note confirms >28V capacitor condition). [Manufacturer PDF mirrored by Mouser, March 24, 2025](https://www.mouser.com/datasheet/2/687/tsr0_5_datasheet-3049847.pdf), pp1–3: 6.5–32V, 5V/500mA, internal input cap, max 220µF capacitive load, pinout/SIP dimensions. No assertion of EMC compliance without its external filter. |
| ST | [STPS1L40 DS1244 rev9](https://www.st.com/resource/en/datasheet/stps1l40.pdf): ZF SOD123Flat, 1A/40V, forward drop/thermal data and cathode orientation. |
| Raspberry Pi | [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf) and [SDK hardware reference](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html): ADC-capable GPIO27, PWM slice mapping (GPIO1 slice0/B; GPIO4 slice2/A), GPIO6/7 I²C1. |
| onsemi mux | [MC74HC4051A rev17](https://www.onsemi.com/download/data-sheet/pdf/mc74hc4051a-d.pdf), pin/function table, supply ranges and D-suffix SOIC package. Native generic U4 links TI instead; onsemi substitute evaluated explicitly here. |
| onsemi transistor | [2N3904](https://www.onsemi.com/download/data-sheet/pdf/2n3904-d.pdf): E/B/C and BU TO-92 variant. Native symbol's older 2N3903/4 source also inspected. |
| ARCTIC | [P8 Max specification](https://www.arctic.de/media/3f/04/7e/1689685872/Spec_Sheet_P8_Max_EN.pdf): ACFAN00286A, 12V/0.19A, numbered fan pinout. [User manual](https://support.arctic.de/en/p8max). |
| Intel | [Celeron 400 thermal/mechanical guide](https://www.intel.com.tr/content/dam/www/public/us/en/documents/design-guides/celeron-400-guide.pdf), Tables 6/11: reference four-wire fan electrical behavior. This does not replace ARCTIC-specific verification. |
| Molex | [470531000](https://www.molex.com/en-us/products/part-detail/470531000): 2.54mm, 4A/contact, PCB locator, recommended thickness. Actual mating orientation/locator drawing comparison remains F6. |
| Murata | [Product page](https://pim.murata.com/en-us/pim/details/?partNum=NCU18XH103F60RB) (full fetch blocked); [manufacturer specification mirrored by Mouser](https://www.mouser.com/pdfDocs/ncu18.pdf), pp1/3/6: 10k/B3380, excitation/self-heating, 1608 body and reflow land recommendation. [2024-08-19 manufacturer product sheet mirror](https://resources.ampheo.com/static/datasheets/murata-manufacturing/ncu18xh103f60rb.pdf) corroborates resistance, B-value, 0.1mA, 5V and package; use approved current ordering specifications for procurement. |
| Phoenix | [MKDS 3/2-5,08 1711725](https://www.phoenixcontact.com/en-in/products/printed-circuit-board-terminal-mkds-3-2-508-1711725): matching candidate for J2, ratings/wire/torque conditions. Selection is still OPEN. |
| JST | [Manufacturer XH family](https://jst.de/product-family/show/184/xh); [manufacturer XH PDF mirror](https://uploadcdn.oneyac.com/attachments/files/brand_pdf/jst/6B/29/B4B-XH-A-RLFSN.pdf), pp1–3 includes B7B-XH-A, housing/crimp and 2.50mm/15mm dimensions. Direct JST PDF fetch was unavailable; inspected full manufacturer-authored mirror. |
| KiCad | [Official footprints at 10.0.7](https://gitlab.com/kicad/libraries/kicad-footprints/-/tree/10.0.7): selected Package_SO, Converter_DCDC, Diode_SMD, Package_TO_SOT_THT, TerminalBlock_Phoenix, Connector_JST, Resistor_SMD and Capacitor_SMD definitions. All five project-local library table paths resolve to tracked assets; selected custom definitions inspected in this repository. |

## Ordered owner actions and readiness

1. **Resolve C3 first:** choose a safe simultaneous-USB/external-power architecture, document it, then make the agreed changes in KiCad. There is no safe universal “move this one wire” instruction for the present VBUS-connected module. Do not reverse D1 as a supposed solution.
2. **Resolve A2/A5:** record PSU/current/startup requirements and actual backplane/rocker integration. If keeping 10A, select a suitable jack/plug/cable; in KiCad replace J1's symbol/footprint only after its datasheet pin mapping is approved. Connect its verified positive terminal to `12V_IN`, return to GND, and handle switched terminals per that part's drawing.
3. **Resolve A6:** select input/branch protection and transient components against that agreed envelope. In KiCad add them in the approved power-path positions while keeping fan/controller branches upstream of RSH1 and HDD positive downstream. No generic fuse/TVS value is approved here.
4. **Resolve F2/C4:** select J2's exact MPN; use Symbol Properties to record it and verify footprint. Complete regulator current/thermal budget. Change wiring only if the resulting engineering decision requires it.
5. **Resolve E4/E3:** choose a compliant NTC land/stencil pattern, assign it to TH1/TH2 and future strip sensors, and approve scan/excitation/error assumptions. Keep the seven-wire mapping above. This is not an instruction to alter divider wiring before choosing the accuracy target.
6. After authorized owner hardware edits: inspect changed nets, rerun KiCad 10 ERC with present checks enabled, save report, commit, and update this document's affected rows/reviewed revision. Complete F6/F8/G2 before fabrication; satisfy the layout and bench requirements in their respective phases.

**Controller freeze:** blocked by A2/C3 and P1 OPEN decisions A5/A6/C4/F2. **Strip schematic:** may begin under the documented seven-wire contract; sensor land pattern/excitation and mechanical bay mapping remain its release gates. **PCB fabrication:** not approved; no layout exists. No schematic, project, library, ERC or PCB files were modified by this documentation review, and neither PR is merged.

## Deferred design plan — low-profile FFC thermistor-strip interface (2026-10-24)

**Status: selected direction for later KiCad implementation, not a verified mating set, released BOM, or fabrication approval.** Owner is away from the workstation. This is documentation only; the existing controller J5 JST XH footprint/netlist and the thermistor-strip KiCad files have **not** been changed. The seven-net electrical contract in the preceding section remains authoritative.

### Intended physical and assembly architecture

- Use a **horizontal-entry, flip-lock FFC/FPC connector on the drive-facing side of the remote thermistor strip**, preferably the **Hirose FH19C-7S-0.5SH(10)** as the first candidate. Seven positions, 0.5 mm pitch; nominal connector height about 0.9 mm above the mounting PCB. Check Hirose's exact ordering suffix, contact orientation, recommended land pattern, and dimensional drawing before footprint assignment.
- Prefer a **matching FFC termination on the controller PCB**, using a factory-finished seven-conductor FFC. The goal is **no hand stripping, individual wire crimping, or wire-to-strip soldering**. The SMT connectors themselves must still be soldered; their retention fittings offer mechanical solder anchors, **not locating pegs or a substitute for cable strain relief**.
- Thermistor sensing portion remains nominally **160 × 8 × 0.6 mm**, with six bay-aligned NTCs. The **base may be widened** to accommodate connector pads, routing, and a removable 3D-printed bracket/strain relief; final overall outline and sensor-center positions require measurements. Keep the thin board supported during assembly and avoid flexing it at ceramic sensors.
- In front view from the **drive side** of the SATA backplane, the sensor strip is **vertical**. Place the connector toward its **lower end**; choose its orientation to let the FFC leave toward the **left**, then route around the **bottom or side perimeter** of the backplane, potentially via openings in the plastic cage, into the rear controller plenum. This **perimeter routing is preferred to feeding the cable through an airflow aperture**, but is not yet physically validated.
- HDDs can be removed to access the connector latch during installation. Thus **latch operating clearance is not required with HDDs installed**; the **closed connector, seated cable, cable exit and bend still must clear the installed HDD/tray/cage**. Verify no sharp-edge abrasion, pinching, contact with moving trays, or interference when the rear shell is reassembled.
- The previous approximately **2 mm assembled-height target** remains a **target**, not a measured local clearance. Illustrative stack: 0.6 mm PCB + about 0.9 mm connector + insulating film = about 1.6 mm **before tolerances, adhesive if any, mechanical stand-off, or cable bend envelope**. A larger base alone does not solve height; locate the connector where the installed-drive clearance is sufficient.
- Use an electrically rated dielectric barrier between the strip and SATA backplane; solder mask alone is not the barrier. Bracket should provide strain relief **independent of the FFC latch**, with no adhesive required if practical.

### Procurement candidates and gates

| Item | Candidate | Status / required verification |
|---|---|---|
| Strip-side FFC connector | Hirose **FH19C-7S-0.5SH(10)**; DigiKey **H125825CT-ND** (candidate listing) | **Preferred candidate, not frozen.** Confirm manufacturer's current drawing, exact cable thickness (nominally 0.20 mm), contact side, pad footprint, hold-down fittings, hand-solder access, and available stock. |
| Controller-side FFC connector | Same Hirose part **if** the chosen FFC contact-side orientation and installed board orientation mate correctly | Not yet selected. Do **not** silently substitute a 0.30 mm-cable connector into a 0.20 mm system. |
| FFC cable | Factory-terminated, 7-conductor, 0.5 mm pitch; length to be measured around enclosure perimeter | **No cable MPN approved.** Verify 0.20 mm mating thickness, stiffener geometry, same-side/opposite-side exposed contacts (Type A/B), cable width, permissible static bend radius, temperature rating, and connector-specific insertion dimensions. A cable matching pitch and pin count alone is not sufficient. |
| Controller J5 | Present JST **B7B-XH-A(LF)(SN)**, seven pins | **Existing implementation remains in KiCad.** Replacing it with FFC requires a future owner-approved schematic/footprint change and pin-order verification. |

### Checklist for return to KiCad

1. Measure installed-drive clearance at proposed connector base and cable exit; check cable path around bottom/side with shell assembled, and measure **required cable length** with a bend/service margin.
2. Obtain manufacturer connector and cable drawings; select a **verified mating pair** and contact orientation before ordering. Confirm the FFC's conductor order at both ends, especially when mating identical bottom-contact connectors on differently oriented PCBs.
3. Decide final strip base width, connector rotation, PCB copper side, bracket/strain relief and insulation; review the manufacturer-recommended **NTC land pattern** (E4) independently.
4. On owner's workstation, update the **strip schematic/PCB** and **controller J5 interface** deliberately; preserve six individual NTC channels and common GND. Verify the new physical pin-number mapping with continuity, not assumptions from a top-view drawing.
5. Run KiCad 10 ERC/DRC and physical clearance checks, then prototype fit and test before fabrication approval.

**Do not interpret this plan as resolving the controller's existing A2/C3 electrical FAIL findings, E3/E4 review gates, or any PCB-release blocker.**
