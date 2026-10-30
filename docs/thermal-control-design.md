# Thermal-control architecture and simulation plan

Status: **proposal, documentation only; no firmware, simulation code, KiCad changes or fabrication approval**. Date: 2026-10-29.

## Hardware authority
Repository: full-of-beans/cenmate-fan-controller. Hardware [PR #3](https://github.com/full-of-beans/cenmate-fan-controller/pull/3) is open and unmerged. This documentation PR targets main independently. PR #3's description still references six remote NTCs and an HC4051; the owner's newer plan removes the strip and uses two local thermistors. Verify the evolving schematic before implementing. Intended board envelope **30 × 100 mm**, possibly shorter. Two PWM/tach channels, XIAO RP2350, INA228 measuring the HDD supply path; confirm whether the measured power excludes fans and controller. Existing electrical safety blockers in PR #3 remain unresolved.

## Agreed requirements
- Standalone control using HDD-path power, two local thermistors (TH1/TH2), and fan RPM. No NAS dependency.
- Optional NAS SMART per-drive temperatures and spin states are **authoritative upon receipt**, with negligible transport lag. Updates are event-driven after a completed successful NAS temperature-sampling pass, with periodic fallback for non-temperature telemetry. Each valid reading corrects that drive's model state; predict conservatively between updates. Preserve per-drive history when readings are absent; do not wake standby disks for telemetry.
- Control from conservative per-drive temperature predictions, not optimistic estimates. Temperature may decline during cooling; uncertainty from unknown power allocation grows without observations. No probabilistic SMART measurement-error model is required.
- Nominal target ~35°C. If any valid SMART temperature reaches 40°C, **both fans 100%**. Whether a conservative predicted 40°C triggers the same override is **open**; evaluate with a fail-safe bias.
- Retain two independent PWM outputs, but whether speeds should be synchronized or differential is **open**. Test fan pressure interaction and maximum differential; do not assume the slower fan consumes more power.
- **Fan curve vs PI remains open pending simulation**. Do not choose production gains or curves yet.

## Nine-state coupled RC thermal model
States: six HDD temperatures, one shared enclosure-air temperature, and two separate local thermistor-region temperatures. Ambient is an external boundary, not assumed equal to TH1 or TH2. Illustrative HDD equation:

```text
Ci*dTi/dt = Pi - Gi(PWM1,PWM2,RPM1,RPM2)*(Ti-Ta)
            + sum_j Gij*(Tj-Ti)
```

Include energy-balanced air and thermistor equations, thermal capacitances, airflow-dependent exchange, sensor lag, and potentially PCB self-heating. Fit coefficients experimentally. A 6×2 fan-influence map is an initial parameterization, not a measured or necessarily linear/normalized airflow matrix. Both fans share a plenum and may interact.

## Conservative observer
Total measured power does not reveal six independent HDD heat sources. Account for measured supply boundary, non-drive overhead, per-drive plausible idle/active/standby power, available spin states, cooling history and fan speeds. Begin with representative feasible allocations (uniform, concentrated in either fan region, least-cooled bay, mixed); propagate a temperature trajectory for each scenario and drive. A handful of scenarios is **not a guaranteed upper bound**. Later test constrained worst-case allocation optimization and explicit model-error allowances. TH1/TH2 may constrain localization but cannot uniquely identify six heat sources. Spun-down drives retain thermal history.

## Passive NAS workload telemetry and adaptive learning
The NAS is an **observer/publisher only**: do not add a high/low I/O classifier, thermal lag, or an EMA of workload. Resolve each HDD by the existing approved stable serial/bay mapping and read Linux `/sys/block/<device>/stat` busy-time deltas without causing disk I/O. Sample counters about every 10 seconds, aggregate the **actual busy milliseconds / elapsed milliseconds** over each reporting interval (initially 30 seconds), and report `io_busy_pct` (0–100), interval length, and observation timing. This is host block-device busy time, not measured watts or a definitive random/sequential classification. Invalid, missing, reset or stale counters mean **unknown**, not zero. A drive can perform internal work without host I/O.

The controller owns workload-to-power inference and all thermal inertia. It combines per-drive raw interval busy fraction, existing spindle/power state, and `smart_test` (`active`, `inactive`, `unknown`) with INA228 aggregate supply power, TH1/TH2, tach/PWM, and intermittent SMART temperatures. **Do not force Linux busy percentage to 100% for SMART tests**: retain a separate test flag because self-tests can consume power with no host I/O. Use a standby-safe firmware self-test execution-status observation to report `active` or `inactive` only when positively established; otherwise report `unknown`. Do not require the NAS or public Linux companion to distinguish short versus long. The private NAS may reuse its existing self-test lifecycle observations, but the controller-facing protocol must not depend on maintenance request accounting. Initial per-model power priors may use the commissioned Seagate ST6000NM0024-1HT17Z and Toshiba MG04ACA600E specifications, but manufacturer whole-drive 5V+12V power figures are not directly equivalent to measured enclosure-input 12V power. Learn bounded drive-specific heat/power parameters passively during informative workload variations; simultaneous correlated activity cannot uniquely identify each drive's power. On drive replacement reset drive-specific learned parameters, retaining enclosure airflow estimates provisionally. Persist calibration sparingly and safely; retain conservative defaults and hard safety overrides. Sustained self-test activity is a natural thermal-learning event, not a reason to schedule additional tests. The controller can account for the **duration** of an active self-test and its aggregate power/temperature evidence: short tests generally contribute little accumulated heat, while prolonged tests can materially affect the thermal state. A binary active flag is an input heat-generation signal, **not** an immediate temperature jump; the RC model supplies thermal inertia. Avoid treating `unknown` as inactive.

### Public Linux companion compatibility
The public controller protocol should require only `active` / `inactive` / `unknown` for SMART self-test status, rather than short/long attribution. A portable minimal Linux companion can read firmware execution status with a bounded, standby-guarded `smartctl` capabilities query (`-d sat -n standby,3,5 -c --json` for validated SATA/SAT bridges), subject to real-hardware verification; skip unsupported or sleeping devices and report `unknown` rather than waking them. Do not infer current test type from the historical SMART self-test log. Keep private NAS implementation details out of the public companion and expose only the common protocol. No companion code is introduced by this PR.

### Event-driven delivery contract (proposed, not implemented)
`nas-config` already has `nas-hdd-temperature.timer`/`nas-hdd-temperature.service`, `/usr/local/libexec/nas-hdd-temperature-sample`, and the volatile `/run/nas-disk-monitoring.json` cache with separately timestamped temperature and spindle observations. After the **completed sampling pass** commits its valid temperature updates, request **one asynchronous, bounded, coalesced** telemetry publish (not one USB send per disk). A separate 60-second periodic fallback sends current spindle/I/O/self-test state and newly valid temperatures, if any. Both paths use the same serialized publisher. The publisher reads the latest committed observations and only attempts USB delivery when enclosure power/online state and controller availability permit it. Failed/blocked USB sends must never fail or delay SMART sampling, acquire storage-lifecycle authority, wake a standby HDD, or affect enclosure policy. Exact trigger mechanism, existing publisher locks, and self-test status projection must be verified against code before implementation.

Send a single six-bay snapshot containing stable drive identity/bay, `spindle_state` (existing NAS vocabulary), `io_busy_pct` (nullable when unknown), `io_window_s`, `smart_test`, and **only fresh SMART temperatures** from the just-completed sampling pass. Omit stale cached temperatures or explicitly mark them stale; a fallback snapshot must not relabel old readings as fresh. Include message sequence/timestamp and observation timing sufficient to reject duplicates, delayed and out-of-order data; per-drive `temperature_age_s` is **not required** for freshly triggered measurements. If individual drives were skipped or failed sampling, mark their temperatures unavailable rather than repeating them as new. Handle sequential per-drive sampling timestamps accurately even if a single snapshot is sent. No new persistent telemetry database or general monitoring framework.

## Current filtering
Compare minimally filtered measured power entering the RC plant against a fast EMA (~3 s) and slow EMA (~30 s), sampled initially at ~1 Hz. EMA: Pf[k]=Pf[k-1]+(1-exp(-dt/tau))*(P[k]-Pf[k-1]). Fast-minus-slow can indicate transients but cannot uniquely identify spin-up. Avoid double-filtering: the plant itself has thermal inertia. Electrical protection must not be delayed by thermal filtering. All times are illustrative pending calibration.

## Candidate fan controllers — all undecided
1. Synchronized fan curve with hysteresis, minimum PWM and ramp limits.
2. Two-zone independent fan curves or PI.
3. Influence-weighted coupled allocation based on calibrated 6×2 fan effects.
4. Model predictive control as a higher-complexity experiment.

Compare fan curve vs PI and synchronized vs independent as **separate dimensions**. PI needs anti-windup and smooth transitions; a curve may settle above the nominal target. Explore PWM differential constraints (20 percentage points is only an example, not a validated limit), tach feedback, shared-pressure interaction, minimum reliable RPM, and both-fan full-speed safety override. Confirm hardware fail-safe when PWM/MCU fails; do not assume.

## Simulation and calibration
Use free/open-source Python, NumPy, SciPy, python-control where useful, and Plotly Dash for interactive tuning. OpenModelica and Scilab/Xcos are optional. No code in this PR. Simulator exposes only local sensors and optional SMART to the controller; true simulated HDD temperatures are for scoring only.

Simulate six independent HDD workloads and spin states, short spin-up pulses, ambient variation, fan nonlinearities/cross-coupling, sparse and absent SMART, missing drives, sensor/tach faults, and model mismatch. Plot actual and estimated temperatures, TH1/TH2, raw/filtered power, PWM/RPM, overshoot, time above thresholds, speed variation, and energy/noise proxies.

Calibrate from measured ambient, SMART, TH1/TH2, HDD-path power and RPM: fixed-fan steady states, fan-1-only steps, fan-2-only steps, both-fan steps, and varied HDD loads/spin states. Fit thermal coefficients with SciPy least squares and validate on held-out runs. Measure individual fan current separately if investigating electrical interaction; HDD-path current sensing may exclude fans.

Define quantitative acceptance criteria before firmware: peak temperatures, conservatism versus false full-speed activations, response to load, stability, minimum RPM, loss of USB, fan stall, and resource usage.

## Explicit open decisions and milestones
Open: fan curve vs PI, fan allocation topology, fan speed differential, 40°C predicted-temperature override, estimator conservatism, model coefficients, current-filter time constants, minimum PWM/ramp, host message serialization/details, ambient sensing, and final thermistor placement/PCB length.

Next: (1) confirm hardware baseline after PR #3 evolves; (2) implement nine-state simulator in a separate PR; (3) gather physical calibration data; (4) compare observers/controllers; (5) decide safety thresholds and production controller; (6) implement and validate firmware separately.

This document records decisions and experiments; it does **not** modify PR #3 or authorize fabrication.
