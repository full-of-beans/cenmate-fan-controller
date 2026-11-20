# CENMATE 6-Bay HDD Enclosure Fan Controller

Custom cooling and power-monitoring controller for a **CENMATE six-bay USB HDD enclosure (JBOD/DAS)**.

> **CMFC001 prototype boards ordered (2026-11-20).** This is an engineering prototype, not a qualified assembled product. See [hardware revision register](docs/hardware-revisions.md) for the immutable KiCad/manufacturing source and order status. Historical audits may describe superseded revisions.

## Current design

- **Controller:** Seeed Studio XIAO RP2350, powered from the enclosure's USB-A 5 V supply through its USB-C connector. There is **no onboard TRACO 12 V-to-5 V converter** and no external 5 V connection to the module's VBUS pin. USB availability also provides the fan-switch enable signal.
- **Fans:** Two independently controlled 12 V, four-wire PWM fans, with separate PWM drive and tachometer inputs. The intended physical fan headers are [MODDIY 2.54 mm right-angle 4-pin male fan connectors](https://www.moddiy.com/products/5408/4-Pin-3-Pin-2.54mm-Pitch-2510-Fan-Male-Angled-Connector-Black.html). **The PCB still uses Molex right-angle header footprints/3D models as placeholders; the MODDIY parts have not been physically verified against those holes or pin orientation.** Do not purchase/install the Molex parts based on the current CAD representation.
- **Fan power:** DMP3056L-7 P-channel MOSFET switches the 12 V fan branch; its gate is controlled by the enclosure rocker via J5 and defaults off if the switch lead opens. PWM duty mapping and safe startup/reset behavior require firmware and bench verification.
- **Temperature sensing:** Two on-board 10 kΩ NTC thermistors, each read on its own XIAO ADC input through a 10 kΩ pull-up. There is **no remote thermistor strip or analog multiplexer**.
- **Current monitor:** TI INA228 on I²C, sensing the HDD backplane's positive 12 V feed through a **1 mΩ Bourns shunt**. The fan supply branches before the shunt, so the shunt measures the HDD branch rather than fan current. Accuracy and Kelvin/landing resistance must be checked by calibration.
- **HDD power path:** Center-positive 12 V barrel input **J1** → shunt **RSH1** → Phoenix Contact two-position screw terminal **J2** → HDD enclosure. **J2 pin 1 = +12 V, pin 2 = GND.** The PCB has explicit `12V` and `GND` silkscreen markings beside J2; verify wiring polarity before connection.
- **Grounding:** Common system ground, with a two-layer PCB and ground copper fill.

The enclosure's power brick is labeled **12 V / 9 A**. The selected **Tensility 54-00286 barrel jack is rated 7 A**; the owner has explicitly accepted the prototype risk rather than replacing it. This does **not** establish that six-drive startup and operating loads meet the connector rating. Measure the actual current, contact voltage drop, and connector heating during controlled bring-up.

## Hardware and mechanical status

The board and custom backplate have been checked for fit by the owner in Onshape. This does not independently verify fabricated edge reliefs, actual mated MODDIY connector geometry, cable bends, or as-built tolerances. Check physical fit before connecting the HDD enclosure.

The CMFC001 prototype PCB was ordered from JLCPCB as **15 bare 30 × 80 mm, two-layer, 1.6 mm FR-4, 1 oz copper, green-mask, lead-free HASL boards**, plus one top-side paste stencil. Actual manufactured fit, CAM interpretation, and assembly remain to be checked when boards arrive.

## Repository layout

```text
hardware/
  fan-controller.kicad_pro   KiCad 10 project
  fan-controller.kicad_sch   Current controller schematic
  fan-controller.kicad_pcb   Current PCB layout
  fan-controller.kicad_dru   Custom DRC rules
  libraries/                 Project-local symbols, footprints, and models
firmware/                   Controller firmware (as developed)
docs/                       Design notes, reviews, and historical verification
mechanical/                 Mechanical files (as available)
```

Open `hardware/fan-controller.kicad_pro` with **KiCad 10**. The project-local `FanController` libraries must resolve before editing or plotting. The retired remote-sensor-strip design and earlier TRACO-based architecture are **historical only**; do not follow them when building the current board.

## Ordered prototype and validation

CMFC001 manufacturing identity, source commit and archive path are recorded in [hardware revisions](docs/hardware-revisions.md). Do not regenerate the ordered ZIP as part of documentation cleanup.

Before connecting drives, perform continuity and polarity checks, current-limited initial power-up, USB/12 V power sequencing, fan enable/PWM/tach tests, INA228 communication and calibration, and thermistor checks. Only then test with the enclosure under controlled startup/full-load conditions while monitoring the accepted 7 A connector risk.

## Documentation provenance

[Controller schematic verification](docs/controller-schematic-verification.md) and earlier PR reviews are retained for engineering history; some sections describe **superseded architectures, parts and pin mappings**. For the ordered CMFC001 hardware, the immutable source commit in the hardware revision register takes precedence. Later branch or merge commits may contain documentation-only updates. Fan-control firmware strategy (fan curve versus PI) remains undecided pending modeling and comparison.
