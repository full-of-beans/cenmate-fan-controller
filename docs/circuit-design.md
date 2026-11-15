# Fan Controller — Circuit Design Notes

**Design status:** descriptive engineering reference for the `initial-schematic` branch, not a fabrication sign-off.  
**Source of truth:** `hardware/fan-controller.kicad_sch` (component references, values, MPNs and links as inspected 2026-11-15). The schematic and PCB take precedence over this document if revised.

## System overview

The CENMATE six-bay enclosure fan controller uses a Seeed XIAO RP2350 (U1), two board-mounted NTC temperature channels, two four-wire fan interfaces, a P-channel MOSFET to gate the fan 12 V supply with the enclosure rocker enable signal, and an INA228 high-side current/power monitor. The controller is powered separately from the fan-switched 12 V rail. Firmware should fail safely when temperature readings or external telemetry are missing.

## 1. Thermistor channels — TH1/TH2, R5/R6, C3/C4

| Channel | Thermistor | Bias resistor | Filter capacitor |
| --- | --- | --- | --- |
| 1 | TH1: Murata NCU18XH103F60RB, 10 kΩ NTC | R5: 33 kΩ | C3: 100 nF |
| 2 | TH2: same part | R6: 33 kΩ | C4: 100 nF |

The 33 kΩ bias and NTC form a 3.3 V resistive divider. For the modeled orientation (33 kΩ to 3.3 V, NTC to GND), the ADC node **falls** as temperature rises. C3/C4 bypass the ADC nodes to ground, suppressing fan/PWM and supply noise. At 25 °C the divider's Thevenin resistance is approximately 7.67 kΩ, giving an approximately **0.77 ms** RC time constant with 100 nF (the capacitor and ADC input loading affect real settling). Temperature varies slowly, so this filtering is appropriate.

**Thermistor datasheet:** [Murata NCU18XH103F60RB](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8942/NCU18XH103F60RB.pdf). Murata specifies **10 kΩ at 25 °C (±1%)**, **B(25/50) = 3380 K (±1%)**, with reference B(25/80) = 3428 K; operating range −55 to +125 °C. The NTC response is nonlinear; the beta equation below is a useful engineering approximation, **not a substitute for the manufacturer's full R–T curve**:

`R(T) = 10,000 × exp[3380 × (1/(T+273.15) − 1/298.15)] Ω`

`V_ADC = 3.3 × R(T)/(33,000 + R(T))`

The table uses that **constant-3380 K beta approximation**, 3.300 V supply/reference, ideal 12-bit ADC (0–4095), and nominal parts. “mV/°C” is the voltage change over the next 1 °C; “°C/count” is quantization-only, **not absolute temperature accuracy**.

| °C | NTC resistance | ADC voltage | ADC code | Sensitivity | °C per count |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 10 | 18.23 kΩ | 1.174 V | 1457 | 31.6 mV/°C | 0.026 |
| 15 | 14.82 kΩ | 1.023 V | 1269 | 28.4 mV/°C | 0.028 |
| 20 | 12.13 kΩ | 0.887 V | 1101 | 25.2 mV/°C | 0.032 |
| 25 | 10.00 kΩ | 0.767 V | 952 | 22.1 mV/°C | 0.036 |
| 30 | 8.30 kΩ | 0.663 V | 823 | 19.2 mV/°C | 0.042 |
| 35 | 6.92 kΩ | 0.572 V | 710 | 16.6 mV/°C | 0.049 |
| 40 | 5.81 kΩ | 0.494 V | 613 | 14.3 mV/°C | 0.057 |
| 45 | 4.90 kΩ | 0.427 V | 530 | 12.2 mV/°C | 0.066 |
| 50 | 4.16 kΩ | 0.369 V | 458 | 10.5 mV/°C | 0.077 |
| 55 | 3.55 kΩ | 0.320 V | 397 | 8.9 mV/°C | 0.090 |
| 60 | 3.04 kΩ | 0.278 V | 345 | 7.6 mV/°C | 0.105 |

