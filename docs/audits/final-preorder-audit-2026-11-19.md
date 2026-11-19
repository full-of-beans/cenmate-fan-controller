# CENMATE fan controller — final pre-order engineering and manufacturing audit

Audit date: November 19, 2026, America/New_York.

| Baseline | Value |
| --- | --- |
| Repository / branch | `full-of-beans/cenmate-fan-controller` / `initial-schematic` |
| **AUDITED_COMMIT** | **`8cc880a96dea908ae958eecb40e3e9b91f2e8c4f`** |
| Previous full audit | `3294cb89a1b5f34098adf01730dca6feb2a4a3dd`, [report](full-design-audit-2026-11-15.md) |
| Previous follow-up | `482af6a78fe4e53589d44535311f52b8d44c7214`, [report](fabrication-readiness-followup-2026-11-17.md) |
| Fabrication basis | Two-layer rigid FR-4, 1.6 mm nominal, 1 oz copper, green solder mask; prototype quantity |
| Publication | This report is the only production-tree addition. Its containing Git commit is the documentation commit; the design baseline remains the SHA above. |

## 1. Executive summary and release decision

**Yes: the current PCB can be ordered as a bare-board engineering prototype without another electrical or layout revision. No mandatory bare-PCB design correction was found.** Fresh electrical, PCB, connectivity and CAM checks support fabrication. This is not a release of an assembled, mechanically qualified or unattended six-drive system.

Before paying, approve the correct nine fabrication streams and the vendor's interpretation of the plated slots, outline and Q1/Q2 mask/rings. The intended MODDIY headers and backplate have not been independently fit-qualified. Check them before ordering if an exact, rework-free mechanical fit is required; otherwise treat physical fit as an explicit prototype risk. Ordinary first-article inspection, calibration, current-limited bring-up and six-drive load testing can wait until boards arrive.

| Release stage | Decision | Boundary |
| --- | --- | --- |
| Schematic and electrical design | **CONDITIONAL PASS** | Nominal topology and device mapping pass; actual rocker, fault returns, transients and partial-power behavior need qualification. |
| PCB layout and routing | **CONDITIONAL PASS** | No open, short or routing regression found. Measurement accuracy and coupling require prototype measurements. |
| Bare PCB manufacturing | **READY** | For the stated stack and vendor CAM approval. No required artwork edit identified. Exact installed fit remains a prototype assumption. |
| Mechanical fit | **NOT VERIFIED** | Fresh/committed model comparison passes except a stock U2 model variant; actual panel, plugs, headers and fasteners are unavailable. |
| Component assembly | **CONDITIONAL** | Hand assembly is practical subject to first-article fit and solder inspection. **Hold a turnkey assembled order** until BOM/placement/stencil and FINAL26-001 are closed. |
| Initial powered bench testing | **CONDITIONAL** | After unpowered inspection, use current-limited supplies and dummy loads; no drives initially. |
| Full six-drive enclosure operation | **HOLD** | Wiring, protection, fan defaults, firmware, current/contact heating and actual cooling remain unqualified. |

New findings: **0 Blocker, 0 Major, 2 Minor**. FINAL26-001 is an assembly-metadata regression. FINAL26-002 records stale verification evidence. Neither changes bare-board copper or drill suitability. Existing accepted jack, mask and legend risks remain applicable because their relevant geometry/topology has not materially worsened.

## 2. Baseline, changes and audit method

Fetched/cloned GitHub, independently read the branch ref through the GitHub connector, and created a detached worktree at AUDITED_COMMIT. The ref and fetched design agreed. A second `git fetch origin initial-schematic` immediately before publication still returned AUDITED_COMMIT; no branch divergence was observed. All design inspection and exports used that immutable worktree. A separate archive copy was used for a saved zone refill; production files were not saved or repaired.

| Commit since November 17 reviewed SHA | Change |
| --- | --- |
| `683e7b9a288c784a8cc023fdc26b3531e5218f37` | Historical follow-up report |
| `cb99627` | Restored RSH1 procurement and descriptive fields on PCB |
| `8cc880a` | Rerouting, small J5/R3/R4/R7 moves, footprint updates, refreshed CAM/STEP |

Endpoint comparison found eight changed placed footprints, 247 → 242 track segments, 38 removed / 33 added segments, and 14 vias at both endpoints. J5 moved (56.4901,97.4085) → (56.1000,97.4000), R3 (61.8,93.1) → (61.5,93.1), R4 (59.1625,82.6) → (59.7,82.6), R7 (61.69,98.3) → (61.5,98.3). J1, J2, U1 and RSH1 centers did not move. J1.A/C and J2.1 now explicitly request solid zone connection; J1.C has a broad solid ground attachment. J2.2 remains connected through the existing routed/filled ground geometry.

The input sense via moved (63.7,87.0) → (63.9,87.2). Sense, I²C and gate routing changed. The shunt lands, C1/C2/U2 placement, main HDD tracks, outline, mounting pattern, other component models and circuit pin mapping remain unchanged. U1/J5 library and placed updates removed fields and added fabrication artwork; electrical pad geometry is unchanged apart from J5's translation. The schematic, project rules, project file and firmware are unchanged from the previous reviewed design. Updated STEP and fabrication files are current geometrically. Connectivity was rechecked independently across unchanged areas too.

### Tools and limitations

Used KiCad **10.0.8-10.0.8~ubuntu24.04.1** (CLI binary SHA-256 `a2625a7d5c3fc5be5e892927d0c0580ff1583f7afff32fc6e0f615bd316a7481`), matching locally available KiCad 10 libraries/models, Git **2.56.1**, Python **3.12.14**, Gerbonara **1.6.3**, Shapely **2.1.2**, and FreeCAD **1.0.0** with Open Cascade. The installed system KiCad 9.0.2 was identified and not used for checks. KiCad 10 was launched through an isolated wrapper setting `LD_LIBRARY_PATH`, `XDG_CONFIG_HOME`, `KICAD10_SYMBOL_DIR`, `KICAD10_FOOTPRINT_DIR` and `KICAD10_3DMODEL_DIR` to the available 10.0.8 toolchain. No prior audit pass was used as current verification evidence.

