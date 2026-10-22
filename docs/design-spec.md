# Electronics design specification (initial, review required)

Status: **design baseline, not fabrication approval**. This records discussion decisions and unresolved checks. Current native KiCad files are the source of truth once schematic design begins.

## Objective and physical design
- Target: CENMATE 6-bay USB HDD enclosure / DAS.
- Main controller PCB target: approximately 30 x 100 mm; XIAO RP2350 USB-C on a **short edge**, pointing along the long board axis, accessible from outside.
- Separate NTC strip: approximately 160 x 8 mm, target 0.6-0.8 mm PCB thickness, 6 surface-mount thermistors aligned with SATA bay positions. Limited clearance near backplane (target no more than ~2 mm above board). Physical measurements, insulation, retention, and mounting must be validated. Avoid glue where possible; one-end clip/bracket considered.
- Rear panel supports 2 x ARCTIC P8 Max 80 mm fans; 71.5 mm fan mounting hole spacing. Verify CAD before final PCB placement.

## Power and measurement (fixed intent)
Existing enclosure **switched 12 V DC feed and common ground**, intercepted between the rear-panel manual power switch and HDD backplane:
```
12 V input --+-- [0.2 milliohm series shunt] -- HDD backplane +12 V
             +-- fused fan +12 V branch -------- 2 x fans
             +-- TRACO 12 V -> 5 V ------------ XIAO controller
GND -------------------------------------------- shared return
```
- INA228 measures **only HDD backplane 12 V branch**; excludes both fans and TRACO/controller consumption.
- **Shunt must be in series** with positive HDD 12 V feed, not merely parallel tapped. INA228 Kelvin-sense across shunt, monitor VBUS on load side per datasheet. Confirm actual 12 V wiring, polarity, connector ratings, supply and ground return.
- 10 A continuous current-path design target; Bourns CSS2H-3920R-L200F 0.2 mΩ, 3920 SMD, 2-terminal. At 10 A, ~2 mV and 20 mW. Verify copper thickness/width, heat, protection, and termination; a tiny 30 mm board may constrain this.
### Physical power wiring and service connections (accepted integration plan)
- **Incoming `12V_IN`:** One switched +12 V wire from the existing manual power switch (mounted in the custom rear panel) soldered directly to a dedicated, suitably sized controller PCB wire termination. No detachable input connector. Provide mechanical strain relief; a small generic SMD test point is not an approved high-current wire termination.
- **Outgoing HDD connection:** One **detachable, polarized 2-pin connector** on the controller PCB: pin 1 = `12V_HDD` (after RSH1), pin 2 = `GND`. Two wires from the mating plug terminate at the original SATA backplane PCB. Connector and return wiring must be rated for the full combined HDD current, including startup/inrush; generic `Conn_01x02` is schematic-only.
- **Ground topology:** Shared electrical ground via the backplane return connection, not chassis as the sole return. Keep the high-current HDD return path and sensitive INA228 sense routing appropriately separated on the PCB. Confirm where the supply ground and backplane ground actually join; the controller ground must have a valid return to the power source.
- **Branching:** TRACO/controller and fused fan supply take `12V_IN` upstream of RSH1. Only HDD +12 V passes through the shunt. The controller must not unintentionally bridge around the shunt.
- **USB-only bench testing:** USB-C should power XIAO and low-voltage peripherals only; HDDs and 12 V fans remain unpowered. The Schottky external-5-V arrangement is the working design baseline, but simultaneous USB/external supply behavior remains a pre-fabrication verification item.
- **Pending selection and validation:** Exact switch output conductor and supply topology; wire gauges, solder-hole/pad geometry, strain relief, two-pin connector/plug/crimps, polarity/keying, current/temperature ratings, serviceability, and continuity of backplane ground return. Do not cut or rewire the enclosure until these are confirmed.

- Fan branch fused independently (fuse value/type TBD after fan startup/inrush verification); **never feed fans from USB 5 V**.
- TRACO TSR 0.5-2450 12 V -> 5 V, 500 mA; STPS1L40ZF Schottky on its 5 V output was proposed, but **USB VBUS backfeed/isolation is not yet validated** against XIAO RP2350 schematic. Do not finalize power topology before this check.

## Sensors and signals
- 6 x Murata **NCU18XH103F60RB** 10k NTC (manufacturer suggested as replacement for discontinued NCP18XH103F03RB); use its actual B-value/R-T curve, not original's.
- NTC strip: common GND plus six individual sense wires; no strip-side connector planned; strain relief and insulation. Controller side 7-way keyed connector, kit fit/pitch must be verified.
- 8:1 analog mux: originally TI CD74HC4051M96 (availability problem), proposed substitute onsemi MC74HC4051ADR2G. Verify exact pinout, 3.3 V operation, analog resistance, footprint and ADC error before commitment.
- One 10k pullup from 3.3 V to mux common/ADC node; NTCs to GND. Scan each sensor with ~1 ms mux settle and ~4 ms ADC averaging (30 ms per 6-sensor cycle). Implement nonblocking acquisition, filtering and fan hysteresis in firmware.
- Two independent open-collector NPN fan PWM drivers (2N3904BU), 1k series base resistor + 100k base pulldown each; PWM target ~25 kHz; transistor inversion in firmware. Two fan tachometer lines with 10k pullups to 3.3 V. Verify ARCTIC fan connector, PWM pullup, tach output, and reset-safe behavior.
- INA228AIDGSR (VSSOP-10) 3.3 V, I2C with 2 x 4.7k external pullups. Each shunt Kelvin lead via 100 ohm series resistor to INA228 IN+/-; 100 nF across IC-side sense pins. Add 100 nF local decoupling at INA228 and HC4051. Check TI datasheet layout and input-filter recommendations. At 0.2 mΩ, INA228 sensitive-range ideal quantization is ~4.69 mW at 12 V; **this is not actual accuracy**.

## Still to resolve before PCB layout/fabrication
1. Exact XIAO RP2350 GPIO/pin mapping and schematic symbol/footprint; USB port placement and access.
2. Actual HDD 12 V supply wiring and whether series shunt can be safely inserted on main PCB.
3. Fan-header pinout and physical mating; 2-pin HDD output connector and mating harness, direct-solder 12 V input termination, fuse/holder, wiring and grounding.
4. External-power/USB power-path isolation and backfeed risk.
5. Availability and exact verified footprint of INA228 and HC4051 substitute; do not substitute lower-accuracy INA237 without review.
6. NTC strip thickness, thermistor pads/clearance, sensor spacing, cable attachment and safe mechanical retention.
7. Schematic ERC, datasheet pin-by-pin review, current/thermal layout, DRC, and STEP mechanical validation.
8. BOM sourcing quantities and final purchased manufacturer part numbers.

## Git / review
- Work on branches and PRs; do not push directly to main.
- Owner manually reviews and squash-merges; CODEOWNERS in .github/CODEOWNERS.
- Never claim electrical verification without documented evidence.
