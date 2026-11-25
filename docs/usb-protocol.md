# USB protocol version 1

Open the XIAO USB-C CDC ACM port with DTR asserted and local echo disabled. The serial line coding does not affect USB throughput. Each frame is one strict UTF-8 JSON object followed by LF. CRLF works through JSON whitespace. Each request carries a unique outstanding unsigned 32-bit integer `id` and a string `cmd`. Send one request at a time and wait for its matching response. Telemetry event frames have an `event` key instead of `id`.

Requests are limited to 3072 bytes before LF. Oversized lines are discarded through LF. An incomplete line expires after 2 s of input inactivity. The parser uses a fixed ~36 KiB arena. Duplicate keys, unknown fields, invalid types and values outside the documented range are rejected on recognized commands. A response is held in an 8192-byte buffer and written only when the endpoint can accept bytes. The firmware applies endpoint backpressure to requests while a reply is pending and skips stream intervals while busy. A partial event already being sent can complete before a command response. Applications can detect skipped intervals using `stream_dropped`. Closing DTR, suspension and observed disconnect clear pending protocol state and turn streaming off. Reopen, request `get_info`, and re-enable streaming as needed.

## Requests and replies

All rows take `id` and `cmd`. The table lists additional fields. Success replies include `id` and `ok: true`.

| Command | Additional request fields | Additional success fields |
| --- | --- | --- |
| `get_info` | None | `hardware`, `board`, `version`, build `revision`, `protocol`, `schema_version`, `receive_limit`, `transmit_buffer` |
| `get_status` | None | `status` |
| `get_sensors` | Optional `format`: `RAW`, `PHYSICAL`, or default `FULL` | `sensors` |
| `set_mode` | `mode`: `AUTO` or `FIXED`. Optional `upper_pwm`, `lower_pwm`, `persist`. | `requested`, `persisted` |
| `set_pwm` | At least one of `upper_pwm`, `lower_pwm`. Optional `persist`. Active mode must be FIXED. | `requested`, `persisted` |
| `update_telemetry` | `drives`: array of 0–6 drive objects | `count` |
| `config_get` | None | `active`, `saved`, `active_operating`, `saved_operating`, `saved_record_valid`, `generation` |
| `config_set` | `schema_version: 1`, `config`: object of changed keys | `config_revision` |
| `config_save` | None | `persisted: true` |
| `config_reload` | None | No additional fields |
| `config_reset` | None | `persisted: true` |
| `stream_set` | `enabled`: boolean. Optional `format` and `interval_ms`: integer 250–60000, default 1000. | `enabled`, `interval_ms` |
| `clear_faults` | None | No additional fields |

PWM values are integers from 0 to 100 and omitted values retain their active value. `requested` gives mode and both duties. `persist` defaults to true. `clear_faults` clears latched storage and protocol diagnostics, including the boot-time corrupt configuration indication. Current sensor, thermal and lifecycle conditions are reevaluated normally. Watchdog reset reason is fixed for the boot.

Error replies have `id` (or `null` when the ID cannot be trusted), `ok: false`, `error` and `applied`. `applied` is false for validation errors and true for a storage failure after settings have taken effect in RAM.

| Error code | Cause |
| --- | --- |
| `message_too_large`, `partial_timeout` | Frame length or inactivity limit exceeded |
| `malformed_json`, `invalid_id` | Invalid JSON or missing/out-of-range numeric ID |
| `invalid_request`, `unknown_command`, `requires_fixed` | Schema validation failure, unknown command, or PWM command outside FIXED |
| `storage_failure` | Flash save failed. RAM settings remain active. |
| `parser_unavailable`, `response_too_large` | Internal buffer or formatting failure |

### Drive observations

`drive_id` is 1–48 ASCII characters from letters, digits, `.`, `_`, `:`, `-`. IDs must be unique in the message. `temperature_c` is −20 to 100 °C or null. A numeric temperature requires `temperature_age_ms`, an unsigned 32-bit integer measured at send time. Use omitted or null age for an unavailable temperature. `spindle` accepts `ACTIVE`, `STANDBY`, and `UNKNOWN`. `io` and `self_test` accept `YES`, `NO`, and `UNKNOWN`. Omitted or null states are unknown. At least one known state requires an unsigned `state_age_ms`. Group observations using the age of the oldest known state in the group. If all states are unknown, omit the age or use null. Send the full set each time. Invalid messages leave the previous set and its ages intact.

