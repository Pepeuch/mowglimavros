# Firmware providers

The backend resolves `MAVROS_FIRMWARE` once before creating its two nodes. `auto`
is the default; explicit `ardupilot` and `px4` values bypass heartbeat detection.
Bootstrap metadata lives in `python/mowgli_mavros_bridge/firmware_provider.py`.
The selected concrete firmware is passed to the bridge through the private process
environment `MAVROS_RESOLVED_FIRMWARE`, without adding a ROS parameter or topic.

The C++ `FirmwareProvider` owns MANUAL_CONTROL conversion and command
capabilities. ArduPilot and PX4 share the existing
conversion and policy implementation; no native PX4 axis or emergency-mode change
is introduced by this extraction. Each concrete provider declares its identity.
A standalone bridge without the private environment retains its existing Rover
conversion. The bridge applies HOLD plus DISARM for Safety/E-stop.

| Capability / policy | ArduPilot provider | PX4 provider |
| --- | --- | --- |
| MAVROS profile | `apm` | `px4` |
| Heartbeat autodetection | Implemented | Implemented |
| MANUAL_CONTROL mapping | Existing steering `y`, throttle `z` | Same existing mapping preserved |
| Arm request path implemented | Yes | Yes |
| Disarm request path implemented | Yes | Yes |
| Mode request path implemented | Yes | Yes |
| Default emergency policy | `HOLD` + disarm | Existing `HOLD` + disarm preserved |
| MowerControl OFF | Native183 servo neutral, deduplicated | Unsupported (rejected) |
| MowerControl ON | Native183, guarded FWD/REV, already armed FCU required | Unsupported (rejected) |
| Physical actuation validated | No | No |

Capabilities describe implemented software paths; physical actuation still depends
on the configured FCU and actuators. Valid finite `/cmd_vel` input is mapped
to Rover `MANUAL_CONTROL`, independently of FCU armed state; active Safety/E-stop
produces neutral traction. The bridge retains
timestamps, MAVROS clients and asynchronous response handling:
emergency-service success means local forwarding, not FCU confirmation.

`mower_control` owns only the blade actuator, never ARM/DISARM or mode changes.
ArduPilot uses `MAV_CMD_DO_SET_SERVO` (183); startup parameters are read-only:
`blade_servo_channel=3`, `blade_neutral_pwm=1500`, `blade_forward_pwm=1450`,
`blade_reverse_pwm=1550`. The directional defaults are LOW-EXCURSION acceptance
values for the recorded VESC installation, not universal or nominal-speed values.
They can be configured at startup; no FCU/VESC parameters are written and no
direction is inferred from RPM. Enable0 requests OFF regardless of direction;
enable1/direction0 requests forward, enable1/direction1 reverse. Other ON values
are rejected. ON also requires a connected, already armed FCU, fresh heartbeat,
released/fresh hardware safety, no active/latched emergency, `mowing_enabled`,
and valid/fresh ESC acquisition. The blade does not arm the FCU for the caller.
The blade gate also bounds the ROS acquisition-stamp age to1000ms before
recording monotonic receipt liveness. The generic ESC tracker has a separate
3s display timeout; it must not freshen delayed acquisitions for blade control.

State machine: OFF/FWD/REV, TRANSITION_TO_OFF, WAIT_STOP_BEFORE_REVERSE, FAILED.
Inversions first send neutral and require fresh, distinct zero-RPM observations
after its ACK (at least five, stable for one second). Legacy observations must
advance their counter as well as stamp; replaying cached telemetry cannot prove
an arrest. A missing/stale observation or 15-second stop timeout cancels ON;
it NEVER permits an opposite direction. ACK timeout is three seconds. Failed
commands do not retry each tick; a connection or new explicit safety generation
fences old callbacks and requests neutral again.

Responses are deferred, not blocking callbacks: success means ACK accepted
(`success=true/result0`), or deduplicated already-confirmed command. It is NOT
proof of stopped/spinning hardware. Requests superseded by OFF/safety, rejected
ACKs, invalid transitions and missing services return false. Pending identical
requests share a transaction; at most32 callers are retained. A reversal response
can take the coast-down interval plus ACK time (bounded20 s).

