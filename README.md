# CENMATE 6-Bay HDD Enclosure Fan Controller

Custom cooling and power-monitoring controller for a **CENMATE six-bay USB HDD enclosure (JBOD/DAS)**.

> **Prototype design in review.** The KiCad 10 schematic and PCB are implemented in the open [PCB PR #6](https://github.com/full-of-beans/cenmate-fan-controller/pull/6). Do not treat the design as production-qualified or order fabrication without checking the latest PR audit, ERC/DRC, manufacturing files, and mechanical fit. Older verification documents describe superseded revisions.

## Current design

- **Controller:** Seeed Studio XIAO RP2350, powered from the enclosure's USB-A 5 V supply through its USB-C connector. There is **no onboard TRACO 12 V-to-5 V converter** and no external 5 V connection to the module's VBUS pin. USB availability also provides the fan-switch enable signal.
- **Fans:** Two independently controlled 12 V, four-wire PWM fans, with separate PWM drive and tachometer inputs. The intended physical fan headers are [MODDIY 2.54 mm right-angle 4-pin male fan connectors](https://www.moddiy.com/products/5408/4-Pin-3-Pin-2.54mm-Pitch-2510-Fan-Male-Angled-Connector-Black.html). **The PCB still uses Molex right-angle header footprints/3D models as placeholders; the MODDIY parts have not been physically verified against those holes or pin orientation.** Do not purchase/install the Molex parts based on the current CAD representation.
- **Fan power:** TPS22810 high-side load switch controls the 12 V fan branch. Its EN is driven from USB VBUS, with a pull-down so fans are switched off when USB power is absent. PWM duty mapping and safe startup/reset behavior require firmware and bench verification.
- **Temperature sensing:** Two on-board 10 kΩ NTC thermistors, each read on its own XIAO ADC input through a 33 kΩ pull-up. There is **no remote thermistor strip or analog multiplexer**.
- **Current monitor:** TI INA228 on I²C, sensing the HDD backplane's positive 12 V feed through a **1 mΩ Bourns shunt**. The fan supply branches before the shunt, so the shunt measures the HDD branch rather than fan current. Accuracy and Kelvin/landing resistance must be checked by calibration.
- **HDD power path:** Center-positive 12 V barrel input **J1** → shunt **RSH1** → Phoenix Contact two-position screw terminal **J2** → HDD enclosure. **J2 pin 1 = +12 V, pin 2 = GND.** The PCB has explicit `12V` and `GND` silkscreen markings beside J2; verify wiring polarity before connection.
- **Grounding:** Common system ground, with a two-layer PCB and ground copper fill.

The enclosure's power brick is labeled **12 V / 9 A**. The selected **Tensility 54-00286 barrel jack is rated 7 A**; the owner has explicitly accepted the prototype risk rather than replacing it. This does **not** establish that six-drive startup and operating loads meet the connector rating. Measure the actual current, contact voltage drop, and connector heating during controlled bring-up.

## Hardware and mechanical status

The board and custom backplate have been checked for fit by the owner in Onshape. This does not independently verify fabricated edge reliefs, actual mated MODDIY connector geometry, cable bends, or as-built tolerances. Check physical fit before connecting the HDD enclosure.

The prototype PCB is intended for **JLCPCB standard two-layer, 1 oz copper, green solder mask**. The green-mask choice accommodates the nominal 0.12 mm solder-mask web between Q1/Q2 pads under the cited 0.10 mm green-mask limit; review the final manufacturer CAM preview. Existing silkscreen and exposed component/connector pads are accepted for prototype testing without adding dedicated test-point footprints.

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

## Prototype release and validation checklist

Before ordering:

1. Review the latest findings and owner dispositions in [PR #6](https://github.com/full-of-beans/cenmate-fan-controller/pull/6). Original audit: [pre-fabrication engineering review](https://github.com/full-of-beans/cenmate-fan-controller/pull/6#pullrequestreview-5802921394).
2. Run KiCad 10 schematic ERC and PCB DRC with schematic parity checks on the **current commit**, not just historical reports. Review all remaining warnings and exceptions.
3. Verify symbol/PCB footprint correspondence, BOM/manufacturer part numbers, and J1/J2 and fan-header pin mapping. The MODDIY header's actual lead pattern, orientation, mating and retention remain unverified.
4. Plot and inspect copper, solder-mask, silkscreen, Edge.Cuts, and drill files. Confirm cutout and mounting-hole geometry in the manufacturer's preview; select **green solder mask and 1 oz copper**.
5. Check assembly access and the physical backplate, fan, USB, barrel plug, and screw-terminal clearances.

Before connecting drives, perform continuity and polarity checks, current-limited initial power-up, USB-present/absent versus 12 V-present/absent sequencing, fan enable/PWM/tach tests, INA228 communication and calibration, and thermistor checks. Only then test with the enclosure under controlled startup/full-load conditions while monitoring the accepted 7 A connector risk.

## Documentation provenance

[Controller schematic verification](docs/controller-schematic-verification.md) and earlier PR reviews are retained for engineering history; some sections describe **superseded architectures, parts and pin mappings**. The active schematic/PCB on the latest PR commit and its current review/disposition comments take precedence. Fan-control firmware strategy (fan curve versus PI) remains undecided pending modeling and comparison.
