# Thermal-control architecture and simulation plan

Status: **proposal, documentation only; no firmware, simulation code, KiCad changes or fabrication approval**. Date: 2026-10-29.

## Hardware authority
Repository: full-of-beans/cenmate-fan-controller. Hardware [PR #3](https://github.com/full-of-beans/cenmate-fan-controller/pull/3) is open and unmerged. This documentation PR targets main independently. PR #3's description still references six remote NTCs and an HC4051; the owner's newer plan removes the strip and uses two local thermistors. Verify the evolving schematic before implementing. Intended board envelope **30 × 100 mm**, possibly shorter. Two PWM/tach channels, XIAO RP2350, INA228 measuring the HDD supply path; confirm whether the measured power excludes fans and controller. Existing electrical safety blockers in PR #3 remain unresolved.

## Agreed requirements
- Standalone control using HDD-path power, two local thermistors (TH1/TH2), and fan RPM. No NAS dependency.
- Optional NAS SMART per-drive temperatures and spin states are **authoritative upon receipt**, with negligible transport lag. Updates may be spaced 1–5 minutes. Each valid reading corrects that drive's model state; predict conservatively between updates. Preserve per-drive history when readings are absent; do not wake standby disks for telemetry.
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
Open: fan curve vs PI, fan allocation topology, fan speed differential, 40°C predicted-temperature override, estimator conservatism, model coefficients, current-filter time constants, minimum PWM/ramp, host message schema, ambient sensing, and final thermistor placement/PCB length.

Next: (1) confirm hardware baseline after PR #3 evolves; (2) implement nine-state simulator in a separate PR; (3) gather physical calibration data; (4) compare observers/controllers; (5) decide safety thresholds and production controller; (6) implement and validate firmware separately.

This document records decisions and experiments; it does **not** modify PR #3 or authorize fabrication.