The intended **15–60 °C** measurement range corresponds to about **1.02–0.28 V**, or **1269–345 ADC counts**. Near the intended 35 °C control setpoint, the nominal signal changes about **16–17 mV/°C**, equivalent to about **20 ADC counts/°C**. Thus 12-bit quantization is adequate for control; practical accuracy is limited more by thermistor/bias tolerances, RP2350 ADC noise and reference error, placement/thermal coupling, and beta-curve approximation. Use averaging and calibration/lookup tables where appropriate. The thermistor dissipation at 25 °C is about **0.059 mW** (3.3 V / 43 kΩ ≈ 77 µA); with Murata's typical 1 mW/°C dissipation constant this suggests approximately **0.06 °C self-heating**, highly dependent on board airflow and mounting.

**Firmware:** Use a measured/calibrated ADC reference, reject open/shorted sensors, and consider a manufacturer's tabulated R–T curve for better accuracy than the single-beta approximation.

## 2. Fan PWM / tachometer interfaces — R8–R13, Q1/Q2, J3/J4

| Component | Value / part | Purpose |
| --- | --- | --- |
| R10, R13 | 10 kΩ each | Tachometer pull-ups to the logic rail |
| R8, R11 | 1 kΩ each | Base drive/current limiting for the 2N3904 PWM interface transistors |
| R9, R12 | 100 kΩ each | Base pull-downs; define an inactive state when MCU outputs float |
| Q1, Q2 | onsemi 2N3904BU NPN | Low-side, open-collector-style PWM signal sinks |
| J3, J4 | Molex 0022053041, four positions | Individual four-wire fan connections |

