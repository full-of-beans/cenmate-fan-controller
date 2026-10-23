# CENMATE 6-Bay HDD Enclosure Fan Controller

Custom cooling controller and temperature-sensor strip for a **CENMATE 6-bay USB HDD enclosure / DAS (JBOD)**.

## Purpose

Replace or supplement the enclosure's cooling arrangement with two independently controlled **ARCTIC P8 Max 80 mm 4-pin PWM fans**. Monitor temperatures near all six drive bays, read fan tachometers, and measure the HDD backplane's **12 V input current** separately from the fans and controller power.

The controller schematic is in development in PR #3. The [controller schematic verification](docs/controller-schematic-verification.md) records the reviewed revision, verified connections, unresolved electrical issues, layout constraints, and the seven-wire sensor-strip interface. The controller is not ready to freeze or fabricate.

## Planned hardware

- **Controller:** Seeed Studio XIAO RP2350
- **Fans:** 2 × ARCTIC P8 Max, independent PWM and tachometer
- **Temperature sensing:** 6 × Murata NCU18XH103F60RB NTC thermistors on a separate narrow PCB strip (manufacturer-recommended replacement for original sensor)
- **Analog selection:** 8:1 HC4051 multiplexer, e.g. onsemi MC74HC4051ADR2G
- **Current/power monitor:** TI INA228AIDGSR, I²C, measuring a 0.2 mΩ Bourns CSS2H-3920R-L200F shunt
- **Controller supply:** TRACO TSR 0.5-2450, 12 V to 5 V
- **PWM outputs:** Two open-collector NPN transistor drivers
- **Power architecture:** A 12 V supply feeds separate branches for (1) the HDD backplane through the shunt, (2) the fans, and (3) the TRACO converter. Common ground. The INA228 excludes fan and controller consumption.

**Critical:** The shunt must be installed *in series* with the HDD backplane's positive 12 V supply; a parallel voltage tap alone cannot measure current. Confirm the enclosure's actual wiring before modifying it.

## Repository layout

```text
hardware/
  controller/         Main controller KiCad project
  thermistor-strip/   Six-sensor strip KiCad project (placeholder)
  libraries/          Project-specific KiCad symbols/footprints if needed
firmware/             RP2350 firmware (future)
docs/                 Schematic verification and design evidence
mechanical/           Mechanical references and STEP exports (future)
```

## Getting started in KiCad

1. Clone the repository locally.
2. Open `hardware/controller/controller.kicad_pro` in KiCad 10 to work on the existing controller project.
3. Open `hardware/thermistor-strip/thermistor-strip.kicad_pro` for the separate strip project; its design files are placeholders.
4. Follow the verification report and obtain owner approval before hardware changes; do not recreate either project over its existing files.

KiCad is a **local desktop editor**; GitHub stores and versions the files, rather than opening them as an editable online project.

## Design checks before PCB fabrication

- Verify XIAO RP2350 pin assignment, power input, and simultaneous USB/external 5 V behavior.
- Confirm the 12 V backplane wiring, 10 A current-path copper and connectors, branch protection, and Kelvin shunt routing.
- Confirm actual fan-header mating and pinout, reset-safe PWM behavior, and tachometer voltage.
- Verify exact HC4051 pinout, on-resistance effects, and NTC resistance-temperature curve.
- Validate footprints and mechanical clearance against the custom rear panel.

## Status

**Controller schematic under review — freeze blocked; PCB layout and thermistor-strip design pending.** See the [verification report](docs/controller-schematic-verification.md) before editing hardware.
