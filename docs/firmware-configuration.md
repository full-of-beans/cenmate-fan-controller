# Firmware configuration schema 1

Operating settings comprise mode (`AUTO` factory default or `FIXED`) and two integer duties (0–100%, both factory default 100%). By default a mode or duty command saves all three together with the **last saved general configuration**. `persist: false` leaves the previous record intact. General edits are transactional RAM patches until explicitly saved. `config_save` saves the general configuration with the **last saved operating settings**. `config_reload` restores general configuration only. `config_reset` applies factory general and operating settings, then saves them together. A flash failure reports an error while leaving the RAM change active. With no valid record, the values shown as saved are factory defaults and `saved_record_valid` is false.

The following are the exact flat keys under `config`. Numeric ranges are inclusive. Flag and count fields, PWM percentages and timing use JSON integers. For thermistor and tach entries, the two position-prefixed fields are separately configurable with the same defaults.

| Category | Key | Default | Accepted range and units |
| --- | --- | ---: | --- |
| Electrical | `shunt_ohm` | 0.001 | 0.0001–0.01 Ω |
| Electrical | `current_gain` | 1 | 0.5–2× |
| Electrical | `current_offset_a` | 0 | −2–2 A |
| Electrical | `voltage_gain` | 1 | 0.9–1.1× |
| Electrical | `voltage_offset_v` | 0 | −0.5–0.5 V |
| Electrical | `ina_calibrated` | 0 | 0–1 commissioning flag |
| Thermistors | `upper_r0_ohm`, `lower_r0_ohm` | 10000 | 1000–100000 Ω |
| Thermistors | `upper_t0_c`, `lower_t0_c` | 25 | 0–50 °C |
| Thermistors | `upper_beta_k`, `lower_beta_k` | 3380 | 2000–6000 K |
| Thermistors | `upper_divider_ohm`, `lower_divider_ohm` | 10000 | 1000–100000 Ω |
| Thermistors | `upper_offset_c`, `lower_offset_c` | 0 | −15–15 °C |
| Thermistors | `upper_adc_gain`, `lower_adc_gain` | 1 | 0.95–1.05× |
| Thermistors | `upper_ntc_calibrated`, `lower_ntc_calibrated` | 0 | 0–1 commissioning flag |
| Tach | `upper_ppr`, `lower_ppr` | 2 | 1–8 pulses per revolution |
| Power inference | `power_detection_validated` | 0 | 0–1. Set to 1 only after characterization. |
| Power inference | `absent_bus_v` | 1 | 0.1–5 V |
| Power inference | `present_bus_v` | 9 | 6–15 V |
| Power inference | `off_current_a` | 0.02 | 0–2 A |
| Power inference | `on_current_a` | 0.1 | 0.001–5 A |
| Power inference | `power_debounce_ms` | 1000 | 200–30000 ms |
| Fan startup | `startup_grace_ms` | 5000 | 1000–60000 ms after inferred ON before stall classification |
| Fan startup | `startup_boost_ms` | 0 | 0–60000 ms of full AUTO duty after inferred ON |
| AUTO | `auto_fallback_pwm` | 100 | 50–100% in eligible ON conditions. Characterize before lowering. |
| Stall | `stall_min_pwm` | 0 | 0–100%. Zero disables confirmed-stall classification. Configure after measuring starting duty. |
| Stall | `stall_confirm_ms` | 3000 | 2000–30000 ms of continuously confirmable missing rotation |
| Thermal protection | `hdd_target_c` | 35 | 15–39 °C. Reference target for later modeling. |
| Thermal protection | `hdd_emergency_c` | 40 | 25–60 °C |
| Thermal protection | `plenum_limit_enabled` | 0 | 0–1. Set to 1 only with a measured limit. |
| Thermal protection | `plenum_emergency_c` | 0 | 0–100 °C, at least 15 °C when enabled |
| Thermal protection | `recovery_hysteresis_c` | 2 | 0.5–10 °C below the active emergency threshold |
| Thermal protection | `recovery_ms` | 3000 | 200–60000 ms dwell after protective faults clear |
| Host | `host_expected_ms` | 60000 | 1000–300000 ms. Reported update cadence for the producer. |
| Host | `host_fresh_ms` | 90000 | 1000–600000 ms |
| Host | `host_expire_ms` | 240000 | 2000–1800000 ms |

The validated falling current threshold must be below the rising one. Host thresholds require `expected ≤ fresh < expire`. HDD target must be at or below the emergency threshold minus hysteresis. Thermal and power thresholds are provisional. Calibration flags are operator declarations and should be cleared explicitly after a calibration parameter or hardware change until rechecked. Values initially use nominal electrical components and a provisional two-pulse tach convention. Configuration and calibration revisions count changes within a boot. Flash generation increments only when a different record is saved. Neither threshold nor calibration flags imply a valid autonomous model.
