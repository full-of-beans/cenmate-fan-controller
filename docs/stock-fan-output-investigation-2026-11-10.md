# Stock fan power and connector investigation (2026-11-10)

## Scope and evidence

These are owner-reported observations of the existing CENMATE 6-bay HDD enclosure fan wiring, gathered during exploration of a simpler fan-controller power architecture. They are **not** a validated regulator rating or approval to substitute higher-current fans.

## Observed hardware

- The enclosure has **two separate 2-pin fan outputs**, each supplying one stock fan. Stock fans use red (+) and black (-) leads. Confirm polarity electrically before wiring any adapter or PCB.
- Stock fan hub identifies **Jingweite / JING WEI TE JWT7015**, nominal 70 × 70 × 15 mm, 12 V. Hub marking does not identify an L/M/H catalog suffix.
- The stock cable connector has distinctive arrow-like molded/keying features. After comparison with photographs and listings, the owner identified a visually matching **XH2.54 (2.54 mm pitch) 2-pin connector family** on AliExpress and found **10 cm female-to-female XH2.54 cables** suitable for a proposed connection to two male PCB headers.
- **Connector identification remains provisional** until physical mating, pitch, latch/keying, and polarity are verified with actual samples. Do not assume JST-XH certification, pin 1 orientation, or interchangeability solely from marketplace naming.
- Photos show nearby inductors marked 4R7, electrolytic capacitors, and small ICs around the two FAN headers. These do not establish output current rating or exact regulator topology.

## Electrical measurements (owner, 2026-11-10)

| Quantity | Observation | Notes |
| --- | --- | --- |
| Stock fan current | approximately **0.050 A (50 mA)** | Multimeter connected **in series** with one stock fan's red positive lead, fan operating |
| Stock fan supply voltage | approximately **12.1 V DC** | Multimeter measured **across** fan red/black while operating |
| Calculated stock fan input power | **0.605 W** | 12.1 V × 0.050 A; approximate, not separately measured |
| Original fan output capacity | **unknown** | 50 mA running load does not establish maximum current or startup capacity |

The JWT7015 manufacturer catalog lists 12 V versions at approximately 0.11 A (L), 0.18 A (M), and 0.21 A (H). The observed ~0.05 A is lower than these catalog figures; the exact OEM variant, measurement conditions, and reason for the discrepancy are **unresolved**. Do not infer a specific model suffix or fan performance curve from this measurement.

Manufacturer reference: https://www.szjwfan.com/161.html

## Proposed replacement and power-capacity validation

The proposed **ARCTIC P8 Max 80 mm PWM** fan has a nominal/rated current of approximately **0.19 A at 12 V**, or **2.28 W**. This is approximately **3.8× the observed running current** of the stock fan. Actual steady-state and startup current may differ.

**Each existing enclosure fan output must be validated at the replacement fan's expected load, including startup demands, before the outputs can be relied on for the redesigned controller.** The output current capacity is currently unknown; no higher-load validation has been completed.

## Architecture implications — proposal, not an implemented change

Potential simplification: reuse each stock 2-pin output for the corresponding P8 Max fan's 12 V/GND; use enclosure USB 5 V for the XIAO/controller; retain independent PWM/tach control and on-board thermistors. This could eliminate the separate high-current 12 V pass-through path, barrel jack, shunt/INA228, screw terminal, and high-side fan supply switch, with optional power telemetry sourced via NAS/smart plug.

Before schematic changes, verify:
1. Both output pinouts and connector fit with real cables; label polarity explicitly.
2. Each output's continuous-load and actual fan startup behavior.
3. Ground compatibility between both fan outputs and USB-powered controller **before connecting grounds**.
4. PWM fan behavior with PWM disconnected or MCU unpowered, and safe startup/fail-safe cooling.
5. Whether enclosure switching reliably turns fan power on/off in the desired states.

Keep existing schematic and manufacturing artifacts unchanged pending validation.
