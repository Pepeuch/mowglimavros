# ESC observations and wheel odometry

The external `esc_wheel_odometry` MAVROS plugin is the sole `/wheel_odom`
producer. It normalizes MAVLink COMMON `ESC_STATUS`/`ESC_INFO` and ArduPilot
legacy `ESC_TELEMETRY_*` into the internal
`/mavros/esc_wheel_odometry/esc_observation` topic (`EscObservation`, best effort,
depth 64). The bridge consumes this message without decoding MAVLink. Each field
has explicit validity; unavailable fields do not use sentinel values. Temperature
is decoded directly from COMMON centidegrees, avoiding the pinned MAVROS scaling
problem. INFO freshness is independent of STATUS freshness.

Canonical wheel motion stays in C++. The common differential core computes
`v=(left+right)/2` and `yaw_rate=(right-left)/track_width`, preserving existing
frames, covariance and twist-only output. Pose remains unknown; WHEEL_DISTANCE
does not introduce pose integration. Blade RPM uses magnitude. Legacy unsigned
ESC telemetry RPM never supplies wheel direction.

## Sources and clocks

`source` accepts `auto` (default), `wheel_distance`, `esc_status`, or
`ardupilot_legacy`. Auto chooses valid, configured, fresh sources in that order.
A first distance sample establishes a baseline. Source changes, disconnects,
measurement-clock resets and accepted configuration changes reset motion
references; a timer never republishes wheel motion. A WHEEL_DISTANCE clock
regression resets only its owner/baseline; a COMMON STATUS clock regression resets
only COMMON. An INFO clock regression invalidates that four-slot metadata group,
without losing STATUS RPM/current or other sources. Repeated reset INFO packets
cannot restore validity. Disconnects and ROS reception-clock rollback clear every
source. Independent producers retain their fresh data and motion references. Exact repeated measurement
stamps do not refresh data. Invalid inputs and stale sources stop motion output.

COMMON observations retain `sample_stamp_ns` from positive MAVLink `time_usec`
and `receipt_stamp_ns` from ROS reception. Distance velocity uses measurement
intervals, independently of reception jitter. The core rejects nonmonotone
measurement stamps. Freshness and timeouts use reception; odometry headers retain
the existing ROS reception-time convention. RPM #226 lacks a source timestamp,
so its legacy adapter uses reception time and genuine telemetry counter gating.

WHEEL_DISTANCE uses MAVROS `SystemAndOk`: valid framing and target SYSID, without
restricting the sender to the autopilot COMPID. An ESP32 encoder on the same
vehicle is supported. One live encoder component owns the distance baseline;
another can take over after expiry. Encoder messages never create ESC reports.

COMMON STATUS and INFO share a single component owner because their indices are
unique only within a component. `esc_component_id=-1` lets STATUS acquire the
owner; only genuinely new STATUS renews its lease. INFO enriches the current
STATUS owner and cannot acquire, renew or take over that lease. Before the first
STATUS, one component's INFO may be retained as candidate metadata. STATUS from
the same component can use that fresh candidate; first STATUS from another
component discards the candidate and becomes owner immediately, without waiting
for INFO expiry. A different STATUS sender is ignored until the established
STATUS owner expires. An accepted takeover clears only COMMON status, metadata
and pairing state. A configured component ID
restricts both message types. A separate WHEEL_DISTANCE component can continue
in parallel. Ownership freshness advances only on genuinely new STATUS, not INFO or repeated
frames; INFO retains its independent metadata freshness.

Wheel STATUS pairing uses `common_pair_max_skew_s` (default 0.25 seconds),
independently of the reception freshness timeout. The skew must be finite,
positive and no larger than `observation_timeout_s`. Both wheel measurements
must advance, including after a source transition; cached wheels cannot be paired
with a newly arrived wheel across that transition. This threshold does not apply
to slower INFO metadata, which retains its separate freshness/coherence checks.

## Configuration ownership

Mappings and geometry are installation inputs, not constants or firmware policies.
MowgliNext can provide these parameters at startup or through ROS parameter
services without changing the architecture. This repository does not modify
MowgliNext or synchronize parameters between its nodes automatically.

| Plugin parameter | Default | Meaning |
| --- | --- | --- |
| `left_esc_slot`, `right_esc_slot` | -1, -1 | Configured ESC wheel slots, 0..63; -1 disables |
| `left_wheel_index`, `right_wheel_index` | -1, -1 | WHEEL_DISTANCE indices, 0..15; -1 disables |
| `left_wheel_radius_m`, `right_wheel_radius_m` | 0, 0 | Physical radii; positive for RPM sources |
| `track_width_m` | 0 | Positive width required for every wheel source |
| `left_esc_rpm_to_wheel_ratio`, `right_esc_rpm_to_wheel_ratio` | 1, 1 | COMMON motor-to-wheel factor, finite and nonzero; negative for configured orientation |
| `left_rpm_instance`, `right_rpm_instance` | -1, -1 | Distinct RPM #226 fields 1/2 for legacy |
| `expected_esc_telem_mav_offset` | -1 | Legacy enabled only with validated offset 0 |
| `esc_component_id` | -1 | One live COMMON STATUS/INFO sender; 0..255 restricts both handlers |
| `common_pair_max_skew_s` | 0.25 | Maximum measurement-time skew of wheel STATUS pairs, <= freshness timeout |
| `wheel_distance_component_id` | -1 | Any live target-system encoder; 0..255 restricts sender |
| `max_distance_speed_mps` | 10 | Positive discontinuity guard, installation configurable |
| `observation_timeout_s` | 3 | Positive reception freshness timeout, at most 3600 seconds |

Unconfigured defaults preserve the existing safe-disabled wheel behavior; ESC
normalization remains active independently. Legacy slots are limited to 0..11.
Legacy RPM is already wheel-scaled by the FCU: COMMON conversion factors do not
apply to it. Distinct slot/index mappings, finite nonnegative geometry and valid
frames/covariance are checked. Zero geometry disables the corresponding source.
Atomic parameter updates validate the entire candidate before applying it;
rejection preserves the previous configuration and motion references. Accepted
updates preserve compatible normalized ESC reports but reset wheel references.
Changing the COMMON component restriction discards reports from the old excluded
component. No cross-node configuration service is introduced.

Bridge role parameters are `right_esc_slot=0`, `left_esc_slot=1`,
`blade_esc_slot=2` for compatibility. They accept distinct 0..63 slots or -1 to
disable a role, including runtime atomic remapping. Configure the bridge roles
and plugin wheel slots consistently; their parameter services are independent.
No decoder assumes that every firmware assigns physical roles to slots 0/1/2.

Public Mowgli messages are unchanged. With fresh blade RPM/current but unknown
or stale INFO temperature, blade telemetry remains available and public
`mower_esc_temperature` is NaN because the existing public schema lacks a validity
flag. The internal message uses `temperature_valid=false`; fresh INFO restores
a valid temperature. `/diagnostics` reports active source and ESC field validity.

## Validation boundary

Deterministic tests cover retained legacy behavior, COMMON signs/scaling,
distance deltas and measurement clocks, source fallback, invalid data and resets.
The native ROS test runs pinned MAVROS and the bridge with synthetic decoded
frames, including different COMMON/encoder COMPIDs, foreign sender rejection,
source-isolated clock resets, parameter restrictions, disconnect and reconnect. It does not use an FCU or move actuators. Passive Rock/Pixhawk evidence
currently shows legacy RPM/ESC telemetry, without COMMON messages in the observed
window. Physical mappings, calibration, signs and distance accuracy remain bench
acceptance items; COMMON ingestion alone does not validate a firmware runtime.
