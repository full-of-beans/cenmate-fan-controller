# CENMATE 6-Bay HDD Enclosure Fan Controller

Custom cooling controller for a **CENMATE 6-bay USB HDD enclosure / DAS (JBOD)**.

## Purpose

Replace or supplement the enclosure's cooling arrangement with two independently controlled **ARCTIC P8 Max 80 mm 4-pin PWM fans**. Monitor two local rear-plenum temperatures (with optional NAS SMART telemetry), read fan tachometers, and measure the HDD backplane's **12 V input current** separately from the fans and controller power.

The controller schematic is in development in PR #3. **The existing KiCad schematic still implements the older eight-thermistor design; the simplified two-thermistor architecture and USB power plan are proposed documentation-only changes, not implemented hardware.** The [controller schematic verification](docs/controller-schematic-verification.md) records the reviewed revision, verified connections, unresolved electrical issues, layout constraints, and the seven-wire sensor-strip interface. The controller is not ready to freeze or fabricate.

## Planned hardware

- **Controller:** Seeed Studio XIAO RP2350
- **Fans:** 2 × ARCTIC P8 Max, independent PWM and tachometer
- **Proposed temperature sensing:** 2 × onboard NTC thermistors in the rear plenum, connected to separate ADC pins; optional six-drive SMART temperature reports from NAS; remove the remote strip and HC4051 in a future owner-approved KiCad edit.
- **Current/power monitor:** TI INA228AIDGSR, I²C, measuring a 0.2 mΩ Bourns CSS2H-3920R-L200F shunt
- **Proposed controller supply:** retain TRACO TSR 0.5-2450 (12 V to 5 V) and enable USB 5 V via a properly isolated two-source power path. Proposed external USB-C receptacle on carrier plus short four-conductor USB 2.0 pigtail plugged into the XIAO's existing USB-C receptacle; verify before implementation.
- **PWM outputs:** Two open-collector NPN transistor drivers
- **Power architecture:** A 12 V supply feeds separate branches for (1) the HDD backplane through the shunt, (2) the fans, and (3) the TRACO converter. Common ground. The INA228 excludes fan and controller consumption.

**Critical:** The shunt must be installed *in series* with the HDD backplane's positive 12 V supply; a parallel voltage tap alone cannot measure current. Confirm the enclosure's actual wiring before modifying it.

## Repository layout

```text
hardware/
  controller/         Main controller KiCad project
  thermistor-strip/   Legacy six-sensor strip KiCad placeholder (proposed to abandon)
  libraries/          Project-specific KiCad symbols/footprints if needed
firmware/             RP2350 firmware (future)
docs/                 Schematic verification and design evidence
mechanical/           Mechanical references and STEP exports (future)
```

## Getting started in KiCad

1. Clone the repository locally.
2. Open `hardware/controller/controller.kicad_pro` in KiCad 10 to work on the existing controller project.
3. The `hardware/thermistor-strip/` project is a historical placeholder; do not develop it unless the owner reverses the simplified thermal-sensing decision.
4. Follow the verification report and obtain owner approval before hardware changes; do not recreate either project over its existing files.

KiCad is a **local desktop editor**; GitHub stores and versions the files, rather than opening them as an editable online project.

## Design checks before PCB fabrication

- Verify XIAO RP2350 pin assignment, power input, and simultaneous USB/external 5 V behavior.
- Confirm the 12 V backplane wiring, 10 A current-path copper and connectors, branch protection, and Kelvin shunt routing.
- Confirm actual fan-header mating and pinout, reset-safe PWM behavior, and tachometer voltage.
- Proposed redesign: remove HC4051/J5/remote strip, route TH1 and TH2 to distinct verified ADC pins with individual divider components, and test rear-plenum vs hottest-drive SMART temperatures.
- Validate footprints and mechanical clearance against the custom rear panel.

## Status

**Controller schematic under review — freeze blocked; PCB layout pending. The current KiCad file predates the documented simplification and USB power-path proposal.** See the [verification report](docs/controller-schematic-verification.md) before editing hardware.
