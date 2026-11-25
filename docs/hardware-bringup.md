# CMFC001 build and bench bring-up

## Reproduce the build

Use CMake 3.28 or newer, Python 3, Git, a native GCC with C++20 support (GCC 13 or 14), and Arm GNU Toolchain 13.3.Rel1 or the Ubuntu 24.04 ARM GNU 13 toolchain with ARM newlib and C++ runtime. On Ubuntu the cross packages are `gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib`. Pico SDK is pinned to 2.3.0 commit `98a542c1a62fb549ffb5d66a3e5892b06276b670`. The SDK builds picotool 2.3.0 from source if necessary. CMake fetches yyjson 0.12.0 by commit. A local yyjson checkout at the same commit can be supplied through `FETCHCONTENT_SOURCE_DIR_YYJSON`.

```sh
git clone --branch 2.3.0 --depth 1 https://github.com/raspberrypi/pico-sdk.git ../pico-sdk
git -C ../pico-sdk submodule update --init lib/tinyusb
cmake -S firmware -B firmware/build-native -DCMAKE_BUILD_TYPE=Debug
cmake --build firmware/build-native --parallel
ctest --test-dir firmware/build-native --output-on-failure
cmake -S firmware -B firmware/build-pico -DCMFC_PICO=ON -DCMAKE_BUILD_TYPE=Release \
  -DPICO_SDK_PATH="$(realpath ../pico-sdk)"
cmake --build firmware/build-pico --parallel
arm-none-eabi-size firmware/build-pico/cenmate.elf
```

For a standalone ARM toolchain directory, add its `bin` to PATH or point `PICO_TOOLCHAIN_PATH` at the extracted directory before configuring. The build emits `cenmate.uf2`, `cenmate.elf`, the linker map, and `build-metadata.txt`. Use a clean checkout to associate reported Git revision with the tested artifact. The Firmware workflow publishes the UF2, ELF, metadata and memory summary as `cenmate-cmfc001`.

With the enclosure and fans disconnected, hold **BOOT** on the XIAO while plugging in USB-C to enter the RP2350 USB bootloader. Copy the UF2 to the mounted RP2350 bootloader volume and allow it to finish and reboot. Use the CDC ACM device for [USB protocol](usb-protocol.md) commands. Check `get_info` for CMFC001, firmware revision and protocol version. The prototype shares the Pico CDC USB VID/PID. Match the reported hardware and device serial when multiple boards are present. BOOTSEL remains the firmware reflash path.

## Electrical checkout

1. Check assembly identity and continuity with all power disconnected. Verify Q1/Q2 transistor lead orientation, U2 and RSH1 fit and soldering, the J1/J2 connection and polarity, J5 enable leads, and actual MODDIY fan connector mating. Map J3 to the upper fan and J4 to the lower fan. Confirm each four-wire harness end to end in the order ground, 12 V, tach, PWM. Orient TH1 upper and TH2 lower in the installed enclosure. Treat the CAD fan-header fit as pending first-article qualification.
2. Power the controller by an independent USB host with J1 disconnected. Check U1 3.3 V and U2 VS, USB, `get_info`, and `get_sensors FULL`. Confirm INA 0x40 responds, ready and trim status is credible, raw readings are present, VBUS and no-load shunt voltage agree with a meter, and factory mode selects equal full duty. With 12 V still absent, read both thermistors at known room temperature and warm each channel individually to confirm orientation and sign. Compare against a nearby reference across multiple temperatures before setting calibration flags.
3. Use a current-limited supply for the first 12 V test. Verify rocker operation and fan supply first, then add known loads and compare J2 voltage and signed current against reference instruments at no load and multiple loads. Retain both raw and converted samples. Check the barrel connector drop and temperature before six-drive spin-up. The selected 7 A connector and prototype thermal limits remain to be qualified for the enclosure load.
4. Scope both fan PWM outputs on the connector with the fan pull-ups present. Use temporary FIXED settings and test 0%, intermediate duties and 100% separately per fan. Verify approximately 25 kHz and electrical inversion, pulse polarity, fan stop behavior, measured RPM and actual PPR against a scope or optical reference. Check reset/reflash with fan power present, including released PWM before the MCU initializes. Return to 100% AUTO when finished.
5. Test persistence with safe powered-fan duties or with fans disconnected. Save a FIXED pair, send a different temporary pair, reboot and check the saved pair returns. Change a calibration field without saving, save a new FIXED setting, reboot and confirm only the operating settings survived. Test `config_save`, `config_reload` and `config_reset`. On a spare bench unit, interrupt a flash save by removing USB power with loads disconnected. Reboot and compare with the preceding valid record. Reset to factory before returning the unit to general use.
6. With loads disconnected and SWD available, halt the core in the main loop for longer than 2 s to exercise watchdog recovery (watchdog debug pause is disabled). Confirm re-enumeration, a new boot ID, the `WATCHDOG_RESET` diagnostic and restored settings. Test temporary INA communication loss on an accessible bench fixture and verify recovery while USB and fan PWM continue.

