# CMFC001 firmware architecture

The RP2350 firmware runs on the XIAO RP2350 in the ordered **CMFC001** prototype. Firmware started from [repository commit 6ffd5dd](https://github.com/full-of-beans/cenmate-fan-controller/tree/6ffd5dd17bffbed2ae37e57d05c8f3c74b49a291). The [manufacturing source](https://github.com/full-of-beans/cenmate-fan-controller/tree/513063f552a1c78585b386359e5f215d1ef3b81e/hardware) and that commit have identical schematic and PCB Git blobs. The production archive checksum matches the [hardware revision register](hardware-revisions.md). Pin mapping is verified from the CAD nets, installed-parts intent, and the [Seeed pin map](https://wiki.seeedstudio.com/xiao_rp2350_arduino/#pin-map). Physical installation and electrical behavior await [bench verification](hardware-bringup.md).

## Hardware mapping

The firmware calls TH1 and J3 **upper**, and TH2 and J4 **lower**. TH1 is at board coordinate Y=56.0 mm and TH2 at Y=122.05 mm. Mount with TH1 at the upper plenum and route J3 to the upper fan. Verify actual placement and harness orientation on the first article.

| Signal | CMFC001 references | XIAO pad / GPIO | Peripheral and electrical behavior |
| --- | --- | --- | --- |
| Upper plenum | TH1, R5, C3 | D0, GPIO26 | ADC0. R5 to 3.3 V and TH1 to ground. Hotter reads lower. |
| Lower plenum | TH2, R6, C4 | D1, GPIO27 | ADC1. R6 to 3.3 V and TH2 to ground. Hotter reads lower. |
| INA228 SDA and SCL | U2 pins 4 and 5, R1/R2 | D4/D5, GPIO6/GPIO7 | I²C1 at 100 kHz, 4.7 kΩ pull-ups to 3.3 V. |
| Upper fan PWM | R8/R9, Q1, J3 pin 4 | D8, GPIO2 | PWM slice 1 channel A. GPIO high sinks fan PWM low. |
| Lower fan PWM | R11/R12, Q2, J4 pin 4 | D9, GPIO4 | PWM slice 2 channel A. GPIO high sinks fan PWM low. |
| Upper fan tach | J3 pin 3, R10 | D7, GPIO1 | GPIO falling-edge IRQ. Fan collector/drain sinks line pulled up to 3.3 V. |
| Lower fan tach | J4 pin 3, R13 | D10, GPIO3 | Same as upper, independent pulse counter. |
| Fan supply | J3/J4 pins 1 and 2, Q3, J5, R7 | — | Ground and switched 12 V. Rocker ON pulls Q3 gate low. R7 returns the gate to source if the lead opens. |
| Enclosure feed | J1, RSH1, J2, R3/R4, U2 pins 8–10 | — | IN+ senses upstream, IN− downstream. VBUS is load-side at J2 pin 1. Positive current flows into the enclosure. J2 pin 2 is ground. |
| Logic supply and USB | U1, U2 pin 6, C1 | U1 USB-C, 3V3 | USB powers U1. Its 3.3 V rail supplies U2, thermistors and logic pull-ups. USB data goes directly to U1. |
| INA228 address and alert | U2 pins 1–3 | — | A0/A1 are both grounded, giving 7-bit address 0x40. ALERT is unconnected. |

U2 is INA228AIDGSR and RSH1 is the two-terminal Bourns CRF2512-FZ-R001ELF 1 mΩ shunt. The fan rail branches before RSH1. J2 measurements exclude that fan branch and include everything powered downstream of J2. An independently powered XIAO is excluded. If its USB power comes from the enclosure's downstream USB port and that port derives its power from the shunted feed, that contribution is included. The enclosure's USB power sourcing is unverified. Q3 switches fan 12 V directly from the rocker circuit. U1 has no rocker, J3/J4 supply-voltage, or fan-current measurement.

TH1/TH2 are Murata NCU18XH103F60RB (10 kΩ at 25 °C, nominal B25/50 3380 K). The ratiometric conversion applies a configurable ADC fraction gain, computes R = Rpullup × fraction / (1 − fraction), then applies the beta equation in kelvin and a Celsius offset. Low/high rails report short/open candidates, including ADC saturation. Invalid acquisition and temperatures outside −55…125 °C have no physical temperature. These channels report **rear plenum** temperatures. See [circuit design notes](circuit-design.md) for component sources.

### INA228 sampling

At initialization, read manufacturer ID 0x5449 and device ID 0x228 (upper 12 bits), program CONFIG = 0x0010 (±40.96 mV range), and set SHUNT_CAL = 0. Each shot writes ADC_CONFIG = 0x3B42 (shunt and bus, 1052 µs each, 16 averages, about 34 ms nominal). Poll conversion ready after 36 ms, allow up to 70 ms, then read both raw 24-bit registers and verify CONFIG. Each register transaction shares a 2 ms deadline. Bus errors reinitialize the controller and allow a bounded open-drain bus clear. A missing or wrong device leaves other measurements and fan control running.

Decode bits 23:4, including two's-complement sign for shunt voltage. Scale the signed shunt code by 78.125 nV (312.5 nV in wide-range conversion tests) and the nonnegative bus code by 195.3125 µV. DIAG_ALRT trim, overflow and shunt-limit bits, clipped ADC rails, and invalid bus codes invalidate physical readings. The raw fields still show acquired register values. Firmware calculates amperes as `current_gain × (shunt_v / shunt_ohm) + current_offset_a` and watts as calibrated bus volts × calibrated amperes. Bus voltage correction is gain and offset with a zero floor. The ADC register map, timing, limits and scaling follow the [TI INA228 datasheet](https://www.ti.com/lit/ds/symlink/ina228.pdf).

### Fan I/O and independent power

The RP2350 starts both PWM slices with preloaded compare values. Logical fan duty of 0% sinks the PWM input continuously; 100% releases it continuously. Intermediate values use a nominal 25 kHz carrier, rounded to a hardware clock tick. Firmware commands integer percentages. Q1/Q2 base pull-downs leave the lines released during MCU reset and before PWM is enabled. A powered fan's response to released PWM, its physical stop behavior and any transition when the hardware takes over are bench checks. IRQ counts wrap modulo 2³² and are read in a short IRQ-masked section. RPM uses the actual window length and configured pulses/revolution. Zero pulses are a valid observation, including when a fan is unpowered. A flash operation invalidates the affected tach window.

USB power can keep sensing, configuration and PWM logic alive independently of the rocker and J1. Inversion is implemented using sinks with no driven high voltage at the fan PWM connector. Tach lines have 3.3 V pull-ups and can still present voltage to an unpowered fan. Backfeed through tach, USB, enclosure wiring, and reverse power sequencing must be measured on the assembly before routine operation. No firmware-driven rail switch or signal isolation is present.

## Execution and control

One core owns application state in a cooperative loop. Hardware I/O lives in `platform.cpp`. `model.cpp` converts cached signals, tracks host freshness and power evidence, and evaluates diagnostics and control. `runtime.cpp` owns desired and saved settings. `storage.cpp` implements the redundant record format against a narrow backend, and `protocol.cpp` owns bounded NDJSON and stream state. The native suites run conversion, control, storage, and protocol using simulated inputs.

| Work | Cadence |
| --- | --- |
| INA228 | Trigger every 100 ms, read in a later iteration when ready |
| Thermistors | Every 100 ms, first sample after divider settling and one discarded read after mux changes |
| Tach window | At least 1 s, using actual elapsed time |
| Diagnostics and control | On new acquisition, on operating/configuration changes, or after 100 ms |
| Future cooling policy | Phase 1 uses a fixed fallback. A calibrated policy can later evaluate on its own cadence. |
| USB | Polled without a wait. At most 128 application RX bytes per loop, one pending outgoing line. |
| Stream | 1 s by default when enabled |
| Watchdog | 2 s timeout, refreshed after a control pass even when sensors report bounded failures |

The watchdog remains active in both modes. Monotonic times are 64-bit milliseconds. Priority is given to processing the control loop through bounded I²C, USB parsing and static transmission storage. Flash programming temporarily suspends code and IRQ service while the PWM peripheral holds its previous duty. Tach windows affected by a write are discarded. No MCU, host, or enclosure control action depends on successful streaming.

- **AUTO** defaults to both fans at 100% and reports its model as uncalibrated. A supplied fallback below 100% is used only with inferred ON and no protective condition or startup boost. Unknown or OFF keeps logical 100% prepared for power restoration. Confirmed overtemperature, confirmed stall, or invalid required measurements force full PWM. Missing optional host telemetry is informational. Recovery requires valid measurements and dwell after clearing faults. A hot HDD remains latched until fresh data for the same drive crosses the hysteresis threshold. If more than six unresolved hot identities accumulate, full duty remains in AUTO until reboot.
- **FIXED** immediately selects the two commanded logical duties once applied, including 0%. It ignores all automatic overrides and transitions. Diagnostics continue. The user or experiment host owns thermal protection. If the host disconnects or stops, the last FIXED duties remain indefinitely. Settings restore from valid flash at boot, before PWM outputs are enabled.
- **Power inference** defaults to UNKNOWN. Once an operator marks thresholds validated, low voltage plus low absolute current can infer OFF, and high voltage plus sufficient positive current can infer STARTING followed by ON after the debounce interval. ON uses a lower current exit threshold while the rail stays present. Ambiguity or invalid input returns UNKNOWN. Low current at a present rail alone never indicates OFF. A missing tach measurement never establishes a stall. Confirmed stall requires configured minimum duty, elapsed grace and confirmation windows, valid tach sampling, inferred enclosure ON, and a recent pulse from the other fan as corroboration of shared fan rail power. Loss of both fans can remain unconfirmed without direct rail feedback.
- **Host observations** are generic complete sets of up to six drives, with monotonic ages for temperatures and other observations. A host can, for example, adapt `disk-monitor` processed snapshots and pass the underlying observation ages through USB. The firmware knows only its USB schema. The Phase 1 host eligibility indicator gives no cooling authority, while fresh hot HDD measurements participate in AUTO protection.

## Flash and evolution

The [XIAO RP2350 specification](https://wiki.seeedstudio.com/xiao_rp2350_arduino/) and Pico SDK **2.3.0** declare 2 MiB physical flash. The linker caps application flash at offset 0x1FE000. Records use two final 4 KiB sectors at offsets 0x1FE000 and 0x1FF000, one 256-byte page per sector. Each record contains magic, schema version, generation, explicitly encoded little-endian values and CRC-32. A commit erases the inactive sector, writes, reads back and checks the page before treating it as saved. Power loss during a write leaves the preceding valid copy available. Both copies invalid or erased falls back to defaults. A corrupt non-erased record is reported. Identical saves skip writes after the first valid record. Erasing the whole chip removes settings. Preserve the allocation when replacing firmware.

The future cooling-policy boundary accepts a calibrated equal-duty demand with diagnostics and current observations available. Characterize conservative power/plenum/temperature relationships and transients before implementing a policy. Use fresh independent HDD observations to validate the autonomous estimate. A later adaptive layer may learn only in stable AUTO, with valid data, no active faults and no enclosure transitions. Increase estimates promptly on demonstrated underprediction and reduce them cautiously within bounds. Keep the independent safety margin, administrative disable and separately inspectable/resettable learned state. Use a separate bounded checkpoint design before persisting learned corrections.