The controller adds monotonic time since receipt to each reported age. Temperature is represented by its last reported value alongside `temperature_freshness` of `MISSING`, `FRESH`, `AGING`, or `EXPIRED`. State fields have independent `state_freshness`. Host eligibility requires an open session and a fresh temperature for every reported drive. An adapter should use the individual observation age from the producer even if it publishes a new snapshot or reports a sleeping disk. Host eligibility is an observational field in this release.

### Cached telemetry

- `status` includes boot ID (random hexadecimal per boot), uptime in ms, requested settings, applied logical PWM in upper/lower order, output reason, AUTO policy and override flag, inferred power state and confidence, power-detection validation flag, configuration and calibration revisions, storage generation, stream drops, host link and freshness, drive details and diagnostic code/severity objects.
- `sensors` contains `ina` and `fans`, in upper/lower order. INA reports validity, calibration flag, sample time and error. RAW adds `config`, `adc_config`, `diag`, `bus_raw`, `shunt_raw`. PHYSICAL adds `bus_v`, `shunt_v`, `current_a`, `power_w`. FULL includes both. The raw voltage registers include their reserved low nibble. Refer to INA physical telemetry to interpret the power-state inference.
- Each fan contains `plenum` and `tach`. Plenum supplies validity, calibration, time and error, RAW `adc_count`, and PHYSICAL `ohm` and `temperature_c`. Tach supplies validity, configured `ppr`, time and last pulse age. RAW adds `pulses` and `window_ms`, and PHYSICAL adds `rpm`.
- Observation times `at_ms` are uptime. The raw values may remain available if the physical sample is invalid. Missing values are null. Zero RPM with a valid time window means zero counted pulses. Plenum and INA physical samples expire at 350 ms and tach windows at 2500 ms.
- Stream events contain `event: "telemetry"`, `protocol`, `version`, `sequence`, `status`, and `sensors`. The sequence counts emitted snapshots. Dropped intervals are reflected separately. Status and sensor queries return cache data without triggering device I/O.

### Examples

Send each line separately and wait for its ID as needed.

```jsonl
{"id":1,"cmd":"get_info"}
{"id":2,"cmd":"set_mode","mode":"FIXED","upper_pwm":40,"lower_pwm":60}
{"id":3,"cmd":"set_pwm","upper_pwm":20,"lower_pwm":30,"persist":false}
{"id":4,"cmd":"get_sensors","format":"FULL"}
{"id":5,"cmd":"update_telemetry","drives":[{"drive_id":"bay.1","temperature_c":34.5,"temperature_age_ms":1200,"spindle":"ACTIVE","io":"NO","state_age_ms":300},{"drive_id":"bay.2","temperature_c":null,"spindle":"STANDBY","state_age_ms":300}]}
{"id":6,"cmd":"config_set","schema_version":1,"config":{"upper_ppr":2,"lower_ppr":2}}
{"id":7,"cmd":"config_save"}
{"id":8,"cmd":"stream_set","enabled":true,"interval_ms":1000,"format":"FULL"}
{"id":9,"cmd":"stream_set","enabled":false}
{"id":10,"cmd":"set_mode","mode":"AUTO"}
```

```jsonl
{"id":2,"ok":true,"requested":{"mode":"FIXED","upper_pwm":40,"lower_pwm":60},"persisted":true}
{"id":3,"ok":true,"requested":{"mode":"FIXED","upper_pwm":20,"lower_pwm":30},"persisted":false}
{"id":7,"ok":true,"persisted":true}
{"id":8,"ok":true,"enabled":true,"interval_ms":1000}
{"id":9,"ok":true,"enabled":false,"interval_ms":1000}
{"id":11,"ok":false,"error":"requires_fixed","applied":false}
```
