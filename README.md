# CENMATE 6-Bay HDD Enclosure Fan Controller

Custom cooling controller and temperature-sensor strip for a **CENMATE 6-bay USB HDD enclosure / DAS (JBOD)**.

## Purpose

Replace or supplement the enclosure's cooling arrangement with two independently controlled **ARCTIC P8 Max 80 mm 4-pin PWM fans**. Monitor temperatures near all six drive bays, read fan tachometers, and measure the HDD backplane's **12 V input current** separately from the fans and controller power.

This repository is an **initial project scaffold**. Schematics, PCB layouts, pin assignments, mechanical dimensions, and protection details are not yet finalized or electrically validated.

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
  controller/         Main controller KiCad project (create here)
  thermistor-strip/   Six-sensor strip KiCad project (create here)
  libraries/          Project-specific KiCad symbols/footprints if needed
firmware/             RP2350 firmware (future)
docs/                 BOM, design notes, pin map, validation (future)
mechanical/           Mechanical references and STEP exports (future)
```

## Getting started in KiCad

1. Clone the repository locally.
2. Open KiCad → **File → New Project**.
3. Create the project named `controller` in `hardware/controller/`. KiCad should generate `controller.kicad_pro`, `controller.kicad_sch`, and `controller.kicad_pcb` there. Avoid an extra nested `controller/controller/` folder.
4. Repeat with project name `thermistor-strip` in `hardware/thermistor-strip/`.
5. Save, commit, and push the project files. Later open either `.kicad_pro` directly in KiCad.

KiCad is a **local desktop editor**; GitHub stores and versions the files, rather than opening them as an editable online project.

## Design checks before PCB fabrication

- Verify XIAO RP2350 pin assignment, power input, and simultaneous USB/external 5 V behavior.
- Confirm the 12 V backplane wiring, 10 A current-path copper and connectors, branch protection, and Kelvin shunt routing.
- Confirm actual fan-header mating and pinout, reset-safe PWM behavior, and tachometer voltage.
- Verify exact HC4051 pinout, on-resistance effects, and NTC resistance-temperature curve.
- Validate footprints and mechanical clearance against the custom rear panel.

## Status

**Scaffold only — no fabrication-ready schematic or PCB yet.**
