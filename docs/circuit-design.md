# Fan Controller — Circuit Design Notes

**Design status:** schematic-level engineering reference for the `initial-schematic` branch.  
**Source of truth:** `hardware/fan-controller.kicad_sch` (component references, values, MPNs and links as inspected 2026-11-15). The schematic takes precedence over this document if revised.

## System overview

The CENMATE six-bay enclosure fan controller uses a Seeed XIAO RP2350 (U1), two board-mounted NTC temperature channels, two four-wire fan interfaces, a P-channel MOSFET to gate the fan 12 V supply with the enclosure rocker enable signal, and an INA228 high-side current/power monitor. The controller is powered separately from the fan-switched 12 V rail. Firmware should fail safely when temperature readings or external telemetry are missing.

## 1. Thermistor channels — TH1/TH2, R5/R6, C3/C4

**Planned schematic change:** R5 and R6 will be changed from 33 kΩ to **10 kΩ**, reusing the existing 10 kΩ resistor type already present elsewhere in the BOM. The calculations and component values below describe this selected design; the KiCad schematic may still show 33 kΩ until that edit is made.

| Channel | Thermistor | Bias resistor | Filter capacitor |
| --- | --- | --- | --- |
| 1 | TH1: Murata NCU18XH103F60RB, 10 kΩ NTC | R5: **10 kΩ (selected)** | C3: 100 nF |
| 2 | TH2: same part | R6: **10 kΩ (selected)** | C4: 100 nF |

The 10 kΩ pull-up and NTC form a 3.3 V resistive divider (10 kΩ to 3.3 V, NTC to GND). The ADC node **falls** as temperature rises. C3/C4 bypass the ADC nodes to ground to suppress fan/PWM and supply noise. At 25 °C the divider's Thevenin resistance is **5 kΩ**, giving an approximately **0.50 ms** RC time constant with 100 nF, ignoring ADC loading. The chosen 10 kΩ resistor reuses an existing BOM value while utilizing almost as much of the 12-bit ADC span over 15–60 °C as the mathematically optimal ~6.7 kΩ divider resistor.

**Thermistor datasheet:** [Murata NCU18XH103F60RB](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8942/NCU18XH103F60RB.pdf). Murata specifies **10 kΩ at 25 °C (±1%)**, **B(25/50) = 3380 K (±1%)**, with reference B(25/80) = 3428 K; operating range −55 to +125 °C. The NTC response is nonlinear; the beta equation is an engineering approximation, **not a substitute for the manufacturer's full R–T curve**:

`R(T) = 10,000 × exp[3380 × (1/(T+273.15) − 1/298.15)] Ω`

`V_ADC = 3.3 × R(T)/(10,000 + R(T))`

The table uses a **constant-3380 K beta approximation**, 3.300 V supply/reference, an ideal 12-bit ADC (0–4095), and nominal parts. “mV/°C” is the voltage change over the next 1 °C; “°C/count” is quantization-only, **not absolute temperature accuracy**.

| °C | NTC resistance | ADC voltage | ADC code | Sensitivity | °C per count |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 10 | 18.23 kΩ | 2.131 V | 2644 | 31.9 mV/°C | 0.025 |
| 15 | 14.82 kΩ | 1.970 V | 2445 | 32.3 mV/°C | 0.025 |
| 20 | 12.13 kΩ | 1.809 V | 2245 | 32.1 mV/°C | 0.025 |
| 25 | 10.00 kΩ | 1.650 V | 2048 | 31.3 mV/°C | 0.026 |
| 30 | 8.29 kΩ | 1.496 V | 1857 | 29.9 mV/°C | 0.027 |
| 35 | 6.92 kΩ | 1.350 V | 1675 | 28.2 mV/°C | 0.029 |
| 40 | 5.81 kΩ | 1.213 V | 1505 | 26.2 mV/°C | 0.031 |
| 45 | 4.90 kΩ | 1.086 V | 1347 | 24.1 mV/°C | 0.033 |
| 50 | 4.16 kΩ | 0.970 V | 1203 | 21.9 mV/°C | 0.037 |
| 55 | 3.55 kΩ | 0.864 V | 1072 | 19.8 mV/°C | 0.041 |
| 60 | 3.04 kΩ | 0.769 V | 954 | 17.8 mV/°C | 0.045 |

