# Hardware manufacturing revision register

This table binds the serial prefix printed or assigned to a physical controller to an immutable KiCad source and fabrication file set. **Do not use a moving branch name as the manufacturing identity.** Subsequent documentation commits and merge commits are not replacements for the source commit.

| Serial prefix | Hardware revision | KiCad/manufacturing source commit | Production archive | Archive SHA-256 | Fabrication order | Status |
| --- | --- | --- | --- | --- | --- | --- |
| **CMFC001** | First ordered PCB revision | [`513063f552a1c78585b386359e5f215d1ef3b81e`](https://github.com/full-of-beans/cenmate-fan-controller/tree/513063f552a1c78585b386359e5f215d1ef3b81e) | [`hardware/production/fan-controller.zip`](https://github.com/full-of-beans/cenmate-fan-controller/blob/513063f552a1c78585b386359e5f215d1ef3b81e/hardware/production/fan-controller.zip) at that exact commit | `d813421abf6412e8a27bcb29b3d2a362c4689602b35062833aafb3a7e9a9b420` | 15 bare PCBs + 1 top-side stencil, JLCPCB; order ID not recorded | Ordered 2026-11-20; assembly and commissioning unverified |

## CMFC001 manufacturing definition

- Source project at the linked commit: `hardware/fan-controller.kicad_pro`, `hardware/fan-controller.kicad_sch`, `hardware/fan-controller.kicad_pcb`, `hardware/fan-controller.kicad_dru`, plus project-local `hardware/libraries/`.
- Fabrication deliverable: the **committed ZIP at the linked source commit**, not an archive regenerated from a later branch HEAD. Its contents are Gerber and Excellon drill streams for the 30 × 80 mm two-layer controller.
- Order specification: 1.6 mm FR-4, 1 oz copper, green mask, lead-free HASL, 15 bare boards; separate frameless top-side 0.12 mm paste stencil. Supplier CAM changes, if any, must be recorded before claiming byte-identical fabrication provenance.
- Design status: bare-board prototype ordered, **not** validated assembly, enclosure fit, firmware, electrical commissioning, or unattended operation.
- Remaining evidence to attach: JLCPCB order number; any JLCPCB processed/modified CAM files; stencil production-file approval; serial allocation and first-article test results.

The archive SHA-256 above was calculated from the 56,103-byte committed ZIP retrieved at the immutable CMFC001 source commit. The Git blob SHA-1 for that archive is `8aefa5ea92ca91c0b78126247827e2590f937d41` (a different hash scheme).

## Identity and future revisions

- Assign physical serials `CMFC001-001` through `CMFC001-015` only after confirming the received boards correspond to this order. Record rejects and unassembled boards rather than reusing their numbers.
- A documentation-only cleanup, PR merge or firmware update **does not** increment CMFC001.
- If the KiCad schematic, PCB, libraries affecting fabrication, or manufacturing ZIP changes for a future board order, create a **new prefix** (for example CMFC002) and a new row with an immutable commit and archive hash.
- After merging to `main`, optionally add the merge commit as a *documentation reference* in a separate column; retain the original CMFC001 source SHA as the authoritative build identity. A merge commit can include documentation changes not present when the order was submitted.

## Manufacturing verification

- The CMFC001 source commit and production archive checksum above identify the ordered fabrication release. Retain any supplier CAM evidence and approval records with the order documentation.
- Outstanding prototype qualifications (connector ratings, actual wiring and mechanical fit, staged power-on, fan PWM/tach behavior, current measurement, temperature calibration, and loaded enclosure testing) are **post-fabrication validation**, not reasons to relabel the ordered artwork.
