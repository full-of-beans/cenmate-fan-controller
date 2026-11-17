# CENMATE fan controller fabrication readiness follow-up

Reviewed November 17, 2026, America/New_York.

| Baseline | Immutable commit |
| --- | --- |
| AUDITED_COMMIT | [`3294cb89a1b5f34098adf01730dca6feb2a4a3dd`](https://github.com/full-of-beans/cenmate-fan-controller/commit/3294cb89a1b5f34098adf01730dca6feb2a4a3dd) |
| REVIEWED_COMMIT | [`482af6a78fe4e53589d44535311f52b8d44c7214`](https://github.com/full-of-beans/cenmate-fan-controller/commit/482af6a78fe4e53589d44535311f52b8d44c7214) |
| Branch | `initial-schematic` |
| Historical findings | [November 15 audit](full-design-audit-2026-11-15.md) |
| Fabrication basis | Two-layer rigid FR-4, 1.6 mm nominal, 1 oz copper, green solder mask. Order finish remains a fabrication and assembly selection. |

## Release decision

**NOT READY TO ORDER a fit-required prototype until the pre-order fit and JLCPCB preview checks below are complete. The bare-board artwork is current and receives a CONDITIONAL PASS. No mandatory PCB design change was identified for the reviewed 1 oz green process.**

| Stage | Verdict | Release boundary |
| --- | --- | --- |
| Electrical design and PCB routing | CONDITIONAL PASS | Ground and positive paths are connected, sense layout improved, and nominal copper geometry passes. Full-load and partial-power behavior require qualification. |
| Bare PCB manufacturing | CONDITIONAL PASS | Submit the verified CAM for vendor review. Confirm plating, profile, and Q1/Q2 rings and separate mask openings. Close fit checks before paying for boards intended to fit the chosen hardware and panel. |
| Mechanical fit | NOT VERIFIED | Current PCB and STEP are aligned. Actual MODDIY parts, inserted fasteners, mated cables, and the installed Onshape assembly are unverified. |
| Component assembly | HOLD | Verify real headers and fit list, assembly sides, stencil and XIAO seating. Resolve the RSH1 PCB metadata discrepancy in production assembly data. |
| Initial powered bench testing | CONDITIONAL PASS | After unpowered inspection and polarity checks, start with an isolated current-limited supply and dummy loads. Enclosure hookup is held pending harness and fault-path qualification. |
| Full six-drive operation | HOLD | Demonstrate electrical, thermal, firmware fallback, and cold spin-up margins with actual loads. |

### Before ordering bare PCBs

1. **Confirm fit with parts or a 1:1 pattern.** Check J3/J4 using an actual CO975 sample or a dimensioned drawing, J1 slot tabs, J5 lead pitch/fit, and the four 2.20 mm mounting holes and relief against the fixed panel. Use the latest STEP with the installed Onshape panel to check H4, jack alignment, connector and tool access, and the mated plug and wire bend. If a footprint or panel check fails, correct the affected geometry and reverify any changed fabrication files.
2. **Upload the reviewed files and approve the JLCPCB preview before paying.** Use the seven Gerbers and the separate PTH and NPTH drill streams in [hardware/gerbers](../../hardware/gerbers). The empty NPTH file is intentional. Use 1 oz green and 1.6 mm. Confirm the 30 × 80 mm routed contour and post relief, all three J1 slots as plated, four plated mounting holes, and the 0.20 mm nominal Q1/Q2 component rings. Ask CAM to confirm the separate 0.12 mm Q1/Q2 mask dams and specify that the bridges are required. Check J2 polarity markings and the nonessential rear shunt legend. JLCPCB's capability table lists 0.10 mm for 1 oz green and its general order guide calls for 0.20 mm on two-layer boards in usual cases. Resolve that order-specific discrepancy with CAM. Changing to 2 oz or black/white needs a fresh process check. [JLCPCB capabilities](https://jlcpcb.com/capabilities/pcb-capabilities) · [Ordering guide](https://jlcpcb.com/help/article/instructions-for-ordering)

Recommended before an assembled order are resynchronizing the RSH1 metadata, approving the exact BOM/placement/stencil and sides, and confirming the actual XIAO, MODDIY and JST fit. A later revision can improve nonessential rear silk, the U2 width rule and further shorten the bypass loop if measurements warrant it. None of those is a required bare-board layout edit under the stated process.

## Commit comparison

The original baseline is an ancestor. The fetched tip matched the previously observed tip. Review and exports used a detached worktree at REVIEWED_COMMIT. A separate archive copy was used for a saved zone refill.

| Commit | Relevant change |
| --- | --- |
| `60d5e9d` | Added the historical report. |
| `b5aa95f` | Updated the RSH1 library footprint for the Bourns lands, courtyard and legend. |
| `482af6a` | Adopted the RSH1 footprint on B.Cu, rerouted both shunt/load legs and sense pickups, moved C1/C2/R3, refreshed R4 artwork/orientation metadata, moved three vias, and refilled ground. Changed plot settings including silk mask subtraction and output folder. Added seven Gerbers, two drills and a job manifest. Regenerated STEP and updated the DRC report date. |

Endpoint comparison found five changed placed footprints (C1, C2, R3, R4, RSH1), 236 to 247 track segments and 14 vias at both endpoints. The changed tracks are limited to 3.3 V, GND, 12V_IN, 12V_HDD and the two filtered INA228 nets. The moved vias serve those two 12 V nets and GND. R4's center and pad centers stayed fixed. The only project-file change is the last-used plot folder. The schematic, symbols, custom rules, remaining footprint and component model files, outline, mounting centers, docs for the circuit, and firmware and mechanical placeholders are byte-identical at both endpoints. Only the original audit document was added outside `hardware/`.

## Original findings

“Bare PCB” refers to bare fabrication at the accepted stack. “Powered” distinguishes isolated bench work from operation with the enclosure.

| ID | Original severity and status | Current status | Verified change or continuing evidence | Remaining action and release effect |
| --- | --- | --- | --- | --- |
| AUD26-001 | Major · Requires Physical Validation | **Accepted Risk** | J5.1 still grounds Q3 gate through the enclosure rocker, J5.2 is GND, and R7 pulls gate to source if that branch opens. The HDD path remains live when J5 is disconnected or its joint fails. The owner accepts this prototype case. The owner judges normal failure of the original rocker unlikely to produce it because the enclosure's own switching arrangement would also disable HDD operation. The latter is an integration assumption. | Verify keyed wiring polarity, rocker loading, USB-loss fan default, alternate returns and sequencing before enclosure hookup. No PCB redesign solely for J5 disconnection. Bare PCB and unpowered assembly can proceed. |
| AUD26-002 | Major · Requires Physical Validation | **Unchanged** | J1/J2, chassis mounts, USB and J5 still share ground. The carrier has no fuse, reverse-polarity protection or surge clamp. J2 return failure can divert current through unintended grounds. Plane refill changes local geometry without changing topology. | Verify bonding and fault clearing with current-limited dummy loads and the actual brick/harness. Qualify the load return before enclosure hookup. Bare PCB and unpowered assembly can proceed. |
| AUD26-003 | Major · Accepted Risk | **Accepted Risk** | J1 is still the 7 A Tensility 54-00286, upstream of both drive and fan branches. A 9 A adapter does not enforce the jack rating. | Measure cold-start peaks, sustained current, mated voltage drop, plug/jack temperature and load voltage. Establish usable margin against plug/jack ratings and derating. Gates full six-drive operation. |
| AUD26-004 | Major · Requires Physical Validation | **Unchanged** | CAD remains Molex 0022053041. MODDIY CO975 confirms a four-pin 3+1 right-angle 2.54 mm part. Its listing provides no toleranced lead or envelope drawing. | Gauge sample against J3/J4; check key, retention, pin mapping and plug clearance. Gates fit-required order, intended header assembly and powered fan hookup. |
| AUD26-005 | Major · Open | **Partially Resolved** | Board and library now match Bourns 3.10 × 4.00 mm, 1.30 mm gap. Sense branches leave facing inner edges and current arrives from opposite outward sides. Two shared solder joints and terminations remain. | Inspect backside joints and calibrate zero, gain, drift and fan coupling before relying on current or power. Bare fabrication and calibration-prototype assembly can proceed. |
| AUD26-006 | Major · Open | **Partially Resolved** | The stale STEP was replaced. All 30 fitted refs, including J5 and Q3, and all four drilled mounts are represented. C2 moved out of the JST body's projected X span. Several component models still have known registration/geometry artifacts. | Check current panel/insert alignment, relief and mated J1–J5/USB envelopes. Check underside module and leads on first article. Gates fit-required ordering and fit-qualified assembly. |
| AUD26-007 | Major · Open | **Unchanged** | `firmware/` remains a placeholder. Reset releases both open-collector PWM outputs, but a high GPIO or wrong inversion can command fan stop. Q3 has independent rocker control. | Verify inverted PWM/duty, reset watchdog, conservative defaults, valid tach and stale or faulty telemetry handling. Holds autonomous six-drive use. |
| AUD26-008 | Minor · Requires Physical Validation | **Unchanged** | Q3 gate, power branch, rocker and absence of a local power clamp or controlled ramp are unchanged. | Scope VGS, VDS, switched supply and current at toggles, startup, shutdown and representative adverse load. Gates powered enclosure qualification; isolated testing can proceed. |
| AUD26-009 | Minor · Open | **Partially Resolved** | C1 VS 16.53 → 6.93 mm, plus a 5.38 mm direct GND leg now links C1 and U2. Both C2 post-filter routes dropped to about half or less. B.Cu supplies a connected but locally interrupted reference. | Measure VS relative to U2 GND and monitor sense, tach and ADC under load steps. Gates noise and measurement qualification. |
| AUD26-010 | Minor · Requires Physical Validation | **Unchanged** | TH1/TH2 retain 10 kΩ continuous excitation and 100 nF. Geometry and ADC routes unchanged. Nominal self-heat screening estimate is ~0.27 °C at 25 °C. | Verify ratiometric calculation, ADC settling, fault rails, thermal calibration and worst-bay temperature. Gates thermal control qualification. |
| AUD26-011 | Minor · Open | **Regressed** | Assembly fit/stencil issues remain. Board-side RSH1 manufacturer, MPN and DigiKey fields were removed and datasheet/description cleared. Schematic BOM retains exact part. See FR26-001. | Use schematic BOM, reconcile PCB assembly fields, approve connector fit, vendor list, stencil/sides and orientations. Holds fit-qualified assembled order. |
| AUD26-012 | Minor · Accepted Risk | **Accepted Risk** | No artwork overlaps mask or outline. Front/back strokes still reach 0.120/0.127 mm. Relocated/enlarged shunt now leaves only 0.0845 mm from a rear legend stroke to mask, previously 0.1545 mm. See FR26-002. | Confirm critical legends in preview and on first article. Use a current assembly drawing for pin 1 and add temporary probe access as needed. A clipped nonessential shunt stroke is a cosmetic concern. |
| AUD26-013 | Minor · Open | **Unchanged** | The rule still selects by U2 footprint reference. Free-track 0.30 and 0.50 mm escapes coexist with the rule's apparent 0.20–0.25 mm width range. Actual 0.20 mm U2 clearance confirmed. | If the width maximum is required later, target a rule area or matching tracks and test deliberate violations. No stage blocked by this clause. |
| AUD26-014 | Informational · Accepted Risk | **Accepted Risk** | Q1/Q2 nominal ring 0.200 mm, adjacent copper and mask web 0.120 mm, hole-wall spacing 0.520 mm. Geometry unchanged. | Retain footprint for 1 oz green subject to vendor approval and first-article fit/bridge inspection. Verify transistor E/B/C. |

No original issue satisfies all criteria for a fully Resolved disposition. The shunt land mismatch, stale mechanical export and long bypass routes were addressed as sub-items.

### New findings

| ID | Finding | Action |
| --- | --- | --- |
| FR26-001 | **Minor · Assembly data regression.** RSH1's pad nets, assigned footprint, value and placement identity agree. PCB procurement fields were lost during footprint update. Native parity reports the missing Manufacturer field. Independent parity catches MPN and DigiKey as well, and a field compare finds blank Datasheet and Description. | Source RSH1 from the schematic BOM as CRF2512-FZ-R001ELF and restore the five PCB fields before using board-derived assembly data. Copper and drill release are unaffected. |
| FR26-002 | **Minor · Rear legend clearance.** The nearest rear outline stroke for RSH1 is 0.0845 mm from the enlarged mask opening. JLCPCB publishes a 0.15 mm legend-to-pad target. Local artwork is clear of the opening. | Check JLCPCB clipping. Existing minor-silk acceptance applies to cosmetic loss. If vendor processing would obscure a critical identifier, revise that identifier before release. |

## Electrical measurements

### AUD26-005 shunt and current path

- RSH1 center is (68.10, 87.90), rotated 90° on B.Cu. Pad 2 on 12V_IN is centered at (68.10, 85.70) and pad 1 on 12V_HDD at (68.10, 90.10). The new 4.40 mm pitch gives a 1.30 mm inner gap and 7.50 mm outside span, versus 4.62 mm pitch, 1.92 mm gap and 7.32 mm span originally. Each land is 3.10 mm axial × 4.00 mm transverse, up from 2.70 × 3.42 mm. These match the Bourns recommended 1–3 mΩ pattern.
- Bourns's 1 mΩ envelope is 6.40 ±0.20 × 3.20 ±0.20 × 1.10 ±0.20 mm, with 2.20 ±0.30 mm terminal length. The local STEP is a 2 mΩ series proxy measuring 6.40 × 3.20 × 0.80 mm, 0.30 mm below nominal thickness and 0.10 mm below the 1 mΩ drawing's minimum. Copper extends 0.55 mm past each nominal body end and 0.40 mm past each side. Even a centered maximum-size body leaves about 0.45/0.30 mm end/side copper. The nominal pad area and length are consistent with the recommended land, while actual backside clearance should use the 1.30 mm maximum thickness plus mounting and solder tolerances. [Bourns CRF datasheet, p. 3](https://www.bourns.com/docs/Product-Datasheets/CRF.pdf)
- Current enters pad 2 from Y < 85.70 and leaves pad 1 toward Y > 90.10. The 0.30 mm sense branches extend inward on the resistor centerline, exit the facing edges at Y = 87.25 and 88.55, then turn at (68.10, 87.70) and (68.10, 88.20). They match Bourns's inner-edge takeoff principle, with a lateral turn between the pads. The actual copper inside each land remains continuous from outer feed to inner pickup, so solder and termination spreading resistance, joint asymmetry and thermal EMF can still affect the reading. The parallel traces approach to 0.20 mm edge-to-edge. The 0.30 mm sense branches carry monitor current only. Neither is a series neck in the HDD feed.
- Actual B.Cu copper is 4.00 mm along 5.52/5.77 mm of the upstream/downstream centerlines and 3.00 mm for 0.70 mm per side inside the rectangular lands. The nominal 3 mm transitions are within the 4 mm-wide pad copper. The old downstream alignment stub was removed. There is no series via. CAM shows no opens, overlaps or out-of-profile copper. Using 35 µm copper and 20 °C bulk resistivity, summing trace centerlines as 4/3 mm gives ~1.62 mΩ, slightly conservative at the wide lands versus the baseline ~1.53 mΩ. Copper-only losses are ~11.3 mV and 79 mW at 7 A, or ~14.6 mV and 131 mW at 9 A. Adding the nominal 1 mΩ shunt gives ~18.3 mV/128 mW and ~23.6 mV/212 mW. These estimates exclude solder, connector, return and wire losses and do not qualify 9 A operation, temperature rise, or measurement accuracy.
- Two B.Paste flashes match the rectangular copper exactly (12.40 mm² per land). Mask expands +0.102 mm per side to 3.304 × 4.204 mm, with 1.096 mm mask separation across the gap. Inspect wetting, voids, centering and joints. Compare against a calibrated four-wire current reference at zero and several loads, cold and hot, with fan switching at fixed downstream current.

### AUD26-009 INA228 routing

Centerline lengths include pad centers and exclude plated depth. The routes from the shunt centers include the within-pad takeoff length.

| Leg | Nov 15, mm | Reviewed, mm | Reviewed geometry |
| --- | ---: | ---: | --- |
| C1.1 → U2.6 supply | 16.53 | 6.93 | F.Cu 0.30 mm. |
| C1.2 → U2.7 ground | 8.26 center spacing; plane-mediated | 5.38 routed, 4.43 center spacing | Direct F.Cu 0.30 mm. The two direct legs total 12.31 mm excluding pad spans. |
| C2.1 → U2.10 IN+ | 10.39 | 5.23 | F.Cu 0.30 mm. |
| C2.2 → U2.9 IN− | 11.00 | 3.89 | F.Cu 0.30 mm. |
| R3.2 → C2.1 | 4.26 | 4.50 | F.Cu 0.50 mm with 0.65 mm of 0.25 mm near C2. |
| R4.2 → C2.2 | 4.23 | 5.04 | F.Cu 0.50 mm. |
| RSH1.2 → R3.1 | Center pickup | 15.01 | 0.30/0.50 mm, via at (63.70, 87.00). |
| RSH1.1 → R4.1 | 12.93, center pickup | 13.70 | 0.30/0.50 mm, via at (61.85, 85.00). |

C1 moved (72.00, 83.10) → (66.50, 88.50), C2 (54.60, 84.95) → (60.40, 87.70), and R3 (59.2125, 87.25) → (61.80, 93.10). R4 remains at (59.1625, 82.60). Netlist verifies 100 nF VS bypass and symmetrical 10 Ω series resistors with 100 nF differential capacitance on the INA228 side, ideal differential corner ~79.6 kHz. Both filtered legs are shorter, and the routed local ground makes the bypass loop materially better. The input pair separates around the filter and is ~1.34 mm asymmetric between C2 and U2. Projected centerline over B.Cu ground is 2.02/6.93 mm for VS, 3.14/5.38 for its GND leg, and 3.36/5.23 and 1.91/3.89 mm for the filtered pair. This geometric measure identifies plane gaps but does not measure return impedance. The shunt-side sense traces run beside the HDD current path and their ground reference detours around supply copper. Q3 switches on the upstream side and shares input and GND impedance. FAN2 tach's 6.47 mm B.Cu hop and TH2 ADC's 7.49 mm hop are unchanged. No measured interference is available. Probe VS to U2.7 and verify readings, tach edges and ADC at coincident drive/fan load transitions. [TI INA228, §§8–10](https://www.ti.com/lit/ds/symlink/ina228.pdf)

### Other electrical continuity and margin

| Circuit | Reviewed evidence and operating limit |
| --- | --- |
| Main distribution and return | J1.A → 12V_IN → RSH1.2/.1 → 12V_HDD → J2.1. J1.C and J2.2 are GND; J1.B is the unused switched contact. U2.8 measures downstream bus voltage. One B.Cu GND region spans ~2014.23 mm² and connects the main return without a series via. At Y = 80.15 the outer corridors are ~13.20/4.90 mm and at Y = 87.90 ~11.44/11.48 mm. These slices support a broad return and do not include contact resistance, partial return failures, or a thermal model. The 7 A input and actual plug, harness and thermal limits govern full-load qualification. J2 is Phoenix 5442206, with installation-dependent limits and conductor/torque requirements. [Tensility](https://tensility.s3.us-west-2.amazonaws.com/imports/product_spec_sheets/54-00286.pdf) · [Phoenix](https://www.phoenixcontact.com/en-us/products/printed-circuit-board-terminal-bc-508x10-2-gn-5442206) |
| Q3 fan switch | Pin 2 source at 12V_IN, pin 3 drain at 12V_SWITCH, pin 1 gate through J5 with 100 kΩ source pull-up. Direct gate route ~20.27 mm including 18.17 mm on B.Cu, with additional R7 branching. At a reported 12.2 V rail and grounded gate, VGS ≈ −12.2 V, and added rocker load is ~122 µA. DMP3056L-7 has 30 V magnitude VDS and ±25 V VGS absolute limits. At 0.38 A nominal for two 0.19 A P8 Max, 50 mΩ gives ~19 mV and 7.2 mW at the datasheet condition. Actual turn-on stress and fault energy remain unknown. Q3's body diode can feed input from a separately driven switched output. [Diodes](https://www.diodes.com/datasheet/download/DMP3056L.pdf) · [ARCTIC](https://www.arctic.de/en/P8-Max/ACFAN00286A/) |
| Partial power and fans | XIAO is USB powered; 3.3 V feeds INA228 and the signal networks, and no second VBUS feed is routed. TI permits the 12 V input common-mode with VS off and ground intact. Q1/Q2 are grounded-emitter open-collector PWM sinks with 1 kΩ base and 100 kΩ bias; 3.3 V high yields ~2.6 mA base current and pulls PWM low. Tach wires feed XIAO with 10 kΩ to 3.3 V (~0.33 mA pull-up current). Verify actual tach output topology, reset/open-PWM behavior, input voltages under partial power, and GND/USB return currents with the intended harness. |
| Thermistors | 10 kΩ top/10 kΩ nominal NTC bottom with 100 nF at each ADC. ~1.65 V, 165 µA, 0.272 mW and 0.5 ms nominal node constant at 25 °C. Verify ADC reference/excitation and the board-to-drive bias under airflow. |

## Mechanical verification

Imported the committed centered model and independent populated/bare STEP into Open Cascade. Each core is a valid solid, 30 × 80 × 1.51 mm and 3542.6247 mm³. 1.51 mm is the dielectric core; the PCB stack totals 1.60 mm with copper and solder mask. The committed file has 30 fitted references, seven extra artwork objects, and the core. Both populated exports include all current part refs. H1–H4 are board holes without separate screw models. This run counted 30 fitted parts and 22 front SMD positions, plus one rear position. The historical report printed 29 and 21 respectively. The unchanged schematic and population show this is a count discrepancy in the historical report, with no added part in this revision.

Mount-hole centers measured from STEP and matched PCB/Excellon in PCB XY (mm): H1 (53.20, 51.70), H2 (77.80, 126.30), H3 (77.80, 51.70), H4 (53.20, 105.00). All are plated 2.20 mm. H4's 0.20 mm shift relative to the older panel check exceeds the nominal 0.10 mm radial clearance on an M2 screw. Check insert axis and actual tolerance against the current panel.

| Item | Committed STEP measurements and implications |
| --- | --- |
| J1 barrel | X 60.00–71.00, Y 72.1475–81.1475, Z min −14.085 mm relative to core bottom. Orientation places the jack body below the core. Check pass-through and actual plug seating. |
| J2 terminal | X 64.03–74.76, Y 91.10–99.20, top Z +11.595 mm. Check wire exit, tool access and clearance to nearby parts. Small ~0.055 mm³ modeled core intersection warrants lead/finished-hole first-article inspection. |
| J3/J4 | Right-angle Molex proxies reach Z +7.770 mm and their envelopes extend toward the Y = 129 board end. Y reaches 128.1025/128.0625 before mating. Drills start at (53.6919, 115.5594) and (65.9319, 115.5194) with 2.54 mm pitch, 7.62 mm end span, 1.067 mm nominal holes, 0.2535 mm nominal ring. MODDIY CO975 listing confirms pitch and angle only. Compare actual lead diagonals, bends and coplanarity against finished holes (about 0.987–1.197 mm using published tolerance), identify 1 GND, 2 switched 12 V, 3 tach, 4 PWM, and check key engagement and mated wire clearance. ~2.657 mm³ core intersection per Molex proxy describes the CAD assets and cannot establish MODDIY interference. [Molex drawing](https://www.molex.com/pdm_docs/sd/022053041_sd.pdf) · [MODDIY CO975](https://www.moddiy.com/products/5408/4-Pin-3-Pin-2.54mm-Pitch-2510-Fan-Male-Angled-Connector-Black.html) |
| J5 and C2 | J5 reaches Z +7.695 mm across X 51.5401–58.9401 and Y 89.8085–101.3085. C2 now has X min 59.775 mm, 0.835 mm beyond the body envelope. The shift improves clearance on the mating side. The mated housing and cable are absent from the model. Mid-core model lead centers are +0.120/−0.120 mm from their drilled centers. The ~0.028 mm³ overlap is a model-registration artifact. The pads use 2.50 mm pitch and 0.9906 mm nominal holes. Use JST's board pattern and verify finished holes and joint fill. [JST XH drawing](https://www.jst.com/wp-content/uploads/2025/06/eXH.pdf) |
| U1, U2 and proxies | The XIAO USB end stops ~0.189 mm inboard of Y = 49.00. U1 overlaps the core by ~58.50 mm³ in the model and each NTC proxy by ~0.460 mm³. These modeled overlaps do not establish physical clashes. Inspect the actual underside gap, exposed/unused XIAO pads and via coverage. The two exporters use slightly different stock U2 body models (top Z 2.695 vs 2.595 mm). Use TI's actual maximum for vertical fit. Q3 uses an SOT-23 proxy. RSH1 body height caveat is described above. |

## Fabrication verification

| Check | Independent result |
| --- | --- |
| KiCad ERC | 10.0.8, 0 errors and 0 warnings. Project ignores SPICE model issues. |
| KiCad DRC and parity | As stored, refilled in memory and refilled/saved in a scratch copy each give 0 board DRC violations and 0 unconnected pads. Each reports one local-override parity warning for missing RSH1 Manufacturer. This is why the strict command returns a violation exit status. Design's ignored checks are missing courtyard, via endpoint centering, tuning geometry, footprint filter, and footprint component type. |
| Independent parity | XML vs PCB and IPC-D-356 verifies 30 schematic refs, 88 pin/pad assignments, 26 net groups and four grounded board-only mounts with no connectivity or footprint association mismatch. RSH1 fields differ as documented in FR26-001. Explicit no-connects J1.B, U1.3/.4/.7 and U2.3 are isolated. U1.14 is a single-pad VBUS net without carrier load. |
| Exact CAM reconciliation | Committed X2 from KiCad 10.0.7 equals fresh 10.0.8 output using stored board plot settings, for each of seven .gbr files and both Excellon files. Normalized job manifests agree. Only line endings, generator/timestamp metadata, and JSON serialization order were normalized. Fresh controlled export after a saved zone refill also agrees with the as-stored controlled export in all seven layers, both drills and the job manifest. No committed output is stale. |
| Parsed copper | All 26 nets form one group each after plating; no different-net touches or overlaps in front/back artwork. Front widths 0.25, 0.30, 0.50, 0.75, 1.00 mm; back 0.30, 0.50, 1.00, 3.00, 4.00 mm. Minimum track-to-track and track-to-pad 0.20 mm, pad-to-pad 0.12 mm at Q1/Q2. Smallest edge clearances 0.550 mm front and 0.30045 mm back. |
| Outline and drills | One 30 × 80 mm contour, seven lines, seven circular arcs, max joined endpoint gap 0.000013 mm. Four R2 corners, R3 relief blended by R1 arcs. 36 plated round hits (9 × 0.30, 5 × 0.40, 6 × 0.75, 2 × 0.991, 8 × 1.067, 2 × 1.30 and 4 × 2.20 mm). Three routed PTH slots: J1.A 1.00 × 3.30 mm along X; J1.B/C 0.80 × 2.50 mm along Y, agreeing with board within Excellon rounding (2.501 mm parsed end to end). All 39 centers, tools and lengths match source. NPTH empty. All slot dimensions meet published 0.50 mm and 2:1 criteria. Gerbonara warns about placement of G90 after the header; verified absolute geometry against source. |
| Plating rings and mask | Minimum via ring 0.150 mm, component ring 0.200 mm, slot ring ~0.2495 mm. Mount rings 0.90 mm. Hole wall spacing 0.520 mm minimum. 90 front and 27 back mask openings register to component pads; no vias have mask apertures. Q1/Q2 are 0.120 mm web on both sides and U2's minimum is 0.200 mm. Minimum unrelated trace to mask is 0.200 mm on front. |
| Legend and assembly | Zero geometric silk/mask and silk/edge overlap. Minimum gap 0.200 mm front and 0.0845 mm rear, minimum strokes 0.120/0.127 mm. J2's GND/12V characters are 1.0 mm high with 0.15 mm stroke. Native BOM has 16 groups, 30 MPNs and 29 explicit DigiKey entries (J5 field blank). Positions reconcile to 22 front SMD placements and one rear. Paste has 65 front and two rear apertures. Verify actual footprint substitutions, stencil and fit list separately. |

For outsourced assembly approve explicit sides and placements for the XIAO and backside shunt, Q3 single drain-side pad orientation, U2 pin 1, thermistor and SOT-23 land wetting and J5 seating. Review Q3's nominal footprint dimensions (0.559 × 1.27 mm versus supplier's 0.80 × 0.90 mm example) and NTC lands (1.00 × 0.97 mm, 0.47 mm gap versus Murata's 0.60–0.70 × 0.60–0.80 mm, 0.60–1.20 mm reflow gap) during first-article joint inspection. The neutral paste files are scratch evidence, and the fabrication package has no paste layers. [Diodes land pattern](https://www.diodes.com/datasheet/download/DMP3056L.pdf) · [Murata guide](https://www.mouser.com/pdfDocs/ncu18.pdf)

## Post-fabrication validation

1. Inspect rings, slots, mask dams and critical ink. Gauge connector and screw fit, inspect XIAO underside, terminations and through-hole joints, and continuity-check polarity and isolation before applying power.
2. Start with current-limited supplies and dummy loads, without drives. Exercise 12 V only, USB only, both and neither, both power orders, and rocker ON/OFF/open. Verify wiring, unintended connector/USB/chassis returns using only low-current fault simulation initially, fan default with USB down, VGS/VDS and rail behavior, and effective source/harness protection. Enclosure hookup waits for these checks.
3. Test PWM inversion, frequency, tach and restart on both actual fans. Calibrate INA228 and thermistor readings and measure supply/ADC/tach noise with fan and drive loads stepped. Qualify firmware startup, watchdog, stale telemetry, bad sensors and stalled fan response.
4. Capture cold spin-up of six actual drives and both fans, upstream input current, J2 voltage and steady jack/plug and wiring temperature in the final enclosure. Verify worst-bay drive temperatures, supply margins and recovery behavior before unattended use.

## Tools, commands and limits

Used Git 2.52.0, KiCad 10.0.8 Linux Lite AppImage (SHA-256 `4dfc6a52f89aa072d206e466a2a1de4c5ef5975d7ef49da24e4d4db0bb40ba3b`), Python 3.12.14, Gerbonara 1.6.3, Shapely 2.1.2, CadQuery 2.7.0 and OCP 7.8.1.1. The publisher's Lite AppImage was extracted and run with `APPDIR=<extract> <extract>/AppRun kicad-cli …` and isolated `XDG_CONFIG_HOME`. Copied the bundled `sym-lib-table` and `fp-lib-table` into `kicad/10.0/`. Used project-local models and fetched the four referenced stock model files into `KICAD10_3DMODEL_DIR` from the KiCad packages3D master tree. Latest stock-model geometry can differ from the committed exporter, as observed at U2.

```bash
git fetch origin initial-schematic
git rev-parse FETCH_HEAD
git diff --name-status 3294cb89a1b5f34098adf01730dca6feb2a4a3dd 482af6a78fe4e53589d44535311f52b8d44c7214
git worktree add --detach "$out/src" 482af6a78fe4e53589d44535311f52b8d44c7214
cd "$out/src"
p=hardware/fan-controller
"$cli" version
"$cli" sch erc --severity-all --exit-code-violations -o "$out/erc.rpt" "$p.kicad_sch"
"$cli" pcb drc --severity-all --all-track-errors --schematic-parity --exit-code-violations -o "$out/stored.rpt" "$p.kicad_pcb"
"$cli" pcb drc --severity-all --all-track-errors --schematic-parity --refill-zones --exit-code-violations -o "$out/refill.rpt" "$p.kicad_pcb"
"$cli" sch export netlist --format kicadxml -o "$out/net.xml" "$p.kicad_sch"
"$cli" pcb export ipcd356 -o "$out/board.d356" "$p.kicad_pcb"
python scripts/check-schematic-pcb-parity.py "$out/net.xml" "$out/board.d356" "$p.kicad_pcb"
KICAD_CLI="$cli" bash scripts/export-pcb-manufacturing.sh "$out/cam"
python scripts/inspect-pcb-manufacturing.py "$out/cam" "$p.kicad_pcb"
"$cli" pcb export gerbers --board-plot-params -o "$out/gui/" "$p.kicad_pcb"
"$cli" pcb export gerbers -l F.Paste,B.Paste -o "$out/paste/" "$p.kicad_pcb"
"$cli" pcb export pos --format csv --units mm --smd-only --side both -o "$out/position.csv" "$p.kicad_pcb"
"$cli" sch export bom --fields 'Reference,Value,Footprint,QUANTITY,Manufacturer,MPN,DigiKey_PN,Datasheet,Description' \
  --labels 'Reference,Value,Footprint,Quantity,Manufacturer,MPN,DigiKey_PN,Datasheet,Description' \
  --group-by 'Value,Footprint,MPN' --exclude-dnp -o "$out/bom.csv" "$p.kicad_sch"
"$cli" pcb export step --force --board-only -o "$out/bare.step" "$p.kicad_pcb"
"$cli" pcb export step --force --no-dnp -o "$out/fresh.step" "$p.kicad_pcb"
```

For the saved refill, extract `git archive REVIEWED_COMMIT hardware scripts` with the actual SHA into a scratch directory, repeat refilled DRC with `--save-board`, then repeat the CAM export and parse there even though strict DRC returns the metadata-warning exit status. Compare normalized files using the Gerber CreationDate and GenerationSoftware attributes, KiCad created-by line, Excellon date and generator comments and job Header timestamp/generator removed. Compare the job's file attributes by file function/path. The original source and committed CAM were read-only.

For the additional measurements, use the parity reader to parse pad centers and segment/via geometry, transform local footprint pad centers into PCB coordinates and sum centerlines between named pins. Compare S-expression geometry at both SHAs. In Gerbonara/Shapely, form per-net copper unions and test every different-net pair for intersection, then intersect B.Cu GND with the route centerlines and horizontal cross-sections. The repository CAM scripts validate drill/pad alignment, maintain plating connectivity and analyze the other clearances above. OCP's STEPCAF reader enumerates instances by reference and produces transformed solids for envelope, cylinder-axis and model/core overlap checks. The committed model translates STEP X by −65.5 and STEP Y by +89.0 relative to the independent absolute export; STEP Y reverses PCB Y.

No actual JLCPCB CAM review, populated board, physical connectors, live Onshape assembly, thermal or EMC simulations, or firmware execution were available. Calculated losses are screening estimates. Post-fabrication measurements and order-specific vendor review are essential to the stage decisions above.