The intended **15–60 °C** range corresponds to approximately **1.97–0.77 V**, or **2445–954 ADC counts** (**1491 codes** of span). Near the **35 °C** control setpoint, the nominal voltage changes approximately **28.2 mV/°C**, or **35.0 ADC counts/°C**. Twelve-bit quantization is sufficient for fan control; real accuracy is more strongly affected by thermistor/bias tolerance, ADC noise/reference behavior, thermal placement and beta-model approximation. At 25 °C the divider draws **165 µA**, and the NTC dissipates **0.272 mW**. If the dissipation constant is ~1 mW/°C, this corresponds to approximately **0.27 °C** of self-heating; actual heating depends on airflow and PCB mounting.

**Firmware:** Use the selected 10 kΩ pull-up value in the temperature conversion, reject open/shorted sensors, and consider the manufacturer's R–T table or calibration for accuracy.

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

**Logic inversion:** An NPN low-side PWM sink inverts the MCU output: GPIO high turns Q1/Q2 on and pulls the fan PWM input low. Firmware must account for this. The selected **ARCTIC P8 Max (ACFAN00286A)** manufacturer specification confirms a **four-pin PWM fan**, **12 V**, **0.19 A rated current per fan**, and **500–5000 rpm**, with fan stop below 5% PWM. Its documented fan-plug pin order is **1 GND, 2 +12 V, 3 tach output, 4 PWM input**. The standard four-wire fan control convention uses approximately **25 kHz** open-collector PWM; ARCTIC confirms PWM regulation but does **not** explicitly publish a PWM-frequency tolerance or tach pulses-per-revolution figure in its P8 Max specification. [ARCTIC P8 Max specification](https://www.arctic.de/media/3f/04/7e/1689685872/Spec_Sheet_P8_Max_EN.pdf). Do **not** pull the PWM line to 12 V. Tach outputs are normally open-collector/open-drain pulse trains and need logic-voltage pull-ups; 10 kΩ to 3.3 V implies at most **0.33 mA** sink current per tach channel. Interpret tach frequency using the fan's pulses-per-revolution specification (often two, but confirm).

**Connector implementation:** J3/J4 are **Molex 0022053041 four-pin headers**, not the fan's original molded four-pin PC connector. A mating harness must deliberately map the ARCTIC fan's 1/2/3/4 signals to the schematic nets; do not assume connector numbering is interchangeable. The KiCad schematic defines the intended signal mapping; a custom mating harness must preserve it. Q1/Q2 are 2N3904BU devices; check their E/B/C physical orientation against the assigned footprint when assembling. Fan tach pulses-per-revolution and fan behavior with PWM disconnected are not explicitly documented by the ARCTIC spec; characterize if needed. Check for signal-line back-power when the fan 12 V rail or controller is off. The 12 V fan supply is switched separately in Section 3.

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

**Datasheet and rating review:** The Diodes Incorporated [DMP3056L manufacturer datasheet](https://www.tme.eu/pl/Document/250d260377fe1e01ee709e3eafa0c268/DMP3056L.pdf) specifies **30 V VDS**, **±25 V VGS absolute maximum**, **4.3 A continuous at 25 °C** (3.4 A at 70 °C, under the specified thermal mounting conditions), and **50 mΩ maximum RDS(on) at VGS = −10 V**. The schematic symbol assigns **pin 1 = gate, pin 2 = source, pin 3 = drain**, consistent with the device's SOT-23 pin configuration. The symbol's G/S/D numbering should be preserved when assigning the matching SOT-23 footprint. At the two fans' combined **0.38 A rated current**, worst-case room-temperature conduction loss using 50 mΩ is approximately **7.2 mW**, with approximately **19 mV** drop. This is comfortably small for steady operation; the fan specification does **not** give startup/stall peak current, so those transients cannot be certified from the published fan rating. The nominal −12.2 V gate drive is within the ±25 V absolute limit, but **12 V rail spikes and separate-supply sequencing** have not been measured; the margin is not a substitute for transient testing. The enclosure enable topology and exact common-supply behavior remain unverified. When OFF, `12V_SWITCH` may float rather than read zero.

## 4. Enclosure current/power sensing — R1–R4, C1/C2, RSH1, U2

| Component | Value / part | Purpose |
| --- | --- | --- |
| U2 | Texas Instruments INA228AIDGSR | 20-bit I²C current, bus-voltage, power/energy monitor |
| RSH1 | Bourns CRF2512-FZ-R001ELF, **1 mΩ, ±1%, 2 W** | **Two-terminal** high-side current-sense resistor |
| R1, R2 | 4.7 kΩ each | I²C SDA/SCL pull-ups to 3.3 V |
| R3, R4 | 10 Ω each | Series input resistors in differential sense filter |
| C1, C2 | 100 nF each | **C1: INA228 VS-to-GND supply bypass; C2: differential capacitor across filtered IN+/IN− sense nodes** |

**Datasheets:** [TI INA228](https://www.ti.com/lit/ds/symlink/ina228.pdf); [Bourns CRF shunts](https://www.bourns.com/docs/Product-Datasheets/CRF.pdf).

The shunt is in the enclosure's 12 V high-current path. The selected Bourns CRF2512-FZ-R001ELF is **a two-terminal, 2512 metal-element resistor**, not a four-terminal shunt. The component has only two current terminals; the sense connections must branch from those same terminals, rather than from dedicated Kelvin terminals. The [Bourns CRF datasheet](https://www.bourns.com/docs/Product-Datasheets/CRF.pdf) and [part listing](https://www.digikey.com/en/products/detail/bourns-inc/CRF2512-FZ-R001ELF/6211165) specify **1 mΩ, ±1%, 2 W, 2512, ±50 ppm/°C**. At **7 A**, a **1 mΩ** shunt produces **7 mV** differential signal, **7 mV** supply drop and **49 mW** heat. At **9 A**, those become **9 mV**, **9 mV**, and **81 mW**. The dissipation at 9 A is **81 mW, ~4.1% of the 2 W nominal rating**; actual copper area and enclosure temperature affect thermal performance.

The INA228's shunt ADC is **20-bit**, with differential ranges of ±163.84 mV (312.5 nV/LSB) or ±40.96 mV (78.125 nV/LSB). For a 1 mΩ shunt, these ideal voltage increments correspond to **0.3125 mA/LSB** and **0.078125 mA/LSB**, respectively. These are **code increments, not guaranteed current accuracy**: TI specifies up to ±1 µV shunt offset, equivalent to **±1 mA** with this shunt, before shunt tolerance, layout, noise and gain error. The narrower range is ample for the expected single-digit-ampere enclosure current and provides finer code resolution.

**Schematic connectivity checked:** C2 (100 nF) connects between the two filtered shunt sense nodes, downstream of **R3 and R4 (10 Ω each)**. The ideal differential RC time constant is `(10 + 10) Ω × 100 nF = 2 µs` (cutoff ≈ **79.6 kHz**). C1 (100 nF) connects from the **INA228 VS supply rail to GND**, providing local supply bypass. The INA228 VBUS pin connects to the **12V_HDD (load-side)** net. The shunt measures **enclosure/HDD feed current**, not the separate fan-switched branch. The INA228 should run from the **3.3 V logic rail**, never 12 V; its VBUS and IN+/IN− pins can monitor the higher bus voltage. The 4.7 kΩ pull-ups draw up to ~0.7 mA each when pulled low. Firmware must configure the I²C address pins, ADC range, shunt calibration, averaging and sampling times.

**Package selection:** U2's schematic footprint is `Package_SO:TSSOP-10_3x3mm_P0.5mm`, corresponding to the TI **DGS0010A / VSSOP-10** package family (3 × 3 mm body, 0.5 mm pitch). The INA228 is supplied from **3.3 V**, while VBUS measures the load-side **12V_HDD** net. RSH1 is physically present on the PCB as a two-pad resistor. A two-terminal shunt introduces possible copper/interconnect resistance error; that limitation is relevant when interpreting measurement accuracy. Firmware must label INA228 telemetry **enclosure/HDD current**, not fan-only current.

## 5. Microcontroller and power interface — U1

| Component | Value / part | Purpose |
| --- | --- | --- |
| U1 | Seeed Studio XIAO RP2350, MPN 102010550; schematic value `XIAO-RP2350-SMD` | Main controller: ADC, PWM/tach GPIO, I²C, USB telemetry and control logic |

**Documentation:** [Seeed XIAO RP2350 documentation](https://wiki.seeedstudio.com/getting-started-xiao-rp2350/); [Seeed/DigiKey product PDF](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8932/102010550.pdf); [Raspberry Pi RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).

U1 reads the two thermistor ADC channels, commands the two PWM transistor interfaces, measures fan tach pulses, and reads the INA228 over I²C. The 12 V fan power path is switched by Q3, based on the enclosure rocker signal, independently of U1 firmware. The XIAO operates at **3.3 V GPIO levels** and provides **12-bit ADC readings**; nominal quantization is `3.3 V / 4095 ≈ 0.806 mV/count`. The actual effective resolution and reference stability are worse than this ideal quantization-only model and should be characterized.

U1's power wiring and common ground are defined by the current KiCad schematic. Review power sequencing and possible USB 5 V backfeed against the actual assembled enclosure, particularly when fan 12 V and controller USB power are not present simultaneously. Keep all ADC and GPIO inputs within RP2350 electrical limits, including while U1 or the enclosure is off. Avoid driving fan lines into unpowered devices. Firmware should default to a conservative cooling state on invalid sensor readings and stale NAS/SMART telemetry.

## 6. Integration, assumptions and validation

- **Schematic values above are authoritative for this snapshot.** This document explains circuit intent, component selection and expected electrical behavior; it does not assess PCB implementation.
- **Thermistor estimates are model calculations**, not measured voltages. Verify actual divider orientation, ADC pin assignment and reference voltage before implementing the conversion formula. If the divider is reversed, the voltage table must be inverted (`3.3 V − V_ADC`).
- **PWM/tach**: The ARCTIC fan's pin functions and 0.19 A rating are documented. Verify the custom Molex-to-fan harness continuity and tach pulses/revolution on hardware; ARCTIC does not publish every timing detail.
- **Fan power switch**: Datasheet voltage/current ratings and nominal steady-state dissipation have been checked. Fan spin-up/stall current and 12 V transients are not specified by the available design inputs.
- **Current sensing**: The schematic's C1 supply bypass, C2 differential filter, R3/R4, U2 VBUS load-side connection and U2 package are identified. The **two-terminal 1 mΩ** shunt value is used for calibration.
- **Documentation maintenance**: Update this file whenever component values, U1 power source or fan topology change. Schematic ERC does not validate analog performance.

## Source index

- [Murata NCU18XH103F60RB](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8942/NCU18XH103F60RB.pdf)
- [onsemi 2N3904](https://www.onsemi.com/pub/Collateral/2N3903-D.PDF)
- [Molex 0022053041](https://www.molex.com/pdm_docs/sd/022053041_sd.pdf)
- [Diodes DMP3056L-7 manufacturer datasheet](https://www.tme.eu/pl/Document/250d260377fe1e01ee709e3eafa0c268/DMP3056L.pdf)
- [ARCTIC P8 Max specification and connector diagram](https://www.arctic.de/media/3f/04/7e/1689685872/Spec_Sheet_P8_Max_EN.pdf)
- [JST XH series](https://www.jst-mfg.com/product/pdf/eng/eXH.pdf)
- [TI INA228](https://www.ti.com/lit/ds/symlink/ina228.pdf)
- [Bourns CRF shunts](https://www.bourns.com/docs/Product-Datasheets/CRF.pdf)
- [Seeed XIAO RP2350](https://wiki.seeedstudio.com/getting-started-xiao-rp2350/)
- [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
