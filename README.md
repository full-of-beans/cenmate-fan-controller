# CENMATE 6-Bay HDD Enclosure Fan Controller

Custom cooling controller for a **CENMATE 6-bay USB HDD enclosure / DAS (JBOD)**.

## Purpose

Replace or supplement the enclosure's cooling arrangement with two independently controlled **ARCTIC P8 Max 80 mm 4-pin PWM fans**. Monitor two local rear-plenum temperatures and six remote bay temperatures, read fan tachometers, and measure the HDD backplane's **12 V input current** separately from the fans and controller power.

PR #3 establishes the initial Seeed Studio XIAO RP2350 schematic, including the eight-thermistor sensing interface. This is the current schematic baseline, not a fabrication-ready design. The [controller schematic verification](docs/controller-schematic-verification.md) records the reviewed revision, verified connections, unresolved electrical issues, layout constraints, and the seven-wire sensor-strip interface. The controller is not ready to freeze or fabricate.

## Initial schematic hardware

- **Controller:** Seeed Studio XIAO RP2350
- **Fans:** 2 × ARCTIC P8 Max, independent PWM and tachometer
- **Temperature sensing:** 2 × local NTC thermistors plus 6 × remote NTC thermistors on a separate sensor strip, selected through an HC4051 analog multiplexer; the strip uses a seven-wire signal/ground interface.
- **Current/power monitor:** TI INA228AIDGSR, I²C, measuring a 0.2 mΩ Bourns CSS2H-3920R-L200F shunt
- **Controller supply:** TRACO TSR 0.5-2450 (12 V to 5 V) through a Schottky diode to the XIAO 5 V/VBUS pin. **USB host backfeed is an unresolved safety/design issue; do not connect simultaneous external power and ordinary USB until isolation is resolved.**
- **PWM outputs:** Two open-collector NPN transistor drivers
- **Power architecture:** A 12 V supply feeds separate branches for (1) the HDD backplane through the shunt, (2) the fans, and (3) the TRACO converter. Common ground. The INA228 excludes fan and controller consumption.

**Critical:** The shunt must be installed *in series* with the HDD backplane's positive 12 V supply; a parallel voltage tap alone cannot measure current. Confirm the enclosure's actual wiring before modifying it.

## Repository layout

```text
hardware/
  fan-controller.kicad_pro   Controller KiCad project
  fan-controller.kicad_sch   Schematic
  fan-controller.kicad_pcb   PCB layout
  fan-controller.kicad_dru   Custom DRC rules
  libraries/                 Project-local FanController libraries and models
firmware/                   RP2350 firmware (future)
docs/                       Design verification and history
mechanical/                 Mechanical design (future)
```

## Getting started in KiCad

1. Clone the repository locally.
2. Open `hardware/fan-controller.kicad_pro` in KiCad 10 to work on the existing controller project.
3. The separate thermistor-strip project was retired; the controller schematic still contains its original multiplexer/remote-sensor circuitry pending an independent electrical redesign.
4. Custom symbols and footprints use project-local `FanController` libraries. Confirm the electrical design before fabrication.

KiCad is a **local desktop editor**; GitHub stores and versions the files, rather than opening them as an editable online project.

## Design checks before PCB fabrication

- Verify XIAO RP2350 pin assignment, power input, and simultaneous USB/external 5 V behavior.
- Confirm the 12 V backplane wiring, 10 A current-path copper and connectors, branch protection, and Kelvin shunt routing.
- Confirm actual fan-header mating and pinout, reset-safe PWM behavior, and tachometer voltage.
- Verify the HC4051 multiplexer, seven-wire remote sensor-strip interface, thermistor excitation, accuracy, and NTC footprints.
- Validate footprints and mechanical clearance against the custom rear panel.

## Status

**Initial XIAO RP2350 controller schematic under review — freeze blocked; PCB layout pending.** See the [verification report](docs/controller-schematic-verification.md) before editing hardware.
