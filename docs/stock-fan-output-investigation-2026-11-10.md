# Stock fan power and connector investigation (2026-11-10)

## Scope and evidence

These are owner-reported observations of the existing CENMATE 6-bay HDD enclosure fan wiring, gathered during exploration of a simpler fan-controller power architecture. They are **not** a validated regulator rating or approval to substitute higher-current fans.

**Status reconciled 2026-11-21:** powering replacement fans from the stock 2-pin outputs was **considered but not adopted**. This file preserves the experiment, not a second circuit specification. The final CMFC001 hardware was merged into `main` in [PR #6](https://github.com/full-of-beans/cenmate-fan-controller/pull/6). The inspected hardware/documentation baseline is [`3c8fc34a2a3ca2ecf6e2fdb442d05977b714f652`](https://github.com/full-of-beans/cenmate-fan-controller/tree/3c8fc34a2a3ca2ecf6e2fdb442d05977b714f652); the deleted `initial-schematic` branch's final PR head was `52c36c09f20104b0e3a8dbf3593d9b400edf9369`, with identical merged content.

Use the [schematic](../hardware/fan-controller.kicad_sch) for component choices and connections, the [PCB](../hardware/fan-controller.kicad_pcb) for implementation, [circuit-design notes](circuit-design.md) for circuit intent, and the [hardware revision register](hardware-revisions.md) for immutable manufacturing provenance and outstanding qualification. The ordered CMFC001 source remains `513063f552a1c78585b386359e5f215d1ef3b81e`; the later documentation/merge baseline does not replace it.

## Observed hardware

- The enclosure has **two separate 2-pin fan outputs**, each supplying one stock fan. Stock fans use red (+) and black (-) leads. These are observed lead colors, not a verified connector cavity/pin-number assignment.
- Stock fan hub identifies **Jingweite / JING WEI TE JWT7015**, nominal 70 × 70 × 15 mm, 12 V. Hub marking does not identify an L/M/H catalog suffix.
- The stock cable connector has distinctive arrow-like molded/keying features. After comparison with photographs and listings, the owner identified a visually matching **XH-style 2-pin family**, sold as **“XH2.54”**, on AliExpress. A 10 cm female-to-female cable was considered for a proposed connection to two male PCB headers; this was a historical candidate, not a verified fit or current procurement requirement.
- **Connector identification remains provisional:** two positions, wire colors and molded appearance were observed; measured pitch, physical mating with purchased samples, latch/keying compatibility and cavity polarity were not documented as verified. Marketplace naming does not establish genuine JST manufacture or interchangeability. Genuine [JST XH](https://www.jst-mfg.com/product/pdf/eng/eXH.pdf) is specified at **2.50 mm pitch**, rather than the listing's “2.54 mm.”
- Photos show nearby inductors marked 4R7, electrolytic capacitors, and small ICs around the two FAN headers. These do not establish output current rating or exact regulator topology.

## Electrical measurements (owner, 2026-11-10)

| Quantity | Observation | Notes |
| --- | --- | --- |
| Stock fan current | approximately **0.050 A (50 mA)** | Multimeter connected **in series** with one stock fan's red positive lead, fan operating; not a characterization of both fans or startup peaks |
| Stock fan supply voltage | approximately **12.1 V DC** | Multimeter measured **across** fan red/black while operating |
| Calculated stock fan input power | **0.605 W** | 12.1 V × 0.050 A; approximate, not separately measured |
| Original fan output capacity | **unknown** | 50 mA running load does not establish maximum current or startup capacity |

The original investigation recorded JWT7015 manufacturer catalog figures of approximately 0.11 A (L), 0.18 A (M), and 0.21 A (H) for 12 V versions. These are catalog variants, not measurements or identification of the installed OEM fan. The observed ~0.05 A is lower; the exact OEM variant, measurement conditions, and reason for the discrepancy are **unresolved**. Do not infer a specific model suffix or fan performance curve from this measurement.

Manufacturer reference retained from the investigation: [JWT7015 catalog page](https://www.szjwfan.com/161.html).

## Replacement-fan comparison

The [ARCTIC P8 Max specification](https://www.arctic.de/media/3f/04/7e/1689685872/Spec_Sheet_P8_Max_EN.pdf) lists **ACFAN00286A**, an 80 × 80 × 25 mm four-pin PWM fan, at **0.19 A and 12 V**. Calculated input power is **2.28 W per fan**, and its specified current is approximately **3.8× the observed running current** of the tested stock fan. Two fans total **0.38 A / 4.56 W** at those nominal values. These are specification-based calculations, not measured replacement-fan consumption or guaranteed startup/stall limits.

This comparison explains why the stock fan's 50 mA measurement could not qualify the original outputs for P8 Max loads. Their capacity remains unverified; testing them at higher load is not a prerequisite for the adopted controller, which bypasses them for replacement-fan power.

## Architecture implications — considered but not adopted

The November 10 alternative would have reused each stock 2-pin output for one replacement fan's 12 V/GND, used enclosure USB 5 V for the controller, and retained independent PWM/tach control and thermistors. It could have omitted the dedicated 12 V pass-through, shunt/current monitor and fan supply switch, with optional external power telemetry.

That alternative was not adopted. The investigation did not establish continuous/startup capacity or grounding/switching behavior sufficient to justify it. The committed design instead retains a dedicated supply path, HDD current measurement and rocker-controlled fan power. The unresolved output capability supports retaining the independent supply, but this is an engineering inference from the investigation and final circuit, not a recorded full decision rationale or evidence that the stock outputs failed a load test.

At the inspected baseline, the retained architecture is:

- **HDD pass-through:** J1, Tensility **54-00286**, accepts the dedicated center-positive 12 V input. The HDD feed passes through RSH1 to J2, the **Phoenix Contact 5442206** high-current two-position screw-terminal connection to the enclosure. J2 pin 1 is `12V_HDD`, pin 2 is GND. J1 is rated 7 A despite the enclosure brick's 9 A label; the accepted prototype limitation and required load checks remain in the README.
- **Current measurement:** RSH1 is **Bourns CRF2512-FZ-R001ELF, 1 mΩ**, a **two-terminal** resistor with separate Kelvin-style sense takeoffs through R3/R4 to **TI INA228AIDGSR (U2)**. It is not a four-terminal shunt. U2 monitors the HDD branch, with VBUS on `12V_HDD`; the fan branch originates before the shunt and is not included in that current measurement. Calibration/layout resistance remain qualification items.
- **Fan supply:** **DMP3056L-7 (Q3)** switches `12V_IN` to `12V_SWITCH`. The original rocker-enable signal reaches its gate through **J5**, with **R7, 100 kΩ**, pulling the gate to source for an off state if the lead opens. This is a control connection to the enclosure switch, not power drawn from a stock fan output.
- **Fan interfaces:** J3/J4 provide two independent four-pin outputs with switched 12 V, ground, separate tach inputs and separate transistor-driven PWM signals. The intended physical connectors are MODDIY right-angle fan headers; current Molex CAD footprints/models remain placeholders whose actual fit and orientation need verification, as described in the README.
- **Temperature/controller power:** TH1/TH2 are on-board **Murata NCU18XH103F60RB 10 kΩ NTCs**, with 10 kΩ pull-ups. U1 is a **Seeed XIAO RP2350**. Its schematic VBUS connection is on `+5V`; the README specifies enclosure USB-A to XIAO USB-C as the external 5 V source. The external cable/source is an integration requirement, not an independently tested power source established by this stock-output experiment.

**The stock 2-pin fan outputs are not in the final replacement-fan power path.** Refer to the circuit-design notes for the maintained circuit explanation rather than extending this historical snapshot into a specification.

## Connector relevance to the final controller

The stock-output connector investigation applies to the rejected power alternative and possible stock-fan servicing. The final controller does have a specified **JST S2B-XH-A-1(LF)(SN) two-pin right-angle header at J5**, but it serves the **rocker-enable/ground interface**. Selection of that part does not prove the original fan plugs' manufacturer, dimensions, mating compatibility or polarity. J3/J4 are separate four-pin PWM fan interfaces. The historical XH cable candidate is not a required replacement-fan power adapter.

## Disposition of original validation tasks

No bench or physical-mating validation is marked complete by this documentation revision. The prototype is ordered, not commissioned; the hardware revision register records **25 boards plus one stencil**, order `W2026112104450984`.

| Original task or unresolved question | Disposition | Current relevance |
| --- | --- | --- |
| Verify both stock-output pinouts, connector fit and polarity with real cables | Rejected alternative only; still unverified | No stock-output adapter is required by CMFC001. Verify actual J3/J4 fan harness and J5 switch wiring separately during assembly. |
| Establish each stock output's continuous-load and fan-startup capacity | Rejected alternative only; still unverified | No active implementation task. Neither the 50 mA observation nor a connector rating establishes backplane capacity. |
| Check compatibility between both stock-output grounds and USB controller ground | Rejected alternative only; still unverified | The final common-ground circuit supersedes the proposed interconnection. Actual USB/12 V sequencing and back-power behavior still require testing. |
| Check PWM-disconnected / MCU-unpowered behavior and fail-safe cooling | Still applicable | Qualify J3/J4 PWM/tach operation, startup/reset and firmware behavior on the adopted circuit. No completed test is asserted here. |
| Check whether enclosure switching turns fan power on/off correctly | Still applicable; mechanism superseded | Q3/J5 implement rocker-controlled power. Circuit notes record ~0 V ON / ~12.2 V OFF at the enable terminal; this is not full assembled-system validation. |
| Keep design/manufacturing files unchanged pending the alternative's validation | Superseded | Subsequent engineering retained the dedicated architecture and ordered CMFC001. This record imposes no hardware-edit hold; preserve the ordered release per the revision register. |
| Resolve stock OEM suffix / catalog-versus-measured current discrepancy | Historical uncertainty | Preserve the observation; not a blocker for the independent replacement-fan supply. |

## Remaining work relevant to the adopted design

Follow the existing [README commissioning guidance](../README.md#ordered-prototype-and-validation) and [revision register](hardware-revisions.md#manufacturing-verification): verify actual connector fit/polarity, staged power-up, USB/12 V sequencing, rocker switching, PWM/tach and safe startup, current-monitor calibration, thermistors and loaded-enclosure behavior. These remain prototype qualification tasks; this investigation supplies no evidence that they have passed. Stock-output capacity testing and stock-output adapter procurement are not active requirements.