No physical board, actual connectors, live Onshape assembly, backplate file, vendor order/CAM decision, thermal/EMC simulation or executable controller firmware was available. Manufacturer PDFs were freshly obtained and read, including documents whose web text fetch failed. Calculations below are nominal screening estimates, not measured current ratings or IPC thermal certification. Model intersections and route lengths cannot establish delivered tolerances, noise or solder quality. Mechanical placeholders do not establish backplate fit.

## 3. Independent schematic and electrical review

XML netlist, embedded schematic symbols, PCB pin nets, part properties and manufacturer documents were cross-checked. All 30 fitted references and all 88 schematic pin/pad assignments agree electrically. Four unique grounded board-only mounting holes are excluded from BOM/placement. Explicit NCs are J1.B, U1.3/.4/.7 and U2.3; each remains isolated from other nets. `+5V` is a single carrier pad U1.14 with only ~0.033 mm total short stubs, not a second power feed.

| Circuit | Independently verified design and operating implications |
| --- | --- |
| 12 V HDD distribution | J1.A center positive → `/12V_IN` → RSH1.2 → RSH1.1 → `/12V_HDD` → J2.1. J1.C sleeve and J2.2 are GND; switched contact B is unused. U2.8 reads downstream bus voltage. Fans branch upstream of RSH1, so fan current is excluded; enclosure accessory/USB loads downstream are included. |
| Connector limits | Actual BOM is Tensility **54-00286**, 7 A, 5.5/2.1 mm, and Phoenix **5442206**, not 1729128. Phoenix lists 13.5 A nominal with approval/application-dependent limits, 5.08 mm pitch, 1.3 mm PCB holes and 1 × 0.5 mm leads. Use permitted conductors, 6 mm strip and 0.5–0.6 N·m torque; support the housing while tightening. A 9 A brick does not enforce the jack's 7 A rating. [Tensility](https://tensility.s3.us-west-2.amazonaws.com/imports/product_spec_sheets/54-00286.pdf), [Phoenix](https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-bc-508x10-2-gn-5442206). |
| Shunt and polarity | Bourns **CRF2512-FZ-R001ELF**, 1 mΩ, ±1%, ±50 ppm/°C, 2 W rating subject to derating. Upstream pickup feeds R3 → U2.10 IN+; downstream pickup feeds R4 → U2.9 IN−. Positive load current produces positive differential voltage. At 7 A: 7 mV, 49 mW; at 9 A: 9 mV, 81 mW. Neither value qualifies the connector for 9 A. [Bourns CRF](https://www.bourns.com/docs/product-datasheets/crf.pdf). |
| INA228 | **INA228AIDGSR**, DGS VSSOP-10. Pins 1 A1, 2 A0 and 7 GND grounded (address 0x40); 3 ALERT NC; 4 SDA, 5 SCL, 6 VS 3.3 V, 8 VBUS downstream, 9 IN−, 10 IN+. R1/R2 4.7 kΩ, R3/R4 10 Ω, C1 100 nF supply bypass, C2 100 nF differential on IC side. Differential RC = 20 Ω ×100 nF =2 µs, approximately 79.6 kHz. Both shunt ranges exceed operating current requirements: ±163.84 A or ±40.96 A with 1 mΩ. Nominal raw steps 0.3125/0.078125 mA do not equal accuracy. ±1 µV input offset equates to ±1 mA before other errors. TI permits bus/sense common-mode input with VS off provided GND remains connected. [TI INA228](https://www.ti.com/lit/ds/symlink/ina228.pdf). |
| Fan power and rocker | Q3 **DMP3056L-7**: 1 gate, 2 source input, 3 drain switched output. R7 100 kΩ gate-to-source; J5.1 gate, J5.2 GND. Grounded gate at 12.2 V gives approximately −12.2 V VGS and 122 µA rocker load. Nominal values are within −30 V VDS and ±25 V VGS absolute limits. Open J5 pulls Q3 OFF while HDD feed remains live. Q3 has no controlled ramp or circuit current limit; its body diode permits backfeed from an externally energized fan output toward input. Test the actual rocker and transients. At 0.38 A and 50 mΩ, nominal Q3 drop/loss is ~19 mV/7.2 mW at the specified datasheet condition. [Diodes](https://www.diodes.com/datasheet/download/DMP3056L.pdf). |
| PWM | J3/J4: 1 GND, 2 switched 12 V, 3 tach, 4 PWM. Q1/Q2 **2N3904BU** E/B/C =1/2/3, emitters grounded, bases through 1 kΩ with 100 kΩ pull-down. GPIO high sinks the fan PWM line; firmware duty is inverted. Approximate base current (3.3−0.7)/1000 =2.6 mA. Reset/unpowered GPIO releases the collector, but actual fan open-PWM behavior must be tested. [onsemi](https://www.onsemi.com/download/data-sheet/pdf/2n3904-d.pdf). |
| Tach | Each header tach goes directly to its GPIO with 10 kΩ pull-up to 3.3 V, maximum pull-up current ~0.33 mA. There is no added 12 V level protection. Verify actual fan tach output is compatible open collector and does not drive an unpowered MCU. |
| Thermistors | TH1/TH2 **NCU18XH103F60RB**, 10 kΩ ±1%, nominal B25/50 3380 K; 10 kΩ upper resistors, 100 nF to GND at ADC nodes. At 25°C: 1.65 V, 5 kΩ Thévenin, 0.50 ms nominal RC, ~0.272 mW NTC power. Temperature rises make voltage fall. Nominal dissipation-constant screening gives ~0.27°C self-heating; board/airflow bias is likely more important and unmeasured. [Murata NCU18](https://www.murata.com/-/media/webrenewal/products/thermistor/ntc/ncu/ncu18.ashx?la=en). |
| XIAO power | USB-C powers U1; edge 12 supplies external 3.3 V, edge 13 GND. No carrier converter or second 5 V feed remains. External steady demand is approximately 9 mA with both PWM bases driven and logic pull-ups sinking, plus module demand/transients. USB source/cable headroom is unmeasured. [Seeed](https://wiki.seeedstudio.com/xiao_rp2350_arduino/). |

| Carrier function | U1 edge pad / label | RP2350 assignment |
| --- | --- | --- |
| TH1 / TH2 | 1 D0 / 2 D1 | GPIO26 ADC0 / GPIO27 ADC1 |
| SDA / SCL | 5 D4 / 6 D5 | GPIO6 / GPIO7, I²C1 |
| Fan 1 PWM / tach | 9 D8 / 8 D7 | GPIO2 PWM slice 1A / GPIO1 |
| Fan 2 PWM / tach | 10 D9 / 11 D10 | GPIO4 PWM slice 2A / GPIO3 |

Assignments agree with [Seeed pin documentation](https://wiki.seeedstudio.com/XIAO_RP2350_Pin_Multiplexing/) and the [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf). GPIOs are 3.3 V interfaces. Validate actual partial-power pin levels rather than assuming the fan or module provides isolation.

There is no carrier fuse, reverse-polarity protection or surge clamp. Ground is shared by J2 return, USB, J5 and mounts. An open/resistive main return can divert enclosure current through auxiliary cables or metalwork. A connected ground plane does not solve this integration fault. Establish polarity, strain relief, return integrity and fault clearing using low-current dummy-load tests before enclosure hookup. These are existing integration qualifications; no new nominal electrical mistake was found.

## 4. PCB routing, high-current path and measurement integrity

### Main current and return

All HDD series copper remains B.Cu, with no series via. The 4 mm tracks total **11.2816 mm**; two 3 mm segments total **1.40 mm**, both inside the wider shunt lands. The shunt lands are 4 mm transverse. J1's smaller slot land is reinforced by the overlapping 4 mm route; J2's 2.6 mm land overlaps that route. Neither creates an exposed long, thin neck in the series feed. Contact, solder and terminal constriction remain real losses to measure.

Using ρ=1.724×10⁻⁸ Ω·m at 20°C, t=35 µm and R=ρL/(wt), conservative centerline summation gives **~1.62 mΩ** positive copper, **~2.62 mΩ including the nominal shunt**. Wide pad overlaps make the approximation conservative locally; manufacturing thickness/etch tolerances and heat can increase resistance.

| Current | Positive copper drop / heat | Copper + shunt drop / heat |
| --- | --- | --- |
| 7 A | ~11.3 mV / 79 mW | ~18.3 mV / 128 mW |
| 9 A, calculation only | ~14.6 mV / 131 mW | ~23.6 mV / 212 mW |

These exclude return, connectors, harness and solder; at 60°C copper resistance is approximately 16% higher. No justified ampacity or temperature-rise conclusion can be derived solely from these losses. Jack/plug rating is the clearer load constraint.

The refilled B.Cu ground is **one connected region, ~2020.81 mm²**. CAM continuity includes all ground pads after plating. Across Y=80.15, side return corridors are ~13.20 and 4.90 mm; across Y=87.90, ~11.04 and 11.51 mm. J1.C and J2.2 have broad plane access; no via-only HDD return or newly isolated ground island was found. Region area increased modestly from the previous layout, which is geometric evidence rather than proof of lower noise.

Fan input routing is approximately 44.43 mm of 1 mm copper with layer transitions, unchanged. Ideal trace resistance ~21.9 mΩ gives ~8.3 mV/3.2 mW at 0.38 A, excluding vias and Q3. Output routing is 1 mm and splits to the headers. Its shared current is far below the HDD branch; no new fan bottleneck was found. Start/stall/fault current remains unmeasured. ARCTIC specifies 0.19 A per P8 Max and a stop region below 5% PWM; neither specifies this assembly's startup transient. [ARCTIC](https://www.arctic.de/en/P8-Max/ACFAN00286A/).

### Kelvin pickups, filters and decoupling

RSH1 center (68.1,87.9), B.Cu, 90°. Lands centered (68.1,85.7) and (68.1,90.1), each **3.10 mm axial ×4.00 mm transverse**, inner gap **1.30 mm**. They match Bourns's recommended 1–3 mΩ pattern. Current enters/leaves from opposite outer sides; separate 0.30 mm sense branches leave the facing inner edges. The long parallel gap segment now has 0.60 mm center spacing, hence 0.30 mm copper gap, versus 0.20 mm previously. The traces are monitor branches, not HDD current necks.

Two-terminal solder/termination spreading resistance, asymmetrical joints and thermal EMF remain in the measurement. A 0.1 mΩ residual contribution would produce 10% gain error. This is a calibration exposure, not a demonstrated unmanufacturable footprint or reason to require a four-terminal redesign.

| Route, pad-center convention | Nov 17, mm | Current, mm | Interpretation |
| --- | ---: | ---: | --- |
| C1.1 → U2.6 | 6.93 | 6.93 | Unchanged 0.30 mm supply leg |
| C1.2 → U2.7 | 5.38 | 5.38 | Unchanged direct 0.30 mm ground leg |
| C2.1 → U2.10 | 5.23 | 5.23 | Unchanged filtered IN+ |
| C2.2 → U2.9 | 3.89 | 3.89 | Unchanged filtered IN− |
| R3.2 → C2.1 | 4.50 | 4.37 | Slightly shorter |
| R4.2 → C2.2 | 5.04 | 4.82 | Slightly shorter |
| RSH1.2 → R3.1 | 15.01 | 14.77 | Slightly shorter upstream pickup |
| RSH1.1 → R4.1 | ~13.70 | 13.47 | Slightly shorter downstream pickup |

Lengths exclude plated depth; treatment of endpoints within pad copper can change centerline totals by a fraction of a millimeter. No matching of precision differential propagation delay is required here. What matters is pickup impedance, loop area, filtering and coupled transients. C1 remains farther than an ideal local bypass placement, but has a dedicated return; C2 remains on the amplifier side of both resistors. The moved parts do not worsen these loops. Shunt-side routes pass near high-current copper and locally interrupted ground reference. I²C, PWM/tach and thermistor routes have ordinary low-speed lengths; TH2's long route and back-layer hop remain. No new copper coupling or plane isolation mandates rerouting. Measure VS relative to U2.7, zero/gain versus fan state, and ADC/tach noise during load steps.

## 5. Components, footprints and assembly

Manufacturer ordering codes were reviewed by family against actual dimensions/pins, not just footprint names. No fitted INA226, TPS22810, TRACO or remote thermistor strip exists. Unused local library models do not enter the BOM.

| References / exact selection | Footprint evidence and assembly assessment |
| --- | --- |
| U2 INA228AIDGSR | 10 lands 1.45×0.30 mm, 0.50 mm pitch, 4.30 mm row spacing. TI example uses 4.40 mm spacing with same land dimensions; ±0.05 mm per row difference retains lead overlap. Fine-pitch VSSOP despite the generic TSSOP footprint name. Check pin 1, ten distinct joints and solder bridges. TI maximum body height 1.1 mm governs fit. |
| RSH1 CRF2512-FZ-R001ELF | Correct Bourns pattern and bottom side. 6.4±0.2 ×3.2±0.2 mm body; actual maximum height **1.3 mm**, versus 0.8 mm model proxy. Add joint/board tolerances for rear clearance. Two 12.40 mm² rear paste apertures match lands; backside hand soldering is feasible with heat and access. |
| U1 Seeed 102010550 | 14 edge SMD lands, 2.54 mm pitch; underside pads are fabrication graphics, not live carrier pads/paste. Model seating is approximate. Inspect actual underside components/pads and insulation from nearby vias/copper. Place/hand solder after other reflow unless its complete assembly is qualified for the chosen profile. New metadata loss: FINAL26-001. |
| Q3 DMP3056L-7 | Correct SOT-23 G/S/D mapping despite reused diode-family footprint name. Lands 0.5588×1.27 mm differ from supplier 0.8×0.9 mm example; land/joint inspection remains appropriate, not an automatic redesign. Use MOSFET identity and pin 1 rather than the footprint's name for placement. |
| Q1/Q2 2N3904BU | 1.27 mm inline E/B/C pitch, Ø0.75 holes, 1.15×1.50 lands. Lead fit/orientation checked against onsemi TO-92 drawing; inspect actual lead form. Rings/webs below. Solder after reflow. |
| J1 54-00286 | Manufacturer slot pattern and A/B/C identity agree: A 1.00×3.30 mm along board X; B/C 0.80×2.50 mm along Y. Jack body is below PCB. Confirm real tab insertion, seating, plug and panel opening. Solder after reflow. |
| J2 5442206 | 5.08 mm pitch / Ø1.30 mm drills agree with manufacturer pattern. 1×0.5 mm rectangular lead diagonal ~1.12 mm fits nominal drill; tolerances/joint fill still need first-article inspection. Screw and wire access require the installed envelope. |
| J3/J4 Molex 0022053041 CAD; intended MODDIY CO975 | Pattern 2.54 mm pitch, Ø1.0668 holes, 7.62 mm end span. Manufacturer Molex drawing supports the proxy pattern, **not equivalence to MODDIY**. Do not order Molex based solely on this BOM. Gauge intended part, key orientation, pin mapping, retention and mated wire envelope. |
| J5 S2B-XH-A-1(LF)(SN) | 2.50 mm pitch, Ø0.9906 drills, 1.4986 lands; JST's approximately 1 mm two-circuit hole pattern agrees nominally. Small move preserves spacing and improves separation from the capacitor side. Actual mated housing/wire access unmodeled. New fields lost, FINAL26-001. |
| C1–C4 Samsung CL21B104KACNFNC | Four 100 nF, 25 V, ±10%, X7R 0805. 2.0×1.25 mm body; native 0805 lands and paste are consistent. 25 V rating comfortably exceeds 3.3 V supply and nominal shunt differential; C2 does not normally see 12 V across its terminals. |
| R1/R2 RC0805FR-074K7L | Two 4.7 kΩ, 1%, 0805 pull-ups. |
| R3/R4 RC0805FR-0710RL | Two **10 Ω**, 1%, 0.125 W 0805; values/metadata are synchronized. |
| R5/R6/R10/R13 RC0805FR-0710KL | Four 10 kΩ, 1%, 0805. |
| R7/R9/R12 RC0805FR-07100KL | Three 100 kΩ, 1%, 0805. |
| R8/R11 RC0805FR-071KL | Two 1 kΩ, 1%, 0805. Maximum ordinary base-resistor loss ~6.8 mW, R7 at 12.2 V ~1.49 mW, far below 0.125 W. Standard 0805 lands 1.025×1.40 mm /1.825 mm pitch suit the manufacturer package. |
| TH1/TH2 NCU18XH103F60RB | Actual 0603 NTC, not 0805. Lands 1.00×0.97 mm, gap 0.47 mm; larger/closer than Murata's reflow guidance (gap 0.6–1.0, land length 0.6–0.7, width 0.6–0.8 mm). Existing assembly qualification item: avoid excessive paste and board stress, inspect centering, wetting and cracks. Does not establish unusable boards. |

Sources additionally read: [Samsung exact specification](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/658/CL21B104KACNFNC_Spec.pdf), [Yageo RC group](https://yageogroup.com/content/datasheet/asset/file/PYU-RC_GROUP_51_ROHS_L), [Molex drawing](https://www.molex.com/pdm_docs/sd/022053041_sd.pdf), [JST XH](https://www.jst.com/wp-content/uploads/2025/06/eXH.pdf), [Seeed product document](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8932/102010550.pdf). These are manufacturer documents, including manufacturer PDFs hosted by the distributor.

Native BOM: **16 groups / 30 fitted parts**, 30 MPNs, 29 explicit DigiKey fields (J5 blank). SMD positions: **22 top / 1 bottom**, excluding seven through-hole components. Coordinates/sides reconcile with PCB, including moved R3/R4/R7 and rear RSH1. Native positions use absolute X, negative PCB Y and KiCad rotations; they require vendor convention mapping, not blind upload.

Paste: **65 front apertures / 2 rear**. Front total includes 14 XIAO edge apertures. For the proposed workflow excluding U1, rear RSH1 and through-hole parts, order a **front stencil omitting U1's 14 apertures** (51 front apertures remain); no rear stencil is necessary if RSH1 is hand soldered. The fabrication Gerbers do not contain paste layers. Review paste volume at NTCs and Q3; the default land-sized aperture is not an approved process merely because export succeeds. Reflow small front components first, inspect U2, then install U1, rear shunt and through-hole components with appropriate support. No new modeled body/body clash or demonstrated loss of soldering access was found; populated connectors will restrict later access, so sequence matters.

## 6. Mechanical and STEP verification

Fresh populated and bare STEP exports were imported into FreeCAD. The core is a valid solid **30×80×1.51 mm**, volume **3542.624696 mm³**. Project nominal finished stack is 1.60 mm including copper/mask. Do not specify a 1.51 mm finished order.

After converting the committed centered model by X+65.5 and Y−89.0 into absolute STEP coordinates (STEP Y reverses PCB Y), **29/ 30 fitted component solids and the core have zero Boolean volume difference** within numerical tolerance. U2's stock model differs: fresh height 1.0 mm versus committed 1.1 mm, center Z shift 0.05 mm, volume difference ~0.568 mm³. Board placement/lands agree; use TI maximum 1.1 mm rather than the smaller stock model. Committed STEP additionally includes copper/artwork solids absent from the default fresh export. It is not stale mechanically.

Mount centers in PCB XY: H1 (53.20,51.70), H2 (77.80,126.30), H3 (77.80,51.70), H4 (53.20,105.00); all **2.20 mm plated** with 4 mm copper lands. They are unchanged since November 17. The older H4 X=53.00 panel reference remains a fit concern: a 0.20 mm axis shift exceeds nominal 0.10 mm radial clearance of an M2 screw. The current Onshape assembly was unavailable, so this cannot be marked resolved.

Outline: X50.5–80.5, Y49–129; four R2 corners, R3 concave relief around (50.5,112.1), two R1 blends and short tangent segments. No outline change/regression found.

| Modeled item | Current envelope / remaining physical check |
| --- | --- |
| J1 | X60–71, PCB Y72.1475–81.1475; lowest Z −14.085 mm relative to core bottom. Check barrel axis, actual plug seating and backplate opening. |
| J2 | X64.03–74.76, Y91.10–99.20; top Z+11.595 mm. Check screwdriver approach, wire exit, strain relief and housing support. |
| J5 | X51.15–58.55, Y89.80–101.30; top Z+7.695 mm. C2 Xmin59.775 gives 1.225 mm projected X separation, improved from 0.835 mm. Plug and bend are absent. |
| J3/J4 | Top Z+7.770 mm; Y extends to128.1025/128.0625 before mating. Intended MODDIY envelope is unknown; verify plug/key and cable path. |
| U1 | USB-end model reaches Y49.1891, ~0.189 mm inboard of edge; top Z+5.805 mm. Check actual USB plug, insertion access and underside gap. |
| RSH1 | Model bottom Z−0.885 mm, but real 1.3 mm maximum body plus solder governs rear clearance. Check post/panel and insulation. |
| Fasteners | Grounded 4 mm mount lands do not represent screw-head, washer or insert envelopes. Check actual screw length/engagement and all surrounding parts. |

No intercomponent solid collision was detected among the modeled fitted parts. Component/core overlaps exist: XIAO ~58.50 mm³, Molex proxies ~2.657 each, NTC proxies ~0.460 each, J5 ~0.0278 and J2 ~0.0550. These reproduce asset placement/lead artifacts, not proof that real parts collide. JST model lead registration differs from hole axes; supplier pattern and actual lead fit govern. Confirm these areas physically rather than editing footprints solely to eliminate generic-model overlaps.

## 7. ERC, DRC, parity and CAM

| Check | Fresh result |
| --- | --- |
| ERC | 0 errors, 0 warnings, 0 exclusions; exit 0. SPICE-model checks ignored by project. |
| DRC, stored | 0 board violations, 0 unconnected pads; **2 metadata parity warnings**, exit 5. |
| DRC, refilled in memory | Same result. |
| DRC, separate saved refill | Same result. Production PCB unchanged. |
| Independent XML/IPC-D-356/PCB | 30 references, 88 assignments, 26 net groups, no electrical or footprint-association mismatch. Procurement differences isolated in FINAL26-001. |
| CAM copper continuity | All 26 groups connected after plating; no different-net overlap/touch detected. GND is connected on both layers through its pads/vias. |
| Committed versus fresh CAM | All seven Gerbers have equal painted geometry (maximum symmetric difference <2×10⁻¹⁵ mm²); both drill geometry lists identical. Some text streams reorder apertures/objects, so raw text differences are not stale geometry. |
| Saved-refill versus stored CAM | All seven layers and both drills identical after removing timestamps/generator comments. |
| Layer set | F/B copper, F/B mask, F/B legend, Edge.Cuts; correct X2 file functions/polarities and common coordinates. Job manifest lists this set. |

Ignored PCB checks: missing courtyard, track endpoint not centered on via, tuning profile geometry, footprint filter and footprint component-type checks. Consequently a native pass does not certify courtyard or assembler compatibility. Electrical parity is clean; do not describe strict parity as zero-warning while fields remain missing. Committed DRC/ERC reports are historical, FINAL26-002.

| CAM measurement | Result / process implication |
| --- | --- |
| Profile | One closed 30×80 mm contour; 7 lines, 7 arcs. Maximum reconstructed endpoint gap 0.000013 mm, joined at 0.000030 mm tolerance. Shortest segment 0.353554 mm. No out-of-profile copper. |
| Minimum routed trace | 0.250 mm front; 0.300 mm rear. Front inventory 0.25/0.30/0.50/0.75/1.00; rear 0.30/0.50/1.00/3.00/4.00 mm. |
| Different-net clearance | Track/track and track/pad 0.200 mm minimum. Q1/Q2 pad/pad 0.120 mm under explicit pad-only rule. U2 pad gap 0.200 mm. |
| Copper to routed edge | Front minimum 0.550 mm; rear approximately 0.30045 mm. |
| Plated round holes | 36: 9×Ø0.30,5×Ø0.40,6×Ø0.75,2×Ø0.991,8×Ø1.067,2×Ø1.30,4×Ø2.20 mm. |
| Plated slots | Three J1 slots: 1.00×3.30 mm X and two 0.80×2.50 mm Y. Source and Excellon agree within rounding; total 39 plated features. NPTH intentionally empty. |
| Annular rings | Via minimum 0.150 mm; component minimum 0.200 mm at Q1/Q2; slot minimum~0.2495 mm; mounts 0.900 mm. |
| Hole geometry | Minimum wall-to-wall hole spacing 0.520 mm. Q1/Q2 drill wall to neighboring different-net copper 0.320 mm. Nominal Ø0.75 finished-hole published tolerance gives approximately 0.67–0.88 mm; verify actual leads and delivered plating. |
| Mask | 90 front / 27 back openings registered to pads; vias tented in artwork. Q1/Q2 bridges **0.120 mm on both sides**, U2 minimum 0.200 mm. Minimum unrelated front trace-to-opening 0.200 mm. Shunt openings 3.304×4.204 mm, gap 1.096 mm. |
| Legend | No mask/outline overlaps in plotted artwork. Minimum stroke 0.120 mm front /0.127 rear; nearest gaps 0.200 front /0.08452 rear at RSH1. J2 polarity characters 1.0 mm high /0.15 mm stroke. Thin/nonessential rear ink remains accepted cosmetic risk. |

[Current JLCPCB capability table](https://jlcpcb.com/capabilities/pcb-capabilities) supports 0.10 mm bridges for 1 oz green and 0.18 mm absolute two-layer 1 oz PTH ring, with 0.25 mm ring recommended. Q1/Q2's 0.20 mm rings and 0.12 mm bridges therefore remain borderline but inside those published limits. The [general ordering instructions](https://jlcpcb.com/help/article/instructions-for-ordering) still call for 0.20 mm usual two-layer pad spacing for bridges and an explicit bridge requirement note. **Ask order CAM to preserve/accept the 0.12 mm bridges and 0.20 mm rings.** Do not silently substitute 2 oz or black/white: those change the applicable bridge/ring requirements. Confirm all three slots are plated and route compensation preserves the relief. This audit does not constitute vendor acceptance.

## 8. Historical finding dispositions

“Bare” below means the stated calibration prototype; “assembly” includes outsourced data approval. Physical integration is distinct from fabrication.

| ID | Original issue / severity | Current disposition | Current evidence, changes and remaining action | Stage effect |
| --- | --- | --- | --- | --- |
| AUD26-001 | Rocker harness/open J5 disables fans; Major | **Accepted Risk** | Same Q3/R7 topology despite small routing moves. Open J5 still turns fans off while HDD feed stays live. Historical owner acceptance still applies. Verify harness mapping, rocker loading and USB-loss fan default. | No bare blocker; qualify before enclosure hookup. |
| AUD26-002 | Missing protection/alternate ground return; Major | **Open** | Same shared grounds and unfused/unclamped input. Solid J1 ground and connected refill do not eliminate auxiliary return fault. Map bonding and qualify actual source/harness protection at low current. | No bare blocker; gates powered enclosure. |
| AUD26-003 | 7 A jack versus six-drive demand; Major | **Accepted Risk** | Same jack/current topology. No material load-envelope change. Measure upstream startup/sustained current, contact drop and temperatures with actual plug. | No bare blocker; gates six-drive release. |
| AUD26-004 | MODDIY headers differ from CAD; Major | **Open** | J3/J4 unchanged; no toleranced MODDIY pattern supplied. Check sample/drawing, key/mapping, leads and mated envelope. | Exact-fit order assumption; intended connector assembly/powered use gated. |
| AUD26-005 | Shunt lands/pickups and accuracy; Major | **Partially Resolved** | Bourns lands/inner-edge pickups independently verified. Sense routes slightly shorter and gap increased. Shared two-terminal joints still require zero/gain/thermal/fan-state calibration. | No bare blocker; quantitative telemetry/control gated. |
| AUD26-006 | Stale STEP/physical assembly fit; Major | **Partially Resolved** | Current STEP/core/component placement agrees, apart from U2 stock model. Panel/fasteners/mated hardware unavailable. Verify H4, relief, plug and rear clearances. | No blanket bare hold; mechanical fit remains unqualified. |
| AUD26-007 | Firmware/cooling failsafe absent; Major | **Open** | Firmware still placeholder; inverted PWM can stop fans if incorrectly driven. Validate reset/watchdog/stall/sensor/telemetry fallback. | Gates autonomous six-drive operation. |
| AUD26-008 | Fan switch transients not qualified; Minor | **Open** | Q3 unchanged, no ramp/clamp added. Scope VGS/VDS/rail/current under startup, toggles and shutdown. | No bare blocker; enclosure qualification. |
| AUD26-009 | INA bypass/filter routing and return; Minor | **Partially Resolved** | Earlier improvements retained; C1/C2 routes unchanged and R3/R4 wiring slightly shorter. Noise remains unmeasured. | No bare blocker; measurement qualification. |
| AUD26-010 | Thermistor self-heat/ADC/thermal inference; Minor | **Open** | Same values/routes, actual 0603 part verified. Calibrate ADC/NTCs and correlate board readings with worst-bay HDD temperature. | Gates temperature-control qualification. |
| AUD26-011 | Assembly/BOM/stencil/documentation; Minor | **Partially Resolved** | RSH1 metadata repaired; new U1/J5 field loss, FINAL26-001. Exact header fit, paste volume, approved sides and stale README remain. | Hold turnkey assembly until approved; bare unaffected. |
| AUD26-012 | Thin silk/probe access; Minor | **Accepted Risk** | No plotted overlap; same 0.12/0.127 mm strokes and rear 0.08452 gap. Confirm critical polarity/refs survive CAM; use assembly drawing/probe wires. | Cosmetic risk, no bare blocker. |
| AUD26-013 | U2 track-width rule scope ineffective; Minor | **Open** | Same reference-based rule; free tracks 0.30/0.50 mm remain outside apparent maximum's effective scope. Physical clearances independently pass. Clarify rule later if the maximum is required. | No stage blocked. |
| AUD26-014 | Q1/Q2 annular ring/mask; Informational | **Accepted Risk** | Same 0.20 mm rings/0.12 webs/0.52 hole-wall spacing independently measured. Retain 1 oz green and get order CAM approval. | Vendor process gate; no automatic redesign. |
| FR26-001 | RSH1 PCB fields lost; Minor | **Resolved** | All five required fields now match schematic: Manufacturer Bourns, MPN CRF2512-FZ-R001ELF, DigiKey CRF2512-FZ-R001ELFCT-ND, CRF datasheet URL and description. Value/footprint/pads/side agree. | Original RSH1 assembly-data hold removed. |
| FR26-002 | Rear RSH1 legend clearance; Minor | **Accepted Risk** | Same 0.08452 mm nearest gap, zero overlap. Possible vendor clipping affects nonessential ink. | No bare blocker; inspect preview/first article. |

## 9. New findings

### FINAL26-001 — U1/J5 procurement fields removed during footprint updates

**Minor · Open · assembly data regression.** Affected file: `hardware/fan-controller.kicad_pcb`; corresponding local U1/J5 footprint files also lost properties. U1 at (65.481256,61.756756), J5 at (56.1,97.4).

| Ref | Schematic identity retained | PCB fields missing/blank |
| --- | --- | --- |
| U1 | Seeed Technology Co., Ltd /102010550 /1597-102010550-ND | Manufacturer, MPN, DigiKey_PN, Datasheet, Description |
| J5 | JST Sales America /S2B-XH-A-1(LF)(SN) | Manufacturer, MPN, Datasheet, Description; DigiKey remains blank on both sides |

KiCad reports two missing-Manufacturer parity warnings; independent comparison finds the additional fields above. No net, pad or footprint-association change accompanies this loss. **Action:** restore matching fields before generating board-derived procurement/assembly data, or use and explicitly reconcile the intact schematic BOM for a selective assembly release. This does not require a copper redesign or prevent ordering bare boards. No design file was repaired in this audit.

### FINAL26-002 — Committed DRC report no longer represents current parity

**Minor · Open · verification documentation.** `hardware/DRC.rpt`, dated November17, reports zero footprint errors; current stored/refilled checks report the two metadata warnings above. `hardware/ERC.rpt` is dated November14; its zero-error result was independently reproduced today. Manufacturing Gerbers/drills and STEP are current; this finding concerns verification records only.

**Action:** use this report's fresh results for the order review. When a design-maintenance session resolves FINAL26-001, regenerate verification records with their actual checked SHA. No fabrication or operation gate is added by an old report when current evidence is available. Historical audit reports remain untouched.

## 10. Prioritized remaining actions

### Before bare-board payment

- [ ] Use the audited commit's seven Gerbers and separate PTH/NPTH drills; select 2 layers, 1.6 mm, 1 oz, green. Approve 30 × 80 contour, relief, all three plated J1 slots and four plated mounting holes.
- [ ] Obtain order-specific CAM acceptance for Q1/Q2's 0.20 mm rings and separate 0.12 mm mask bridges; explicitly request the bridges. Review critical J2 polarity legend. No artwork edit is needed unless CAM rejects the process.
- [ ] If rework-free installed fit is required, compare actual MODDIY leads/key/polarity and the current panel/mount/plug envelopes to this pattern before paying. Otherwise record these as first-prototype fit risks, not verified fit.

### Before an assembled-board order

- [ ] Close FINAL26-001 or approve the reconciled schematic BOM as the authoritative procurement source. Do not source Molex placeholders as the intended fan headers.
- [ ] Approve exact fitted refs, vendor substitutions, U2/Q3 orientation, top/bottom coordinate transforms and stencil. Exclude U1 apertures for the proposed selective front reflow; inspect NTC/Q3 paste strategy. Decide who installs U1 and bottom RSH1.

### Once the prototype arrives, before enclosure hookup

- [ ] Inspect outline/holes/slots/plating, Q1/Q2 rings/dams, U2 bridges/joints, shunt wetting and actual connector/module seating. Check fastener and rear clearances. Verify polarity, NC isolation, main return and all supply-to-ground resistances.
- [ ] Use isolated/current-limited supplies and dummy loads first. Exercise USB-only,12 V-only, both, neither, both power orders, rocker ON/OFF/open. Verify fan default, GPIO/USB back-power, rocker loading, VGS/VDS and supply transients. Map auxiliary grounds and fault clearing before connecting HDDs.
- [ ] Test both actual fans' PWM inversion/frequency, tach and restart; calibrate INA228 at zero/multiple known currents, warm/cold and fixed current with fan switching. Calibrate NTC/ADC response and check noise/fault behavior.

### Before full or unattended six-drive service

- [ ] Implement/test safe firmware startup, watchdog, lost USB, stale telemetry, bad NTC, failed I²C, missing tach and stalled fan behavior.
- [ ] Capture upstream J1 cold-start peaks/sustained current, J2 droop, jack/plug/contact/wire temperatures and worst-bay HDD temperatures in the installed enclosure. Establish actual limits against the 7 A jack/plug and drive-voltage requirements. Repeat startup and recovery; a successful single spin-up is insufficient qualification.

## 11. Reproduction commands and evidence interpretation

Commands below were executed with the pinned checkout; `$cli` denotes the isolated KiCad 10 wrapper, `$out` a scratch evidence directory and `$src` the detached worktree. Export-only operations read production files; saved refill occurs exclusively in an archive copy.

```bash
git clone https://github.com/full-of-beans/cenmate-fan-controller.git repo
git -C repo fetch origin initial-schematic
git -C repo rev-parse origin/initial-schematic
git -C repo worktree add --detach "$src" 8cc880a96dea908ae958eecb40e3e9b91f2e8c4f
git -C "$src" diff --stat 482af6a78fe4e53589d44535311f52b8d44c7214 HEAD
p="$src/hardware/fan-controller"
"$cli" version
"$cli" sch erc --severity-all --exit-code-violations -o "$out/erc.rpt" "$p.kicad_sch"
"$cli" pcb drc --severity-all --all-track-errors --schematic-parity --exit-code-violations -o "$out/stored.rpt" "$p.kicad_pcb"
"$cli" pcb drc --severity-all --all-track-errors --schematic-parity --refill-zones --exit-code-violations -o "$out/refill.rpt" "$p.kicad_pcb"
"$cli" sch export netlist --format kicadxml -o "$out/net.xml" "$p.kicad_sch"
"$cli" pcb export ipcd356 -o "$out/board.d356" "$p.kicad_pcb"
python "$src/scripts/check-schematic-pcb-parity.py" "$out/net.xml" "$out/board.d356" "$p.kicad_pcb"
KICAD_CLI="$cli" bash "$src/scripts/export-pcb-manufacturing.sh" "$out/cam"
python "$src/scripts/inspect-pcb-manufacturing.py" "$out/cam" "$p.kicad_pcb"
"$cli" pcb export gerbers --board-plot-params -o "$out/gui/" "$p.kicad_pcb"
"$cli" pcb export gerbers -l F.Paste,B.Paste -o "$out/paste/" "$p.kicad_pcb"
"$cli" pcb export pos --format csv --units mm --smd-only --side both -o "$out/positions.csv" "$p.kicad_pcb"
"$cli" sch export bom --fields 'Reference,Value,Footprint,QUANTITY,Manufacturer,MPN,DigiKey_PN,Datasheet,Description' --labels 'Reference,Value,Footprint,Quantity,Manufacturer,MPN,DigiKey_PN,Datasheet,Description' --group-by 'Value,Footprint,MPN' --exclude-dnp -o "$out/bom.csv" "$p.kicad_sch"
"$cli" pcb export step --force --no-dnp -o "$out/fresh.step" "$p.kicad_pcb"
"$cli" pcb export step --force --board-only -o "$out/bare.step" "$p.kicad_pcb"
# Extract an independent archive copy, then run on its PCB:
"$cli" pcb drc --severity-all --all-track-errors --schematic-parity --refill-zones --save-board --exit-code-violations -o "$out/saved-refill.rpt" "$copy/hardware/fan-controller.kicad_pcb"
# Repeat CAM export on that copy and compare against stored-board exports.
```

The strict DRC exit 5 is expected from FINAL26-001, not a hidden copper violation. Independent parity script likewise lists metadata differences rather than an electrical failure. Gerbonara warns about G90 after the Excellon header; absolute hole/slot centers, tools and lengths were explicitly matched to source. Layer comparison used painted primitive unions, not only filenames or dates: seven symmetric differences are zero to numerical tolerance, and Excellon primitive inventories match. Refilled output comparison strips only generator/date comments. Route lengths were recomputed from transformed footprint pads and track graphs; copper-loss calculations use actual wide-route segment lengths. FreeCAD compared transformed component/core solids, separated from additional committed copper/artwork objects. All original design, fabrication and firmware files remain unmodified.

## 12. Final fabrication recommendation

**Order the current bare PCB as a prototype after the CAM/order checks above; no additional design iteration is required by this audit.** Close assembly metadata and release the selective fit list before outsourcing assembly. Fit, solder inspection, calibration and controlled bench testing are valid prototype tasks. Enclosure connection and six-drive operation remain separate gated steps because actual harness behavior, protection, startup current, contact heating, cooling and firmware have not yet been demonstrated.
