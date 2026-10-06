# MM-SAFETY-INPUTS-20261006

Disposition: ACTIVE — software contract validated; target hardware acceptance pending.
Repository `/workspaces/mowglimavros`; branch `feat/esc-odometry`;
baseline HEAD `7282fd473286b33d937baac164a3fb5974b45428`.
No commit, push, deployment, FCU write, actuation, MAVROS upstream, public
interface, or MowgliNext change.

## Implemented contract

- Bridge observes `/mavros/sys_status`; the official
  `mavlink::common::MAV_SYS_STATUS_SENSOR::MOTOR_OUTPUTS` bit in
  `sensors_enabled` drives hardware-safety `engaged/released/unknown`.
- MAVROS 2.16 runtime and native graph identify its UAS receive endpoint as
  `/uas1/mavlink_source` (not `/mavros/mavlink_source`). The bridge accepts only
  `FRAMING_OK` COMMON `BUTTON_CHANGE`, using
  `mavros_msgs::mavlink::convert()` and generated MAVLink deserialization.
- AP_Button bit 0 is left/BTN_PIN1/GPIO50; bit 1 is right/BTN_PIN2/GPIO51.
  State validity, sides, raw bitmap and last receipt are independent. Disconnect
  invalidates wheel state without erasing the last observation.
- Dynamic `wheel_lift_safety_enabled` defaults true and gates only canonical
  wheel-lift safety effects. Diagnostics always retain physical observations.
- Canonical Emergency priority is hardware safety, service emergency, then wheel
  lift. Hardware and service latches persist. One lifted wheel warns; two make
  Emergency active. Hardware safety cannot be disabled or cleared by the wheel
  parameter or service clear. Lift duration follows continuous physical lift.
- Diagnostics expose real hardware-safety state and wheel-lift STALE/OK/WARN/ERROR.
  This does not prove or claim physical blade interruption.

## Validation

- Targeted bridge build PASS.
- `test_safety_state`: 13 cases PASS.
- `test_safety_graph`: PASS against real installed MAVROS 2.16; graph logs
  `UAS Prefix: /uas1`, publishes raw frames only on `/uas1/mavlink_source`, and
  covers dynamic gating, SysStatus, diagnostics, priority, service clear and
  disconnect.
- Full `mowgli_mavros_bridge` suite: 11/12 suites PASS. The unchanged historical
  `test_esc_graph` intermittently fails its retained-left-raw-ticks equality in
  the complete sequence; it passed twice in isolated reruns. Its file/assertion
  has no diff and was not modified.
- Fresh full workspace build PASS: 6/6 packages in 39.4 s.
- Interface contract, 10 external backend contracts, README progress check,
  Python/YAML syntax, ROS `ament_uncrustify`, and `git diff --check` PASS.
- `clang-format` is unavailable and no `.clang-format` exists; the established
  repository C++ formatter is `ament_uncrustify`.

## Hardware pending

Verify on the target Pixhawk/ArduPilot installation: safety-switch polarity and
transitions, BTN_PIN1/GPIO50 left and BTN_PIN2/GPIO51 right mapping, BUTTON_CHANGE
cadence/reconnect, and actual propulsion/blade authority. Blade-on validation
requires separate authorization.

## Exact next step

Review the uncommitted software patch. If accepted, perform a separately
authorized passive/no-motion target deployment and switch/button observation.