**Datasheets:** [onsemi 2N3904](https://www.onsemi.com/pub/Collateral/2N3903-D.PDF); [Molex 0022053041](https://www.molex.com/pdm_docs/sd/022053041_sd.pdf).

Each fan has a 12 V supply and ground, a PWM control line, and a tachometer feedback line. The transistor stages allow the 3.3 V RP2350 GPIO to pull the fan PWM control signal low without directly driving a potentially fan-supplied logic-high level. The 100 kΩ base pull-down prevents a floating base when the MCU is unpowered or resetting. With 3.3 V GPIO and approximately 0.7 V base-emitter drop, a 1 kΩ base resistor implies about **2.6 mA** base current when driven high; the 100 kΩ pull-down adds only a small load.

**Logic inversion:** An NPN low-side PWM sink inverts the MCU output: GPIO high turns Q1/Q2 on and pulls the fan PWM input low. Firmware must account for this. Four-wire PC fans conventionally expect a roughly **25 kHz PWM control signal** with an open-collector/open-drain-style drive; verify the particular ARCTIC P8 Max implementation and connector pin ordering before testing. Do **not** pull the PWM line to 12 V. Tach outputs are normally open-collector/open-drain pulse trains and need logic-voltage pull-ups; 10 kΩ to 3.3 V implies at most **0.33 mA** sink current per tach channel. Interpret tach frequency using the fan's pulses-per-revolution specification (often two, but confirm).

**Design checks:** Verify the actual J3/J4 pin mapping and transistor footprint orientation against the manufactured parts. Ensure fan PWM/tach signals cannot back-power an unpowered controller or fan; define firmware start-up and loss-of-telemetry behavior. The 12 V fan supply is switched separately in Section 3.

## 3. Fan high-side load switch — R7, Q3, J5

| Component | Value / part | Purpose |
| --- | --- | --- |
| Q3 | Diodes Incorporated DMP3056L-7 P-channel MOSFET, SOT-23 | Switches the fans' positive 12 V supply |
| R7 | 100 kΩ | Gate-to-source pull-up; defaults MOSFET off |
| J5 | JST S2B-XH-A-1(LF)(SN), 2-pin right-angle XH header | Interface to enclosure rocker-enable / return wiring |

**Datasheets / product references:** [DMP3056L-7](https://www.diodes.com/part/view/DMP3056L); [JST XH series](https://www.jst-mfg.com/product/pdf/eng/eXH.pdf).

Q3 **source** connects to unswitched `12V_IN`; **drain** supplies `12V_SWITCH` to the fans; **gate** follows the enclosure rocker-enable line. The enclosure enable terminal was measured approximately **0 V when ON** and **12.2 V when OFF**. Consequently:

| Rocker | Gate | Source | VGS | Fan supply |
| --- | --- | --- | --- | --- |
| ON | ~0 V | ~12.2 V | ~−12.2 V | Enabled |
| OFF | ~12.2 V | ~12.2 V | ~0 V | Disabled |

R7 provides a defined OFF state if the enable lead disconnects. When the rocker pulls the gate to ground, the resistor adds only **12.2 V / 100 kΩ ≈ 0.122 mA** to the switch-control path and dissipates about **1.5 mW**. This is normally negligible, but assumes the backplane enable circuit and `12V_IN` share a supply/ground reference; the existing internal pull-up's exact topology has not been measured. A series gate resistor was discussed but **is not present in the referenced component list**; slow manual switching generally does not require one.

**Critical checks:** Confirm Q3's physical SOT-23 pad numbering against its manufacturer datasheet, not merely the imported symbol labels. Check MOSFET gate-source absolute maximum and worst-case 12 V transients, fan startup/stall current and thermal dissipation, and whether the backplane's enable signal can be exposed to a separately powered 12 V rail. When OFF, `12V_SWITCH` can float; it need not measure 0 V without a load.

## 4. Enclosure current/power sensing — R1–R4, C1/C2, RSH1, U2

| Component | Value / part | Purpose |
| --- | --- | --- |
| U2 | Texas Instruments INA228AIDGSR | 20-bit I²C current, bus-voltage, power/energy monitor |
| RSH1 | Bourns CRF2512-FZ-R001ELF, **1 mΩ** | Four-terminal high-side current shunt |
| R1, R2 | 4.7 kΩ each | I²C SDA/SCL pull-ups to 3.3 V |
| R3, R4 | 10 Ω each | Series input resistors in differential sense filter |
| C1, C2 | 100 nF each | Analog filter/decoupling capacitors; exact placement and function should be checked against the schematic/netlist |

**Datasheets:** [TI INA228](https://www.ti.com/lit/ds/symlink/ina228.pdf); [Bourns CRF shunts](https://www.bourns.com/docs/Product-Datasheets/CRF.pdf).

The shunt is in the enclosure's 12 V high-current path, with Kelvin sense traces routed separately from the load-current copper. At **7 A**, a **1 mΩ** shunt produces **7 mV** differential signal, **7 mV** supply drop and **49 mW** heat. At **9 A**, those become **9 mV**, **9 mV**, and **81 mW**. **Important:** this document uses the *actual schematic part* `CRF2512-FZ-R001ELF` (1 mΩ), not the earlier 0.1 mΩ concept.

The INA228's shunt ADC is **20-bit**, with differential ranges of ±163.84 mV (312.5 nV/LSB) or ±40.96 mV (78.125 nV/LSB). For a 1 mΩ shunt, these ideal voltage increments correspond to **0.3125 mA/LSB** and **0.078125 mA/LSB**, respectively. These are **code increments, not guaranteed current accuracy**: TI specifies up to ±1 µV shunt offset, equivalent to **±1 mA** with this shunt, before shunt tolerance, layout, noise and gain error. The narrower range is ample for the expected single-digit-ampere enclosure current and provides finer code resolution.

R3/R4 with a differential capacitor suppress high-frequency noise entering IN+/IN−. If a **single 100 nF capacitor is connected directly across the sense inputs**, the ideal differential time constant is `(10 + 10) Ω × 100 nF = 2 µs` (cutoff ≈ **79.6 kHz**). **Do not assume both C1 and C2 form this exact topology:** their net connections must be confirmed. A 100 nF supply bypass capacitor should be placed close to INA228 VS/GND. The INA228 should run from the **3.3 V logic rail**, never 12 V; its VBUS and IN+/IN− pins can monitor the higher bus voltage. The 4.7 kΩ pull-ups draw up to ~0.7 mA each when pulled low. Firmware must configure the I²C address pins, ADC range, shunt calibration, averaging and sampling times.

**Design checks:** Confirm the 1 mΩ shunt rating, four-terminal Kelvin geometry, exact C1/C2 placements and U2 footprint (TI's DGS 10-pin VSSOP), and that bus-voltage measurement is connected on the intended side of the shunt. Also distinguish enclosure load current from fan-only current in firmware.

## 5. Microcontroller and power interface — U1

| Component | Value / part | Purpose |
| --- | --- | --- |
| U1 | Seeed Studio XIAO RP2350, MPN 102010550; schematic value `XIAO-RP2350-SMD` | Main controller: ADC, PWM/tach GPIO, I²C, USB telemetry and control logic |

**Documentation:** [Seeed XIAO RP2350 documentation](https://wiki.seeedstudio.com/getting-started-xiao-rp2350/); [Seeed/DigiKey product PDF](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8932/102010550.pdf); [Raspberry Pi RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).

U1 is **not a power switch**. It reads the two thermistor ADC channels, commands the two PWM transistor interfaces, measures fan tach pulses, and reads the INA228 over I²C. The 12 V fan power path is switched by Q3, based on the enclosure rocker signal, independently of U1 firmware. The XIAO operates at **3.3 V GPIO levels** and provides **12-bit ADC readings**; nominal quantization is `3.3 V / 4095 ≈ 0.806 mV/count`. The actual effective resolution and reference stability are worse than this ideal quantization-only model and should be characterized.

The board design has explored supplying U1 from the enclosure's USB 5 V port. Verify the currently implemented power entry, 5 V backfeed protection, USB connection and common ground before powering hardware. Keep all ADC and GPIO inputs within RP2350 electrical limits, including while U1 or the enclosure is off. Avoid driving fan lines into unpowered devices. Firmware should default to a conservative cooling state on invalid sensor readings and stale NAS/SMART telemetry.

## 6. Integration, assumptions and validation

- **Schematic values above are authoritative for this snapshot.** The PCB, footprint mapping and manufacturer specifications still require separate checks.
- **Thermistor estimates are model calculations**, not measured voltages. Verify actual divider orientation, ADC pin assignment and reference voltage before implementing the conversion formula. If the divider is reversed, the voltage table must be inverted (`3.3 V − V_ADC`).
- **PWM/tach**: Confirm signal pin assignments, fan-side pull-ups, polarity/inversion, 25 kHz behavior and tach pulses per revolution.
- **Fan power switch**: Confirm the shared 12 V reference and gate-source protection under startup/shutdown and transient conditions; measure fan spin-up current.
- **Current sensing**: Validate INA228 analog filter topology and Kelvin routing, and use the actual **1 mΩ** shunt value for calibration.
- **Documentation maintenance**: Update this file whenever component values, U1 power source or fan topology change. Rerun ERC/DRC after changes; neither alone validates analog performance or mechanical fit.

## Source index

- [Murata NCU18XH103F60RB](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8942/NCU18XH103F60RB.pdf)
- [onsemi 2N3904](https://www.onsemi.com/pub/Collateral/2N3903-D.PDF)
- [Molex 0022053041](https://www.molex.com/pdm_docs/sd/022053041_sd.pdf)
- [Diodes DMP3056L-7](https://www.diodes.com/part/view/DMP3056L)
- [JST XH series](https://www.jst-mfg.com/product/pdf/eng/eXH.pdf)
- [TI INA228](https://www.ti.com/lit/ds/symlink/ina228.pdf)
- [Bourns CRF shunts](https://www.bourns.com/docs/Product-Datasheets/CRF.pdf)
- [Seeed XIAO RP2350](https://wiki.seeedstudio.com/getting-started-xiao-rp2350/)
- [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