### Power sequencing matrix

Repeat each supply and rocker state with enclosure upstream USB unplugged and connected. For each row first use only the enclosure's downstream USB-A to power the XIAO, then repeat with only independent USB power to the XIAO. Never join two independent USB 5 V sources to the controller.

| J1 12 V supply | Rocker | Measurements |
| --- | --- | --- |
| Disconnected | OFF | U1 USB/3.3 V, enclosure downstream 5 V, J2 voltage, shunt voltage and current, fan rail, PWM/tach pin voltages |
| Disconnected | ON | Same, especially any rail rise or current driven from USB |
| Connected | OFF | Same, plus USB data availability and whether fan rail floats |
| Connected | ON | Same, plus fan RPM, current during transitions, and time to resume a previously commanded FIXED duty |

Measure residual voltage and current in both powering directions. Independently power U1 with the fans unpowered and observe tach pull-up behavior and any fan-rail rise. Check fan PWM and tach when only the fan/enclosure supply is powered. Check for USB backfeed toward the enclosure and whether its downstream USB-A is powered with the rocker OFF. A rocker transition should not change commanded FIXED PWM when U1 remains powered. Record actual wiring, fan model, system states, scope traces and board serial. Resolve harmful backfeed, wrong polarity, watchdog issues or unexplained transients before fitting the controller to an operating enclosure.

## Characterization and roadmap

1. **Firmware foundation.** Verify the firmware and protocol on the bench with no drives. Establish stable instrumentation, units, sign, channel labels, freshness indicators, and known reset behavior.
2. **Prototype bring-up.** Complete electrical, connector, fan and power-sequencing checks above. Characterize each fan's starting duty, minimum sustainable duty, 0% behavior, PWM-to-RPM response, time to settle, tach accuracy, fan-to-fan variation, and restoration of fan power with duty already present. Separate no pulses from insufficient supply or low commanded duty.
3. **Thermal and power characterization.** Record time-aligned raw/engineering data, uncertainty, ambient test condition, board and drive identities, rocker/supply state, power, both plenum temperatures, both applied duties/RPM, and fresh per-drive temperatures. Capture disconnected supply, rocker OFF, ON without drives, all drives standby, single-drive and simultaneous spin-up, all drives spinning, representative I/O and shutdown. Look for stable nonoverlapping evidence before enabling power classification. Treat overlap as UNKNOWN. Use all-six representative worst-case workloads and equal PWM sweeps, then vary each fan separately. Include multiple active drive counts, standby transitions and startup/shutdown transients. Establish repeatable steady-state criteria, response times and cooling capacity with repeat runs.
4. **Validated autonomous AUTO.** Fit and assess conservative equal-duty predictions only after repeatable measurements across the intended operating envelope. Hold out workloads and ambient conditions, quantify underprediction and demonstrate margin and recovery. Confirm the worst-case operating envelope, sensor accuracy and protective thresholds before relying on the model.
5. **Host-assisted optimization and bounded learning.** Add host-aware optimization only after baseline AUTO is validated. Then test freshness loss, confidence decay, bounded corrections, independent safety margins and lifecycle suspension of learning.
6. **Production validation.** Repeat on multiple assemblies and approved fan/harness variants, verify fault coverage and prolonged operation, and qualify flash endurance, USB identity and update procedures.

For FIXED thermal runs, **external experiment software or a supervising operator owns temperature limits and aborts**. The provisional HDD target is 35 °C and the provisional abort point is 40 °C. Require fresh observations from every exposed drive during manual sweeps and an independent means of restoring cooling or ending the workload if the host or controller stops responding. Losing the experiment host can leave the controller at its last FIXED duty indefinitely. Begin with high duty and short tests, then extend only after verifying the logging and abort path.
