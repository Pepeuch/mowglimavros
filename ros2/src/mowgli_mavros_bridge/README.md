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
| Blade authorization mapping | `/hardware_bridge/mower_control` → `/mavros/cmd/arming` | Same |
| Physical actuation validated | No | No |

Capabilities describe implemented software paths; physical actuation still depends
on the configured FCU and actuators. Valid finite `/cmd_vel` input is mapped
to Rover `MANUAL_CONTROL`, independently of FCU armed state; active Safety/E-stop
produces neutral traction. The bridge retains
timestamps, MAVROS clients and asynchronous response handling:
emergency-service success means local forwarding, not FCU confirmation.

The bridge also observes `/mavros/sys_status` for the official MAVLink
`MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS` safety state and decodes ArduPilot
`BUTTON_CHANGE` messages from the MAVROS 2.16 UAS receive stream
`/uas1/mavlink_source`. AP_Button state bit 0 is
the left wheel-lift input and bit 1 is the right input. Hardware safety, service
emergency, tilt and wheel lift are composed in that priority order. Double wheel
lift and sustained excessive inclination force blade DISARM. Hardware Safety and
service E-stop send HOLD, neutral traction and blade DISARM.

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