Startup, connection transitions, Emergency, hardware safety, double lift and tilt
also request explicit blade neutral, in addition to the unchanged HOLD/DISARM
policy. Lost transport can only produce a neutral ATTEMPT: no successful write
or physical stop is claimed while disconnected. Outgoing unmatched183 commands
invalidate the cache, including identical PWM; changed rc/out observations also
invalidate it. Own-wire matching uses a bounded expected-message queue; an
external identical command racing an outstanding own message is indistinguishable
on the shared MAVROS endpoint. Deduplication is not exclusive ownership or a
permanent physical-stop proof.

Status projects `blade_requested_direction` as intention (off/forward/reverse,
unknown reserved), not inferred rotation. During neutral/wait/ACK transitions,
`mow_enabled=false`; only an authorized ACK-confirmed ON projects true. Failure
and safety cancel intention to off; actual RPM/current/temperature and
`blade_status_stamp` still come from the unchanged ESC-role projection, and may
show coast-down even with requested off. No public message schema changed.
An OFF request cancels pending ON even if the CommandLong service is unavailable;
it returns false until transport confirmation can be obtained. A deferred neutral
is retained for service recovery, never a deferred superseded ON.
This recovered/adapted revision is NOT deployed. The original October9 product
snapshot passed startup/OFF on target, but its first FWD request was refused;
REV/inversion/safety product acceptance was not acquired. Native A/B bench
evidence is not runtime validation of this recovered/adapted node.
The FCU failsafe for total companion loss remains unresolved; no production
readiness or 100%-speed acceptance is claimed.

The bridge also observes `/mavros/sys_status` for the official MAVLink
`MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS` safety state and decodes ArduPilot
`BUTTON_CHANGE` messages from the MAVROS 2.16 UAS receive stream
`/uas1/mavlink_source`. AP_Button state bit 0 is
the left wheel-lift input and bit 1 is the right input. Hardware safety, service
emergency, tilt and wheel lift are composed in that priority order. Double wheel
lift and sustained excessive inclination request blade neutral plus DISARM.
Hardware Safety and service E-stop send blade neutral, HOLD, neutral traction
and blade DISARM. Blade-neutral forwarding does not replace these protections.

Publishers/subscribers, ROS services and parameters, readiness, generic diagnostics,
canonical GNSS, power, ESC telemetry/projection and wheel odometry remain common.
`betaflight`, `inav` and `mowgli` have explicit unsupported bootstrap capabilities
and still fail with `not implemented`; no C++ runtime provider is constructed.
`auto` is a selection policy, not a concrete firmware provider.

Validation includes profile/selection tests, immutable bootstrap metadata,
exact C++ message equivalence to the previous conversion, emergency policy
and capabilities, and real-bridge tests using mock MAVROS services in isolated
ROS domains. Those tests use no FCU or serial transport. The earlier
[Rock bootstrap evidence](../../../.agent/shared/checkpoints/retained/MM-FIRMWARE-ROCK5B-20261005.md)
refers to the preceding build; it is not new physical validation of this layer.

ESC acquisition is normalized exclusively by the external
[ESC/wheel plugin](../mavros_esc_wheel_odometry/README.md). The bridge subscribes
to its internal `EscObservation` topic; it no longer interprets legacy MAVLink
telemetry counters or wire formats. Role parameters retain `right_esc_slot=0`,
`left_esc_slot=1`, `blade_esc_slot=2` defaults and permit other distinct slots,
including atomic runtime updates. Raw motor RPM accumulation and ticks_per_meter calibration belong to the plugin.
No wheel radius or motor-to-wheel gear ratio is assumed. Track width remains an
uncalibrated installation input.
These are installation inputs, not firmware-provider constants. Public blade
RPM/current freshness remains valid when temperature is unknown; public Status
uses NaN for that temperature while internal validity remains explicit.

The plugin enforces a single COMMON component owner acquired/renewed by STATUS,
with INFO only enriching that owner, and a separate encoder owner. Pre-STATUS INFO
is a metadata candidate and never blocks STATUS from another component. Source clock resets preserve independent observations;
wheel COMMON pairing uses the dedicated 0.25-second skew default. See the plugin
README for `esc_component_id`, `common_pair_max_skew_s` and dynamic validation.
Mowgli wheel PID is not used by the MAVROS command path. The shared feed-forward
tuner calibrates `ticks_per_meter` and `manual_control_linear_scale`; the latter
is persisted through the canonical `wheel_pid_pwm_per_mps` setting.
Cross-node mapping/calibration coherence remains the installation owner's responsibility.
