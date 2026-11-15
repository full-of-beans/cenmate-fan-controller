# CENMATE six-bay fan controller engineering and fabrication audit

| Baseline | Value |
| --- | --- |
| Repository and branch | [full-of-beans/cenmate-fan-controller](https://github.com/full-of-beans/cenmate-fan-controller), `initial-schematic` |
| **AUDITED_COMMIT** | [`3294cb89a1b5f34098adf01730dca6feb2a4a3dd`](https://github.com/full-of-beans/cenmate-fan-controller/commit/3294cb89a1b5f34098adf01730dca6feb2a4a3dd) |
| Audit date | 2026-11-15, America/New_York |
| Tools | KiCad 10.0.8 Linux x86-64 Lite AppImage, Git 2.52.0, Python 3.12.14, Gerbonara 1.6.3, Shapely 2.1.2, FreeCAD 1.0.0 Python/Open Cascade 7.8.1 STEP import with system Python 3.13.5 |
| Process assumption | JLCPCB rigid FR-4, two layers, 1.6 mm nominal thickness, 1 oz copper, green solder mask. Through-hole parts hand-soldered. Finish and SMD fit list require order review. |

## Readiness verdict

**Overall HOLD for an assembled, powered six-drive prototype. Bare PCB fabrication is a CONDITIONAL PASS for the specified stack and order-preview gates.**

| Stage | Verdict | Release criterion |
| --- | --- | --- |
| Schematic electrical correctness | CONDITIONAL PASS | Nominal mapping and topology agree with the selected devices. Qualify the rocker interface, system protection and switching transients in AUD26-001, 002 and 008. |
| PCB connectivity and layout | CONDITIONAL PASS | Native and independent connectivity pass. Shunt lands/takeoffs differ from manufacturer guidance. Qualify current-sense accuracy and bypass behavior in AUD26-005 and 009. |
| Bare PCB fabrication | CONDITIONAL PASS | Approve fresh CAM, plated holes/slots, relief and Q1/Q2 mask treatment for green 1 oz. Check the real connectors and backplate pattern before ordering parts that must fit without rework. AUD26-004, 006, 012 and 014. |
| PCB assembly | HOLD | Verify intended headers, mechanical assembly and actual fit list/stencil. AUD26-004, 006 and 011. |
| Safe initial power-on | HOLD for enclosure hookup | Isolated current-limited bench work can proceed after bare-board continuity and polarity checks. Qualify AUD26-001, 002, 004 and 008 before connecting the enclosure and drives. |
| Normal operation with six HDDs and both fans | HOLD | Measure startup, continuous current and temperatures. Validate cooling failsafe, sensors, telemetry and actual fan behavior. AUD26-001, 002, 003, 005, 007–010. |

**Findings:** 0 Blocker, 7 Major, 6 Minor, 1 Informational. Status totals are 6 Open, 5 Requires Physical Validation and 3 Accepted Risk. Severity reflects consequence. Accepted Risk records an owner disposition. Requires Physical Validation identifies a demonstrated exposure or unqualified interface whose effect depends on hardware. Individual stage gates explain where a finding holds release.

## Baseline, scope and checks executed

Fetched `origin initial-schematic` into a fresh clone, then used a detached worktree at the resulting SHA. This matched the previously observed HEAD. Inspected schematic, PCB, project rules, custom symbols and footprints, relevant datasheets and STEP models, circuit notes, scripts and historical reports. The active design uses Q3/R7/J5 for rocker-controlled fan power, a USB-powered XIAO, an INA228 with a 1 mΩ two-terminal shunt and two 10 kΩ thermistor pull-ups. Only placeholders are tracked under `firmware/` and `mechanical/`. Coordinates below use KiCad PCB XY in mm except where labeled STEP or CAM. Findings' file links refer to the source tree at this baseline.

| Check | Observed result |
| --- | --- |
| Schematic ERC | 0 errors, 0 warnings, 0 exclusions. Project ignores SPICE model issues. Initial environment-only library warnings cleared after copying the bundled global library tables into the isolated config. |
| PCB DRC, as stored and refilled | Both 0 violations, 0 unconnected items, 0 parity issues, exit 0. Ran all severities and all track errors. Project ignores missing courtyard, via endpoint centering, tuning-profile geometries, footprint filters and component-type checks. Refill was in memory on the baseline. |
| Independent XML / PCB / IPC-D-356 parity | 29 components, 88 pin/pad assignments, 26 net groups, plus H1–H4 grounded board-only mounts excluded from BOM/placement. Endpoint groups, footprints, symbol links and population/metadata fields agree. Extended source/XML comparison found no Datasheet or Description field differences. Explicit no-connects are J1.B, U1.3/.4/.7 and U2.3. `+5V` is a separate one-pad net at U1.14 with two same-net stubs totaling ~0.033 mm at its land. |
| Local CAM | Seven X2 Gerbers, two Excellon files, SVG drill maps and a job manifest. Parsed actual geometry and confirmed all 26 net groups are individually connected, with no different-net copper overlaps. Repeated on a saved refill in a separate temporary copy. All nine Gerber/drill streams match after date headers are stripped. Gerbonara warned about G90 placement after the Excellon header and parsed both files in absolute coordinates. Centers, tools and lengths were checked against source. |
| BOM / placement / paste | 16 native BOM groups totaling 29 parts, with 29 MPNs and 28 explicit DigiKey numbers. SMD position export contains 21 top and 1 bottom placements. Paste has 65 front and 2 back apertures. References, quantity and sides agree with the board. |
| STEP | Fresh bare and populated STEP exported. FreeCAD read a valid single core in both, 30 × 80 × 1.51 mm and 3542.625 mm³. The committed centered core is 3544.952 mm³ and its assembly contains superseded hardware. Checked placement envelopes, selected core intersections and modeled JST lead sections. See AUD26-006 for limits. |

### Electrical cross-checks

| Circuit | Independent result |
| --- | --- |
| HDD distribution | J1.A barrel center positive → `/12V_IN` → RSH1.2 → RSH1.1 → `/12V_HDD` → J2.1. J1.C sleeve and J2.2 are ground. J1.B is the isolated switched sleeve contact. Fan power branches before RSH1. U2.VBUS sees downstream voltage. The reading includes any accessory loads drawn from the enclosure, including the XIAO when its USB source is the backplane. |
| Main power copper | 4.00 mm B.Cu positive routes extend 6.105 mm upstream and 6.351 mm downstream. No series via in the HDD path. A 0.014 mm alignment stub lies within the downstream land. Transverse shunt land width is 3.42 mm. For 35 µm copper and 1.724 × 10⁻⁸ Ω·m at 20 °C, these traces are ~1.53 mΩ. With the nominal 1 mΩ shunt, the sum is ~17.7 mV and 124 mW at 7 A, or ~22.8 mV and 205 mW at 9 A. The estimate excludes the return, pad transitions, solder, contacts and harness. One B.Cu GND fill spans ~2009.25 mm², with broad J1.C and J2.2 entry and fan ground thermals. At Y = 80.15 and 87.90, the major side corridors are ~13.20/4.90 and 10.78/10.99 mm. The fan branch routes 0.5–1.0 mm with via transitions and a shared 1 mm output. |
| Q3 and rocker | DMP3056L-7 pad 1 gate, 2 source to input, 3 drain to switched output. R7 is 100 kΩ gate-to-source. J5.1 gate, J5.2 GND. Gate near 0 V at 12.2 V gives VGS ~−12.2 V and adds ~122 µA to the rocker steady load. An open gate lead pulls VGS to zero via R7. The intrinsic diode can conduct from switched output back toward input if the switched side is separately energized. The circuit provides neither controlled ramp nor specified overcurrent limiting. |
| Current monitor | U2 pins 1/2/7 GND (0x40), 3 ALERT NC, 4 SDA, 5 SCL, 6 3.3 V, 8 downstream VBUS, 9 filtered IN−, 10 filtered IN+. R1/R2 4.7 kΩ, C1 100 nF VS bypass, R3/R4 10 Ω and C2 100 nF differential after them. Ideal RC is 2 µs, ~79.6 kHz. TI recommends low series resistance and a 0.1–1 µF differential capacitor. TI §9 allows 0–85 V on input terminals with VS absent and GND connected. The DGS 3 × 3 mm, 0.50 mm-pitch class agrees with the placed lands, which are 1.45 × 0.30 mm with 4.30 mm row spacing versus TI's 4.40 mm example. |
| XIAO power | USB-C receptacle powers U1. Edge pad 12 is 3.3 V output and pad 13 GND. U1.14 has no carrier load or external VBUS source. There is no converter/diode-OR feed. Simultaneous nominal INA228 load, PWM base currents, all four logic pull-ups sinking and both NTCs give ~9 mA external 3.3 V loading, plus the module and transients. USB source and cable headroom still need verification. |
| Fan interfaces | Each header is 1 GND, 2 switched 12 V, 3 tach, 4 PWM. Q1/Q2 1 E to GND, 2 B through 1 kΩ with 100 kΩ pull-down, 3 C to PWM. GPIO high pulls fan PWM low. At 3.3 V and VBE ~0.7 V the base sees ~2.6 mA. Each 10 kΩ tach pull-up sources up to 0.33 mA. P8 Max ACFAN00286A specifies 0.19 A running and stops below 5% fan PWM. Actual fan input pull-up, tach type/PPR, waveforms, restart and leakage need a bench check. |
| Thermistors | Each NTC is below the ADC node to GND, with 10 kΩ above and 100 nF parallel to the NTC. At 25 °C nominal output is 1.65 V with 5 kΩ Thévenin resistance and 0.50 ms RC. Voltage falls as temperature rises. |

| Function | U1 module pad/label | RP2350 |
| --- | --- | --- |
| TH1, TH2 | 1 D0, 2 D1 | GPIO26 ADC0, GPIO27 ADC1 |
| I²C | 5 D4 SDA, 6 D5 SCL | GPIO6/GPIO7 I²C1 |
| Fan 1 | 9 D8 PWM, 8 D7 tach | GPIO2 PWM slice 1 A, GPIO1 tach |
| Fan 2 | 10 D9 PWM, 11 D10 tach | GPIO4 PWM slice 2 A, GPIO3 tach |
| No-connects | 3 D2, 4 D3, 7 D6 | GPIO28, GPIO5, GPIO0 |

Electrical source references are the [Tensility drawing](https://tensility.s3.us-west-2.amazonaws.com/imports/product_spec_sheets/54-00286.pdf), [Diodes DS37386](https://www.diodes.com/datasheet/download/DMP3056L.pdf), [TI INA228 SLYS021A](https://www.ti.com/lit/ds/symlink/ina228.pdf), [Seeed module pin map](https://wiki.seeedstudio.com/xiao_rp2350_arduino/#pin-map), [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf), [onsemi 2N3904](https://www.onsemi.com/download/data-sheet/pdf/2n3904-d.pdf) and [ARCTIC P8 Max specification](https://www.arctic.de/media/3f/04/7e/1689685872/Spec_Sheet_P8_Max_EN.pdf).

## Detailed findings

### AUD26-001 — Rocker harness and loss of fan power

**Major · Requires Physical Validation.** J5, Q3, R7, `Net-(Q3-G)`, `/12V_IN`, `/12V_SWITCH` and enclosure wiring. J5.1 gate at (56.4901, 97.4085), J5.2 GND at (53.9901, 97.4085). The circuit notes report about 0 V ON and 12.2 V OFF. The original switch topology, conductor identification, switching impedance and loaded tee behavior remain open. A reversed harness can hard-ground the backplane enable conductor. An open J5 branch commands fans off through R7 while the HDD feed stays continuous. Losing USB alone leaves Q3 controlled by the rocker, and an unpowered XIAO releases both PWM lines. Actual fan open-PWM behavior determines cooling.

**Action and acceptance:** Record a keyed, continuity-verified diagram including the original switch/backplane and carrier. Demonstrate R7 loading preserves rocker operation, scope VGS and switched output with a dummy load in all power states, and verify both fans run at the intended safe speed with rocker ON and USB absent. Provide a way to prevent or detect a disconnected J5 while the drives are enabled. Confirm off-state GPIO voltage/current is within RP2350 limits and USB VBUS sees no unintended drive. Holds enclosure hookup and normal use. Bare fabrication is conditional only on mechanical fit. [J5 board](../../hardware/fan-controller.kicad_pcb#L134) · [Q3 schematic](../../hardware/fan-controller.kicad_sch#L5878) · [JST drawing](https://www.jst.com/wp-content/uploads/2025/06/eXH.pdf) · [RP2350 limits](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)

### AUD26-002 — Input fault protection and alternate ground return

**Major · Requires Physical Validation.** J1/A/C, J2/1/2, J5.2, U1 USB ground, Q3, RSH1, H1–H4 and the enclosure harness. J1.A is (66.50, 80.1475), J1.C (63.70, 75.3975); J2.1/.2 are (71.64/66.56, 95.10). The direct, unswitched HDD path lacks a carrier fuse, reverse-polarity protection and surge clamp. Q3 has no circuit-level fan current limit. The J2 return, J5 ground, USB ground and plated mounts share GND. Depending on enclosure bonding and harness topology, an open or resistive J2 ground can divert drive current through a USB cable, rocker ground or mounting hardware. Fault-current magnitude and the adapter's response are unknown. The 12 V/9 A label gives 108 W rated output and no information about protection timing.

**Action and acceptance:** Verify polarity, strain relief, torque/wire selection and the intended high-current return. Map parallel ground paths with the installed assembly. Using current-limited dummy loads, demonstrate that the installed power source or added protection clears faults within the smallest applicable jack, connector, copper and harness ratings. Simulate a missing/high-resistance return at low current first. Establish that auxiliary cables and chassis cannot carry unintended HDD current beyond their ratings, or change the integration. Verify J2 polarity at the mating load plug. Holds enclosure hookup and normal use, with bare fabrication and unpowered assembly possible. [J1 PCB](../../hardware/fan-controller.kicad_pcb#L7654) · [Ground fill](../../hardware/fan-controller.kicad_pcb#L10468) · [Phoenix 5442206 ratings and conductors](https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-bc-508x10-2-gn-5442206)

### AUD26-003 — Accepted 7 A jack with unqualified six-drive load

**Major · Accepted Risk.** J1 Tensility 54-00286, matching 5.5 × 2.1 mm plug, `/12V_IN` and GND. J1 carries all drive and fan current. The reported 9 A brick does not constrain operation to 7 A. Two P8 Max running currents of 0.19 A leave 6.62 A for the entire downstream branch at 7 A total, before derating. As an illustrative estimate only, six drives at 1.5 A each on 12 V plus both fans would demand 9.38 A before other backplane loads.

**Action and acceptance:** Keep the approved prototype jack subject to measured load. Capture total J1 current upstream of the split, minimum J2 voltage, mated contact drop and stabilized plug/jack temperatures at expected hot ambient and repeated cold six-drive starts. Verify the jack and actual plug's derating/current envelope and that drives see acceptable voltage without resets. Obtain qualified transient limits for peaks above the continuous rating or change/enforce the power envelope. Holds unconditional six-drive use. Bare fabrication and unpowered assembly can proceed. [J1 board](../../hardware/fan-controller.kicad_pcb#L7654) · [Tensility drawing/derating](https://tensility.s3.us-west-2.amazonaws.com/imports/product_spec_sheets/54-00286.pdf) · [Owner disposition](https://github.com/full-of-beans/cenmate-fan-controller/pull/6#issuecomment-6693782991)

### AUD26-004 — Intended fan headers differ from the CAD parts

**Major · Requires Physical Validation.** J3/J4, custom Molex footprint/model, `/12V_SWITCH`, GND, tach and PWM. Pad 1 centers are (53.6919, 115.5594) and (65.9319, 115.5194). Both have 2.54 mm pitch, 1.067 mm nominal holes and ~0.2535 mm rings. The schematic and model describe Molex 0022053041. The owner selected MODDIY CO975, an angled 3+1 fan header whose lead pattern and mating envelope are undocumented in the repository. Shared pitch alone cannot establish fit. Reversed mapping could put 12 V onto the fan tachometer lead.

**Action and acceptance:** Gauge actual leads and the drill pattern on a sample or 1:1 drawing. Verify keyed one-way seating, retention, contact mapping 1–4, mated clearance and wire bend with the selected fan. Record the approved connector and orientation in the assembly BOM. Holds connector assembly and powered fan use. Check leads before any order expected to fit the selected parts without rework. [J3 board](../../hardware/fan-controller.kicad_pcb#L6310) · [J4 board](../../hardware/fan-controller.kicad_pcb#L3684) · [Molex drawing](https://www.molex.com/pdm_docs/sd/022053041_sd.pdf) · [Owner selection](https://github.com/full-of-beans/cenmate-fan-controller/pull/6#issuecomment-6693782991) · [MODDIY](https://www.moddiy.com/products/5408/4-Pin-3-Pin-2.54mm-Pitch-2510-Fan-Male-Angled-Connector-Black.html)

### AUD26-005 — Shunt land geometry and takeoffs limit uncalibrated accuracy

**Major · Open.** RSH1, R3/R4, C2, U2.9/.10 and both measured 12 V nets. At (68.10, 85.59) and (68.10, 90.21), RSH1's two B.Cu lands are 2.70 mm axial × 3.42 mm transverse, with 1.92 mm inner gap and 7.32 mm outer span. Bourns recommends 3.10 mm axial × 4.00 mm transverse with 1.30 mm gap for the 1–3 mΩ part, and inner-edge sense taps. The board's separate thin traces meet the 4 mm current routing at the land centers. They share pad/solder/termination impedance. A 0.10 mΩ pickup contribution would cause 10% error. The nominal 1 mΩ ±1%, ±50 ppm/°C, 2 W part sees 7 mV and 49 mW at 7 A. INA228's specified ±1 µV offset is ±1 mA equivalent. Its two raw shunt scales give 0.3125 and 0.078125 mA/count nominally. These increments do not establish assembled accuracy.

**Action and acceptance:** Inspect joint wetting and calibrate at zero and multiple known currents through low, nominal and high load. Repeat at elevated temperature and with fan state changed at fixed enclosure current. Establish and meet an error budget for offset, gain, drift and fan coupling. If insufficient, adopt Bourns's inner-edge takeoffs and land pattern or qualify a true four-terminal shunt, then repeat. Holds quantitative telemetry and any current-dependent control. The existing board is suitable for a calibration prototype with first-article inspection. [Shunt board](../../hardware/fan-controller.kicad_pcb#L8027) · [Bourns p. 3](https://www.bourns.com/docs/Product-Datasheets/CRF.pdf) · [TI §10.1](https://www.ti.com/lit/ds/symlink/ina228.pdf)

### AUD26-006 — Stale STEP and unresolved fit of current assembly

**Major · Open.** H1–H4, J1–J5, U1 and the PCB/backplate. The committed STEP last changed at `5ff0fee` and contains TPS22810, with neither J5 nor Q3. The fresh assembly includes both current geometry assets. H1 (53.20, 51.70), H2 (77.80, 126.30), H3 (77.80, 51.70), H4 (53.20, 105.00) are grounded 2.20 mm plated holes. H4 moved 0.20 mm from X = 53.00, more than the nominal 0.10 mm radial clearance for an M2 screw before tolerances. The earlier owner Onshape check predates that shift. With Z = 0 at the bottom of the core, fresh J5 reaches +7.695 mm over X = 51.540–58.940 and KiCad Y = 89.809–101.309. Its mid-board model leads differ by ±0.120 mm in X from the drilled centers, with ~0.028 mm³ core intersection. This is a model registration issue. The official JST drawing governs the physical 2.50 mm pattern. J1 reaches −14.085 mm, J2 +11.595 mm, unmated Molex proxies +7.770 mm. U1's USB end reaches Y ≈ 49.189, about 0.19 mm inside the short edge. The XIAO model intersects the core by ~58.50 mm³, the NTC proxies by ~0.46 mm³ each and the Molex by ~2.66 mm³ per header. Q3 and RSH1 also use package proxies. The current XIAO footprint has 14 electrical edge lands and fabrication marks for discarded underside pads. Copper routes and covered vias beneath the module require real underside clearance and mask inspection.

**Action and acceptance:** Use a current STEP and 1:1 profile/drill print. Verify screw and insert axes, grounded hardware contact/insulation, R3 relief, tool access and actual mated J1–J5/USB envelopes including wire bends with the installed panel. Verify the XIAO sits without underside contact or solder bridges, and trim/inspect through-hole leads before closure. Fit-qualified assembly is held pending the mechanical check. Bare-board release is conditional on the current mounting and profile check when the backplate is already fixed. [Committed STEP](../../hardware/fan-controller.step) · [H4 board](../../hardware/fan-controller.kicad_pcb#L5513) · [Model directory](../../hardware/libraries/3dmodels) · [JST drawing](https://www.jst.com/wp-content/uploads/2025/06/eXH.pdf)

### AUD26-007 — Firmware failsafe and telemetry are unimplemented at this SHA

**Major · Open.** U1/U2, both PWM/tach and ADC nets, and `firmware/`. Only a placeholder is tracked. Hardware reset releases both PWM collectors via R9/R12. When USB is present, wrong inversion or a wedged high GPIO can instead command fan stop while Q3 remains on. Firmware has no Q3 gate output.

**Action and acceptance:** Use the pin map above and initialize to fan-side 100% with collectors released. Qualify around 25 kHz, invert output duty, confirm restart/minimum duty and tach pulses per revolution on each actual fan, and use a watchdog whose reset releases both collectors. Inject open/short sensors, stale or absent SMART/NAS data, stopped fans, boot/reset and INA/I²C failures. Verify defined conservative cooling and annunciation and the separate rocker fault path in AUD26-001. Start at 100 kHz I²C. A 4.7 kΩ pull-up allows about 251 pF at 1 µs Standard-mode rise and 75 pF at 300 ns Fast-mode rise using `tr ≈ 0.847RC`. For INA228, configure 0x40, conversion time, averaging and signed decoding. Example narrow range `ADCRANGE=1` with `CURRENT_LSB=25 µA` has ~±13.1 A signed reporting range and nominal `SHUNT_CAL=1311` for 1 mΩ after rounding the TI equation. Apply measured calibration, verify current sign, power units and stale/saturated handling, and use a faster separate capture for peaks hidden by averaging. Holds autonomous six-drive operation. Bare fabrication and static bench tests can proceed. [Firmware](../../firmware) · [TI calibration and registers](https://www.ti.com/lit/ds/symlink/ina228.pdf) · [I²C UM10204](https://www.nxp.com/docs/en/user-guide/UM10204.pdf) · [ARCTIC P8 Max](https://www.arctic.de/en/P8-Max/ACFAN00286A/)

### AUD26-008 — Fan power switching transients remain unqualified

**Minor · Requires Physical Validation.** Q3, R7, J5, J3/J4, `/12V_IN`, `/12V_SWITCH`. Q3 at (66.285, 110.6706) has ~25.48 mm of gate routing, with a B.Cu leg and direct rocker drive. C1–C4 are logic/sense components. The previous C5 disappeared with TPS22810. At 0.38 A total running current, 50 mΩ yields ~19 mV drop and 7.2 mW at the datasheet test condition. Starting both fans, switch bounce, cable inductance and a fan fault can cause ringing, a prolonged transition or rail droop. Q3's absolute ratings are 30 V drain-source and ±25 V gate-source, with separate SOA and thermal limits.

**Action and acceptance:** Capture VGS, VDS, switched supply and current during hot plug where applicable, both-fan startup, repeated toggles, shutdown and a simulated adverse load. Keep stress and switching energy within derated datasheet limits/SOA, respect actual fan/drive voltage limits and show repeatable transitions without resets. If needed, qualify local decoupling, clamping or gate damping with a repeated switching-energy check. Holds powered enclosure qualification pending testing. Bare fabrication, unpowered assembly and isolated dummy-load tests can proceed. [Q3 PCB](../../hardware/fan-controller.kicad_pcb#L4228) · [Diodes DS37386](https://www.diodes.com/datasheet/download/DMP3056L.pdf) · [ARCTIC P8 Max](https://www.arctic.de/en/P8-Max/ACFAN00286A/)

### AUD26-009 — INA228 bypass/filter placement and reference-plane detours

**Minor · Open.** U2 (65.90, 84.95), C1 (72.00, 83.10), C2 (54.60, 84.95), R3/R4 and logic/sense nets. C1.1 to U2.6 uses ~16.53 mm of VS routing and C1.2 sits ~8.26 mm from U2.7 center-to-center. C2's filtered routes to the IN+ and IN− pins are ~10.39 and 11.00 mm. The remote bypass and long post-filter runs increase sensitivity to switching noise. U2 escape spacing remains 0.20 mm. FAN2 tach and TH2 ADC also slot the B.Cu reference plane with 6.47 mm and 7.49 mm hops in an otherwise connected fill. Their route totals are 70.53 mm and 94.66 mm. These are layout exposures. No hardware noise failure has been measured.

**Action and acceptance:** Probe U2 VS relative to its ground pin, sample shunt readings at constant downstream load, and inspect FAN2 tach and TH2 ADC with both fans and drive load stepped. Verify stable I²C, no false tach edges and readings inside the defined noise/error budgets. If those checks fail, tighten the VS/GND loop and move C2 closer to the input pins, or improve affected return paths based on measured coupling. Holds accuracy qualification if budgets are exceeded. Bare fabrication, assembly and static bench work can proceed. [C1 board](../../hardware/fan-controller.kicad_pcb#L3488) · [C2 board](../../hardware/fan-controller.kicad_pcb#L6114) · [TI §9–10](https://www.ti.com/lit/ds/symlink/ina228.pdf)

### AUD26-010 — NTC self-heating, ADC reference and thermal calibration

**Minor · Requires Physical Validation.** R5/R6, TH1/TH2, C3/C4, ADC0/ADC1 and both ADC nets. R5/R6 now match R10/R13 at 10 kΩ and in procurement fields. Each NTC continuously sees 165 µA and dissipates 0.272 mW at 25 °C, implying ~0.27 °C rise at Murata's typical 1 mW/°C. Murata's 0.1 mA operating-current reference describes 0.1 °C self-heating. The divider remains within the 5 V rating, with nominal ADC span ~1.97–0.77 V over 15–60 °C. NTCs are 66.05 mm apart, with different local heat and airflow exposure. ADC reference and excitation must track for a ratiometric calculation.

**Action and acceptance:** Disable digital inputs/pulls on GPIO26/27 in ADC mode. Use `Rntc = Rpullup × Vnode/(Vexc−Vnode)` and the Murata R–T curve or qualified beta fit, with actual reference/excitation characterized. Allow several RC time constants on power/excitation changes and verify ADC channel settling. Test traceable divider resistances, both fault rails, actual NTC temperatures and board-to-drive temperature bias at varied fan speeds. Meet the agreed temperature error budget and show worst-bay margin. Holds thermal performance release pending results. Fabrication and assembly can proceed. [TH1 PCB](../../hardware/fan-controller.kicad_pcb#L1815) · [R5 schematic](../../hardware/fan-controller.kicad_sch#L9674) · [Murata part data](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8942/NCU18XH103F60RB.pdf) · [Murata current definition](https://www.murata.com/en-US/support/faqs/thermistor/ntc/char/0013) · [RP2350 ADC guidance](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)

### AUD26-011 — Assembly release data and land/stencil fit

**Minor · Open.** J2–J5, Q3, TH1/TH2, U1/U2, RSH1, BOM/CPL/paste and build documents. Neutral position exports include the XIAO and backside shunt. U1 has 14 edge-land apertures with no underside paste. U2 has ten 1.45 × 0.30 mm openings. A rectangular area-ratio estimate on 0.12 mm foil is ~1.04 at U2, against the common 0.66 release screening threshold. This does not establish vendor process acceptance. Q3's diode-named local footprint has nominally compatible lead numbering, pitch and body envelope for DMP3056L, with 0.559 × 1.27 mm lands, 1.83 mm paired pitch and 1.941 mm row spacing. DS37386 recommends 0.80 × 0.90 mm lands with 2.00 mm rows. TH1/TH2 have 1.00 × 0.97 mm pads and a 0.47 mm gap versus Murata's reflow guidance of 0.60–0.70 length, 0.60–0.80 width and 0.60–1.20 mm gap. These differences require joint/paste qualification. See AUD26-005 for shunt land geometry.

J5 lacks an explicit DigiKey field. Its own distributor link gives `455-S2B-XH-A-1-ND`. Q3 and J5's Datasheet fields are distributor SKU pages. Their manufacturer drawings are listed below. J5's holes are 0.9906 mm at 2.50 mm pitch. JST's 1.6 mm PCB pattern uses Ø0.90 +0.10/−0.00 mm and 2.50 ±0.05 mm pitch, with a qualification note for FR-4. The standard fab tolerance gives about 0.911–1.121 mm for the nominal 0.991 mm drill. Check actual fit and fill. The README describes the old TPS22810/USB enable and 33 kΩ. Circuit notes describe the completed 10 kΩ change as planned. A first article should use the current CAD BOM and an explicit assembly view.

**Action and acceptance:** Approve exact connector SKUs, J5 order number, U2 stock or a separately qualified substitute, part orientations, vendor-supported parts, which side is assembled, rails/fiducials, finish and final stencil. Decide explicitly who fits U1 and bottom RSH1. Check U2 pin 1, Q3 single-side drain and part marking, J5 pin 1/lock and the XIAO USB end before assembly. Inspect Q3, NTC, U2 and shunt joints on the first article. Fit SMD before tall through-hole parts and confirm J2 wire/tool access. Holds fit-qualified assembled order, with bare fabrication possible. [Q3 land pattern](https://www.diodes.com/datasheet/download/DMP3056L.pdf) · [Murata guide](https://www.mouser.com/pdfDocs/ncu18.pdf) · [JST](https://www.jst.com/wp-content/uploads/2025/06/eXH.pdf) · [J5 order SKU](https://www.digikey.com/en/products/detail/jst-sales-america-inc/S2B-XH-A-1/9961922) · [JLCPCB assembly rails](https://jlcpcb.com/help/article/specifications-for-adding-process-edges-and-positioning-holes) · [INA228 order listing](https://www.digikey.com/en/products/detail/texas-instruments/INA228AIDGSR/13691042)

### AUD26-012 — Accepted silkscreen and test-access limitations

**Minor · Accepted Risk.** J2 polarity legend, J3/J4 and J5 indexing, Q1–Q3, U1/U2 orientation and access to power/test nodes. Plots show 0.200 mm minimum legend-to-mask on front and 0.1545 mm on back, and zero ink overlap with openings or off the profile. Minimum front plotted stroke is 0.120 mm at C1 and back 0.127 mm near RSH1, below the vendor's general 0.150 mm threshold. J2 GND/12V text is clear in the plot at 1.0 mm height and 0.15 mm stroke, above the terminal's modeled outline. There are no dedicated test points. Use an assembly image for the fan and JST header pin-1 side.

**Action and acceptance:** Use approved prototype artwork, confirm critical labels in the vendor preview and first article, and verify actual continuity and polarity. Attach safe temporary probe leads before tall hardware or enclosure obstructs access. Inspect all close leads and avoid powered probing directly on adjacent VSSOP or transistor pins when other access is available. Does not independently hold bare fabrication. Pin identity and safe measurement access are gates before powered assembly work. [J2 artwork](../../hardware/fan-controller.kicad_pcb#L8435) · [Owner disposition](https://github.com/full-of-beans/cenmate-fan-controller/pull/6#issuecomment-6694086937) · [JLCPCB legend capabilities](https://jlcpcb.com/capabilities/Capabilities)

### AUD26-013 — INA228 width clause does not select free tracks

**Minor · Open.** U2 and custom rules lines 2–5. The rule pairs 0.20 mm clearance with a 0.20–0.25 mm track-width constraint using `A.Reference == 'U2' || B.Reference == 'U2'`. Footprint pads have an inherited U2 reference. Free tracks lack a footprint reference, and a unary width check has no U2 pad operand. Actual U2 escapes include 0.30 and 0.50 mm strokes while native DRC produces no width violations. The actual U2 0.20 mm clearance is verified. The width clause therefore does not enforce the apparent routing intention, and wider traces may be deliberate.

**Action and acceptance:** If a maximum is intended, target the relevant track objects by net/path or a dedicated rule area and demonstrate an out-of-policy case is detected in a test copy. If it is obsolete, document the real rule and retire the unused clause in a later revision. No independent gate on fabrication, assembly or operation. [Custom DRC rules](../../hardware/fan-controller.kicad_dru#L2) · [KiCad 10 rule language](https://docs.kicad.org/10.0/en/pcbnew/pcbnew.html#custom_design_rules)

### AUD26-014 — Accepted Q1/Q2 TO-92 copper, annular ring and mask webs

**Informational · Accepted Risk.** Q1/Q2 TO-92 inline E/B/C and their GND/base/collector nets. Q1 uses X = 58.73/60.00/61.27 at Y = 110.80. Q2 uses X = 71.04/72.31/73.58 at Y = 110.49. Both have a rectangular pin 1, 1.15 × 1.50 mm lands, Ø0.75 mm drill and 1.27 mm pitch. CAM measured 0.120 mm adjacent copper clearance and 0.120 mm mask webs on both sides, with Ø0.75 mm holes giving 0.200 mm nominal minimum rings. Hole-wall to adjacent different-net copper is 0.320 mm and drill-to-drill wall gap is 0.520 mm. A local pad-only rule allows 0.10 mm within each transistor footprint. JLCPCB's 1 oz green capability gives 0.020 mm nominal mask margin, while its generic two-layer order guide asks for 0.20 mm. Black/white 1 oz requires 0.13 mm and 2 oz any color requires 0.20 mm. The 0.20 mm ring is below the 0.25 mm preference and above the 0.18 mm published absolute minimum for two-layer 1 oz. Finished-hole tolerance can put this diameter between 0.67 and 0.88 mm, and nominal art alone cannot establish delivered ring.

**Action and acceptance:** Retain the owner-approved footprint and 1 oz green stack. Confirm separate openings/webs and ring acceptance in vendor CAM. Verify actual 2N3904BU lead fit and E/B/C orientation, then inspect and continuity-test the six solder joints. Fabrication can proceed with these process gates and no automatic redesign. [Q1](../../hardware/fan-controller.kicad_pcb#L2139) · [Q2](../../hardware/fan-controller.kicad_pcb#L5623) · [Local rule](../../hardware/fan-controller.kicad_dru#L7) · [Owner acceptance](https://github.com/full-of-beans/cenmate-fan-controller/pull/6#issuecomment-6693840017) · [JLCPCB capabilities](https://jlcpcb.com/capabilities/Capabilities) · [JLCPCB ordering guide](https://jlcpcb.com/help/article/instructions-for-ordering) · [onsemi package](https://www.onsemi.com/download/data-sheet/pdf/2n3904-d.pdf)

## CAM and fabrication measurements

| Item | Measured on fresh output and checked against process |
| --- | --- |
| Profile | One closed 30 × 80 mm contour, 7 lines and 7 arcs. Shortest segment 0.353554 mm. Largest reconstructed endpoint gap 0.000013 mm, closed at 0.000030 mm join tolerance. Four R2 corners and an R3 concave relief around (50.50, 112.10) blended by R1 arcs. Sampled maximum tangent deflection 0.285°. Verify route compensation, post clearance and standard ±0.20 mm CNC tolerance. |
| Copper | F.Cu route widths 0.30/0.50/0.75/1.00 mm. B.Cu 0.30/0.50/1.00/4.00 mm. Minimum different-net line/line and line/pad clearance 0.200 mm overall. Minimum pad/pad gap 0.120 mm at Q1/Q2. Smallest copper-to-profile 0.550 mm front and 0.30045 mm back. One B.Cu ground region and no isolated GND group. No copper outside route in the local parse. |
| Drills and slots | 36 round plated hits: 9 × 0.30, 5 × 0.40, 6 × 0.75, 2 × 0.991, 8 × 1.067, 2 × 1.30 and 4 × 2.20 mm. Three routed plated J1 slots: 1.00 × 3.30 mm and two 0.80 × 2.50 mm. Slot width and aspect meet published 0.50 mm and 2:1 criteria, and minimum modeled slot ring is ~0.2495 mm. All 39 drill centers, diameters and slot lengths match source to export precision. Empty NPTH file is expected. Standard fab publishes PTH/slot size +0.13/−0.08 mm and hole-position ±0.075 mm. Verify the three slots and secure mating pin fit and four plated mounting holes in vendor review. |
| Rings / hole spacing | 0.150 mm minimum via ring and 0.200 mm minimum component ring. At Q1/Q2 0.520 mm hole wall gap is above the 0.45 mm minimum pad-hole spacing. J2 ring is ~0.65 mm. H1–H4 use 4 mm ground lands with ~0.90 mm rings. The hole wall closest to the outline is J3.1 at ~1.173 mm. |
| Mask | 90 front and 27 back openings register to component lands. No via openings on either side. Mask plotting confirms tenting intent. Physical coverage or plug performance depends on processing. Q1/Q2 webs are the smallest at 0.120 mm. U2's adjacent minimum is 0.200 mm. Minimum front opening-to-unrelated-trace separation is 0.200 mm. |
| Legend and paste | Front and back legend meet local registration geometry. See AUD26-012 for thin strokes. Separate paste plots give 65 front openings (14 U1, 10 U2, 3 Q3, 38 passive) and 2 rear (RSH1). Review stencil and fit list before assembly. |

Use the [JLCPCB rigid PCB capability table](https://jlcpcb.com/capabilities/Capabilities) and [ordering notes](https://jlcpcb.com/help/article/instructions-for-ordering) for order-specific review. This audit verified nominal exported geometry locally. Fabricator compensation, delivered mask dams/rings, finished hole sizes, final finish and assembly stock were outside the available evidence.

## BOM cross-check

| Refs | Qty | MPN | DigiKey identifier or disposition |
| --- | ---: | --- | --- |
| C1–C4 | 4 | Samsung CL21B104KACNFNC, 100 nF X7R 25 V 0805 | 1276-2442-1-ND |
| J1 | 1 | Tensility 54-00286 | 839-54-00286-ND |
| J2 | 1 | Phoenix 5442206, 5.08 mm | 277-12547-ND |
| J3, J4 | 2 | Molex 0022053041 in CAD | WM4302-ND for CAD part. Actual MODDIY approval pending AUD26-004. |
| J5 | 1 | JST S2B-XH-A-1(LF)(SN) | 455-S2B-XH-A-1-ND in listing, field empty |
| Q1, Q2 | 2 | onsemi 2N3904BU | 2N3904FS-ND |
| Q3 | 1 | Diodes DMP3056L-7 | DMP3056L-7DICT-ND |
| R1, R2 | 2 | Yageo RC0805FR-074K7L, 4.7 kΩ | 311-4.70KCRCT-ND |
| R3, R4 | 2 | Yageo RC0805FR-0710RL, 10 Ω | 311-10.0CRCT-ND |
| R5, R6, R10, R13 | 4 | Yageo RC0805FR-0710KL, 10 kΩ | 311-10.0KCRCT-ND |
| R7, R9, R12 | 3 | Yageo RC0805FR-07100KL, 100 kΩ | 311-100KCRCT-ND |
| R8, R11 | 2 | Yageo RC0805FR-071KL, 1 kΩ | 311-1.00KCRCT-ND |
| RSH1 | 1 | Bourns CRF2512-FZ-R001ELF, 1 mΩ | CRF2512-FZ-R001ELFCT-ND |
| TH1, TH2 | 2 | Murata NCU18XH103F60RB, 10 kΩ NTC 0603 | 490-16279-1-ND |
| U1 | 1 | Seeed 102010550 XIAO RP2350 | 1597-102010550-ND |
| U2 | 1 | TI INA228AIDGSR, VSSOP-10 | 296-INA228AIDGSRCT-ND |

J2 is the exact 5442206. The Phoenix listing gives 13.5 A nominal and separate 10 A conditions depending on approvals and wire configuration. Use the specified conductor and torque. No INA226, TPS22810, TRACO, C5, or alternate Phoenix 1729128 is fitted in the schematic or PCB. Unused reference assets in the local libraries do not add to the BOM. Distributor listings establish part identity, while current inventory or contract assembler availability must be checked at order. [Phoenix ratings](https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-bc-508x10-2-gn-5442206) · [10 Ω DigiKey listing](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0710RL/727534) · [10 kΩ DigiKey listing](https://www.digikey.com/en/products/detail/yageo/RC0805FR-0710KL/727535) · [Samsung capacitor listing](https://www.digikey.com/en/products/detail/samsung-electro-mechanics/CL21B104KACNFNC/3888100)

## Historical reconciliation

Compared the earlier [schematic verification](../controller-schematic-verification.md), [PR #6 engineering review and dispositions](https://github.com/full-of-beans/cenmate-fan-controller/pull/6#pullrequestreview-5802921394) at `e81f74b` and its older F-series cross-references, [parity](../schematic-pcb-parity-audit.md) at `9633bbb`, [manufacturing](../pcb-manufacturing-audit.md) at `4f49d32`, and [November 12 layout audit](pcb-layout-audit-2026-11-12.md) at `5ff0fee`. Current evidence comes from the exports and source above.

| Earlier significant issue | Classification at AUDITED_COMMIT | Current evidence |
| --- | --- | --- |
| Schematic verification A5/C4/G2, original supply/switch, power budget and panel | Unverified | Carrier topology changed. Actual USB source, rocker harness, backplate and thermal envelope still need tests. AUD26-001, 002, 006. |
| Old TRACO dual-power USB backfeed C3 and TRACO/mux/remote NTC architecture | Resolved for the feed mechanism, obsolete for old architecture | No local 5 V feed and no dual-feed VBUS connection. U1.14 is a single carrier net. No converter, mux or remote strip. Partial-power GPIO and ground-path qualification still apply. |
| Old current monitor part references U3, original shunt value, 100 Ω filter | Obsolete | Live design U2 INA228, RSH1 1 mΩ, R3/R4 both 10 Ω, 100 nF differential C2. |
| Schematic verification E3/E4 and D4, sensor and fan behavior | Still open | Continuous NTC excitation restores the ~0.27 °C screening estimate at 25 °C, assembly land deviation remains and fan electrical behavior must be measured. AUD26-001, 004, 010, 011. |
| PCB-001, isolated TPS22810 ground | Resolved historically, obsolete in current topology | Earlier parity review verified ground repair. TPS22810 and the pad are gone. Netlist, DRC and CAM confirm the routed Q3/J5/R7 implementation and its GND return. |
| PCB-002/F03, L26-11, J1 rating | Accepted risk | J1 remains 7 A and all load branches pass through it. Real startup and temperatures unmeasured. AUD26-003. |
| PCB-003/F04, L26-02, fan-header selection and fit | Unverified | CAD still uses Molex. Owner-intended MODDIY fit and key mapping require physical check. AUD26-004. |
| PCB-004, Bézier relief kink | Resolved | Plotted contour now has seven circular arcs, R3 relief/R1 blends and sampled deflection below 0.3°. As-built fit/CAM compensation still require review. |
| PCB-005/F07/F13, L26-03 and L26-08, panel fit/model interference | Still open | H4 shift and J2 harness fit remain. Fresh STEP adds J5/Q3 and reproduces known overlaps on U1, NTCs and Molex proxies. Checked-in assembly is stale. AUD26-006. |
| PCB-006/F09, L26-04, shunt sensing | Still open | Two-terminal part and shared terminal/current lands still present. Bourns pattern and inner-edge recommendations quantify the deviation. AUD26-005. |
| PCB-007/F11, L26-05, protection and USB-dependent fans | Partially resolved | Fan power is independent of VBUS. Main protection/ground-path qualification remains and an open J5 disables cooling. AUD26-001, 002. |
| PCB-008/F05, L26-07, TPS22810 bypass/C5 placement | Obsolete for TPS22810 and C5, still open for current transients | C5 and old switch removed. Q3 has no local 12 V bypass or configured slew ramp. AUD26-008. |
| PCB-009/F06, L26-06 and L26-09, bypass/return | Still open | Current routed C1/C2 and reference-plane detours verified. Plane is connected, noise has not been measured. AUD26-009. |
| PCB-010/F08, stale refs and rules | Partially resolved | Native parity clean, Q1/Q2 footprint assignments match, correct U2 reference in DRC rule. Width scope still ineffective for free tracks. AUD26-013. |
| PCB-011/F12, stale notes and terminal polarity | Partially resolved | J2 board and polarity labels match. README still gives old switch/dividers and STEP is stale. AUD26-006, 011. |
| PCB-012/014/F01 and L26-12, Q1/Q2 rings and mask | Accepted risk | 0.20 mm rings resolved the 0.15 mm historical nominal geometry. Measured 0.120 mm webs still use green 1 oz, with vendor confirmation. AUD26-014. |
| PCB-013, L26-13, silkscreen and test access | Accepted risk | Improved yet some <0.15 mm ink remains. Final CAM markings and probe access qualify the owner's disposition. AUD26-012. |
| PCB-015/F02, default enable bias | Obsolete for TPS22810 and resolved for Q3 default | U3 EN is gone. Q3 has source-referenced 100 kΩ, yielding OFF with open switch connection. System failsafe remains AUD26-001. |
| PCB-016/F10, MPN coverage; L26-01 R3/R4/U2 metadata | Resolved | 29/29 schematic parts have MPNs. R3/R4 10 Ω and U2 INA228 agree in schematic, board, BOM and CPL. J5 missing distributor field is a new minor release item. AUD26-011. |
| Nov 6 parity exclusions, duplicate mounts and J2 symbol filters | Resolved | Four unique grounded board-only refs, correct J2 local footprint, zero native parity/ERC or independent parity problems. |
| Nov 6 manufacturing registration, slots, web and silk hold | Partially resolved | Current nine files and 39 drills verified. Local silk overlap and registration pass, thin strokes accepted with conditions. Final vendor preview for slot/outline, mask and legibility remains open. |
| L26-10, SMD exports are unqualified | Still open | Fresh quantities reconcile. Which side is populated, the XIAO/shunt fit list and vendor acceptance remain unknown. AUD26-011. |

## Outstanding physical tests and limitations

1. **Before ordering:** Check 1:1 tooling pattern, real intended J3/J4 and J5, screw/insert location, and panel relief. Review the vendor's 1 oz green CAM for ring/web, slots, adjusted outline, hole tolerances, and polarity/silk. Review the assembly quote, fit list, stencil and mounting for any work sent out.
2. **Unpowered first article:** Inspect all plated openings/slots, mask, joints and XIAO seating. Verify J1/J2/J3/J4/J5 polarity and harness continuity. Check J1.B isolation and continuity through the shunt and main return. Confirm no shorts across supply, Q1/Q2 lead pairs and sensitive IC pins. Identify which metal mounts bond to the rig.
3. **Controlled power-up without drives:** Use isolated/current-limited bench supplies and load emulators. Exercise 12 V only, USB only, both and neither, in both application/removal orders, with rocker ON/OFF/disconnected. Include enclosure USB suspend/restart where relevant. Observe gate/output stress, USB/3.3 V and GPIO back-power, alternate return currents and intended fan default, then verify both fan PWM/tach signals and stalled-fan behavior. Check polarity again before the enclosure hookup.
4. **Calibration and functional validation:** Check INA228 zero, known currents, positive sign, bus voltage, resolution and fan-correlated artifacts. Verify NTC known-resistance and physical temperature sweeps, ADC settling and fan noise. Inject reset/watchdog, sensor, tach, stale telemetry and bus faults, and check the safe fallback at both fans.
5. **Loaded integration:** After the prior gates, measure cold spin-up of all six actual drives and both fans, J1 current/contact drop and temperature, J2 supply droop, branch temperatures and the worst bay's drive temperature during sustained load. Verify manufacturer ratings and drive stability. Record fan model, HDD models, harness, supply, ambient, scope/current capture and cooling configuration so the envelope is reproducible.

No actual vendor CAM order, live Onshape/backplate assembly, physical hardware, thermal/EMC/transient simulation, sourcing commitment or firmware execution was available. The published Seeed pin map and carrier topology were sufficient for pin assignment. The Seeed schematic PDF did not extract cleanly with the local PDF text tool, so module-internal partial-power behavior is a physical validation item. The fresh STEP provides geometry for the modeled assembly. It excludes true mated MODDIY/fan/USB/rocker plugs, the backplate, harness and fastener stack. Circuit estimates are nominal engineering calculations. Design files, stock library files and source reports were not edited.

## Reproduction

Use KiCad 10.0.8 with its matching stock symbol and footprint libraries and project-local library tables. The Linux Lite AppImage was extracted with `--appimage-extract` and launched using `APPDIR=/path/to/squashfs-root /path/to/squashfs-root/AppRun kicad-cli …`, with `XDG_CONFIG_HOME` set to an isolated directory and the bundle's `usr/share/kicad/template/{fp-lib-table,sym-lib-table}` copied to the config's `kicad/10.0/`. AppImage SHA-256 `4dfc6a52f89aa072d206e466a2a1de4c5ef5975d7ef49da24e4d4db0bb40ba3b`. Set `KICAD10_3DMODEL_DIR` to the matching model tree. The four referenced stock models were obtained from the KiCad packages3D repository. Project stack and rules come from the checkout. Extra Python dependencies can be installed in a temporary venv with `pip install gerbonara==1.6.3 shapely==2.1.2`.

```bash
git fetch origin initial-schematic
baseline=3294cb89a1b5f34098adf01730dca6feb2a4a3dd
out=$(mktemp -d)
git worktree add --detach "$out/src" "$baseline"
cd "$out/src"
cli=kicad-cli  # Or absolute path to a wrapper launching the 10.0.8 AppImage.
p=hardware/fan-controller

git rev-parse HEAD
"$cli" version
"$cli" sch erc --severity-all --exit-code-violations -o "$out/erc.rpt" "$p.kicad_sch"
"$cli" pcb drc --severity-all --all-track-errors --schematic-parity \
  --exit-code-violations -o "$out/as-stored.rpt" "$p.kicad_pcb"
"$cli" pcb drc --severity-all --all-track-errors --schematic-parity \
  --refill-zones --exit-code-violations -o "$out/refill.rpt" "$p.kicad_pcb"
"$cli" sch export netlist --format kicadxml -o "$out/net.xml" "$p.kicad_sch"
"$cli" pcb export ipcd356 -o "$out/board.d356" "$p.kicad_pcb"
python scripts/check-schematic-pcb-parity.py "$out/net.xml" "$out/board.d356" "$p.kicad_pcb"
cli=$(command -v "$cli")
KICAD_CLI="$cli" bash scripts/export-pcb-manufacturing.sh "$out/cam"
python scripts/inspect-pcb-manufacturing.py "$out/cam" "$p.kicad_pcb" > "$out/cam.log"
"$cli" pcb export gerbers -l F.Paste,B.Paste -o "$out/paste/" "$p.kicad_pcb"
"$cli" pcb export pos --format csv --units mm --smd-only --side both \
  -o "$out/position.csv" "$p.kicad_pcb"
"$cli" sch export bom --fields 'Reference,Value,Footprint,QUANTITY,Manufacturer,MPN,DigiKey_PN,Datasheet,Description' \
  --labels 'Reference,Value,Footprint,Quantity,Manufacturer,MPN,DigiKey_PN,Datasheet,Description' \
  --group-by 'Value,Footprint,MPN' --exclude-dnp -o "$out/bom.csv" "$p.kicad_sch"
"$cli" pcb export step --force --board-only -o "$out/bare.step" "$p.kicad_pcb"
"$cli" pcb export step --force --no-dnp -o "$out/assembly.step" "$p.kicad_pcb"
git status --short
```

To reproduce the refill equivalence, extract `git archive "$baseline" hardware scripts` into a separate temporary copy, rerun DRC there with `--refill-zones --save-board` and repeat the CAM export/inspection there. Compare seven Gerbers and both drill files with timestamps removed (`CreationDate` and date comments). For the add-on source checks, parse board S-expressions with the parity script's reader, compare schematic XML Datasheet/Description to PCB properties, sum routed segment lengths by net/layer, and compare pad dimensions and takeoffs against manufacturer drawings. Parse paste by file function and flash reference. Use FreeCAD `Import.insert` on the fresh and checked-in STEP files, select `fan-controller_PCB` and confirm one valid solid and bbox/volume. On fresh absolute-origin assembly shapes, use `shape.common(core).Volume` for listed collisions and section J5 at core Z = 0.75 mm to check registration to bore centers. Flip Y when comparing CAM/STEP coordinates to PCB. Use the vendor preview and first-article inspection to close physical checks.

## Follow-up review

Fetch the future branch and record its full SHA as a new immutable review target. Preserve this report and all AUD26 IDs. A documentation-only audit commit should appear in history but produces no design diff.

```bash
git fetch origin initial-schematic
audited=3294cb89a1b5f34098adf01730dca6feb2a4a3dd
future=$(git rev-parse origin/initial-schematic)
git log --oneline "$audited..$future"
git log "$audited..$future" -- hardware/ docs/ firmware/ mechanical/ scripts/
git diff --name-status "$audited" "$future"
git diff "$audited" "$future" -- hardware/ docs/circuit-design.md README.md firmware/ mechanical/ scripts/
git worktree add --detach /path/to/new-review-copy "$future"
```

Use endpoint diffs for hardware and focused `git diff "$audited" "$future" -- <paths>` around each finding. For schematic or library edits, rerun ERC, XML, IPC-D-356 parity and BOM. For PCB or rule edits, rerun refilled DRC and Gerber/drill geometry, with specific attention to affected nets, hole centers, clearance, Q1/Q2 mask webs and stencil. For model, placement or drill changes, redo STEP, mating and outline checks. Repeat bench evidence for changed rocker/power, shunt, fans, NTCs or firmware. If the audited SHA is no longer an ancestor, report the divergence and use the same two endpoint trees with both histories available.

Maintain a separate follow-up record containing each original AUD26 ID, both SHAs, intervening commit(s), new evidence, rerun commands, stage verdict and one status. Mark **resolved** when acceptance evidence passes, **partially resolved** when only some acceptance conditions are met, **unchanged** when current evidence reproduces the same issue or accepted exposure, **regressed** when the comparison demonstrates worse results or reintroduces a closed defect, and **unverified** when relevant evidence or a rerun is missing. Give new issues new IDs. Never renumber these findings or overwrite this baseline audit.
