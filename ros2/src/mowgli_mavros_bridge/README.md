# Firmware providers

The backend resolves `MAVROS_FIRMWARE` once before creating its two nodes. `auto`
is the default; explicit `ardupilot` and `px4` values bypass heartbeat detection.
Bootstrap metadata lives in `python/mowgli_mavros_bridge/firmware_provider.py`.
The selected concrete firmware is passed to the bridge through the private process
environment `MAVROS_RESOLVED_FIRMWARE`, without adding a ROS parameter or topic.

The C++ `FirmwareProvider` owns MANUAL_CONTROL conversion, default/overridden
emergency policy and command capabilities. ArduPilot and PX4 share the existing
conversion and policy implementation; no native PX4 axis or emergency-mode change
is introduced by this extraction. Each concrete provider declares its identity.
A standalone bridge without the private environment retains its existing Rover
conversion and emergency defaults.

| Capability / policy | ArduPilot provider | PX4 provider |
| --- | --- | --- |
| MAVROS profile | `apm` | `px4` |
| Heartbeat autodetection | Implemented | Implemented |
| MANUAL_CONTROL mapping | Existing steering `y`, throttle `z` | Same existing mapping preserved |
| Arm request path implemented | Yes | Yes |
| Disarm request path implemented | Yes | Yes |
| Mode request path implemented | Yes | Yes |
| Default emergency policy | `HOLD` + disarm | Existing `HOLD` + disarm preserved |
| Emergency parameter overrides | Passed through unchanged, including empty mode | Same |
| Blade command implementation | No | No |
| Physical actuation validated | No | No |

Capabilities describe implemented software paths. They do not enable drive or
blade control, synthesize firmware compatibility, or certify FCU/actuator behavior.
The bridge retains its existing finite-input, drive and neutral-only gates. It
also retains timestamps, MAVROS clients and asynchronous response handling:
emergency-service success means local forwarding, not FCU confirmation.

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
