# Rock 5B backend acceptance — preserved historical plan

Imported exact excerpts from Next sensors/mavros/VALIDATION.md.
Original SHA256 721396d6b77492338507fa3ddb9d8fda739a87f890478d5c29ffd18777253bd0.
The labels below are historical, not newly established acceptance.
No hardware run, build or software validation was performed by moving this document.
The old separately operated ARM procedure predates the minimal FCU adapter8eecc57;
current sequence acceptance is [fcu-session-controller.md](../../architecture/fcu-session-controller.md).
GUI HTTP steps remain [in Next](../../../../../../../docs/validation/mavros-refresh/MowgliNext-calibration.md).

<!-- source: MowgliNext:sensors/mavros/VALIDATION.md; lines 1-74; sha256 d331b561164713696f6d488f04781bd9b0ff13b658ec57e2e65c233657cb6a83 -->

# MAVROS refresh acceptance on Rock 5B

Software baseline: MowgliMAVROS `main` at
`b92925b2bc969585350b824ac066004eae903e3b` plus the complete current patch;
MowgliNext `feat/mavros-refresh` at
`24094f32f9748628f354e03d73b106d5c7eceb07` plus the complete current patch.
Capture the actual image digests, FCU firmware, board, ArduPilot parameter
profile and host kernel before acceptance. Results apply only to that recorded
robot/firmware/wiring/image baseline; existing motor-feedback validation stands.

Status: `ENVIRONMENT_PENDING` for the real MAVROS graph and full sidecar build;
`HARDWARE_PENDING` for the new RTK-calibration, safety/traction acceptance and
Hall electrical/GPIO validation. Nothing here is a deployment, arm or motion
command.

## Baseline and software graph (read-only)

On the Rock 5B host:

```bash
uname -a
docker inspect --format '{{.Name}} {{.Image}} {{json .Config.Labels}}' mowgli-mavros mowgli-ros2 mowgli-gui
docker exec mowgli-mavros cat /ros2_ws/config/esc_wheel_odometry.yaml
docker exec mowgli-mavros cat /ros2_ws/config/hardware_bridge.yaml

# Use the sidecar's existing overlays for every ROS CLI invocation.
mavros_ros() {
  docker exec mowgli-mavros bash -lc 'set -e; source /opt/ros/lyrical/setup.bash; source /opt/mowgli/mavros/setup.bash; source /opt/mowgli/universal_gnss/setup.bash; source /ros2_ws/install/setup.bash; if [ -f /integration_ws/install/setup.bash ]; then source /integration_ws/install/setup.bash; fi; exec ros2 "$@"' -- "$@"
}

mavros_ros node info /hardware_bridge
mavros_ros param get /mavros/esc_wheel_odometry ticks_per_meter
mavros_ros param get /mavros/esc_wheel_odometry track_width_m
mavros_ros topic info /wheel_ticks --verbose
mavros_ros topic echo /wheel_ticks --once
mavros_ros topic echo /mavros/state --once
mavros_ros topic echo /mavros/sys_status --once
mavros_ros topic echo /diagnostics --once
curl --fail --silent http://localhost:4006/api/settings/hardware-backend
```

Pass: one `/wheel_ticks` publisher, `mowgli_interfaces/msg/WheelTick`, reliable
QoS; RL/RR valid only from reliable signed feedback; raw units motor revolutions;
`wheel_tick_transport_scale=1000`, and `wheel_tick_factor = ticks_per_meter * 1000`.
Counters accumulate independently of calibration. Source/epoch/segment changes
and reconnect rebase references while published magnitudes remain continuous.
Check both generated files after saving settings and after an approved restart.
Traction defaults false, lift protection true, blade control false.

## Safety telemetry and electrical boundary

Prerequisites: disarmed FCU, traction isolated, blade electrically disconnected,
operator able to use the physical stop. Preserve the already validated Safety
Switch wiring. Use the verified switch only; do not connect Hall sensors until
their exact GPIO assignment and electrical levels have been confirmed.

Observe `/mavros/sys_status`, `/hardware_bridge/emergency`, `/diagnostics` and
Settings → Safety while operating the switch manually. Pass: Unknown when
MOTOR_OUTPUTS is absent; Engaged when present/disabled; Released when enabled.
Clearing the service latch cannot clear an engaged physical switch. Release
clears only its hardware latch. Nonzero traction is blocked for Unknown or
Engaged. Once Hall wiring is verified, lift left/right independently: Unknown /
On ground / Lifted must match reality, one lift orange, two red. Turn lift
protection off through the confirmed GUI action: raw telemetry stays visible,
physical Safety remains authoritative. Restore protection on afterward.

## RTK calibration and explicit traction opt-in

Prerequisites: blade electrically disconnected, clear straight test lane,
physical stop available, verified traction mapping and safety, RTK Fixed with
fresh observations. Enabling traction in Settings → Drive requires explicit
confirmation and Save; it must not arm the FCU. Arming, if separately authorized
for the physical run, remains an operator/FCU action.


<!-- source: MowgliNext:sensors/mavros/VALIDATION.md; lines 86-92; sha256 6ea865973069615c3cc11c0309b37ae76fccc55d60a0a38efa634f9f53dd4bfb -->

Pass: state `ready`, consistent left/right raw motor revolutions, and
`ticks_per_meter = mean(left_revolutions, right_revolutions) / distance_RTK`.
The calculation must be independent of the initial scale. A bad source, epoch,
segment, reversal, clock reset, invalid wheel, stale/frozen stream or RTK loss
rejects the run. Repeat a straight validation pass; measured wheel distance
should agree with independent RTK distance within 5% on this setup.


<!-- source: MowgliNext:sensors/mavros/VALIDATION.md; lines 99-100; sha256 513f7db66af11480c70116bf7a087a0b96afb1f4674f95ffb556294311ee8c5a -->

```bash
mavros_ros param get /mavros/esc_wheel_odometry ticks_per_meter
mavros_ros topic echo /wheel_ticks --once
```

<!-- source: MowgliNext:sensors/mavros/VALIDATION.md; lines 103-121; sha256 00100d833bfa71c4df2d5a83c07d04fdb4d5dfbfedd96b0f9b3b3b0ccc48716c -->

Pass: state `applied`, confirmed live scalar, persisted canonical YAML and
dedicated startup scalar match; every PID/FF value is unchanged. On failure,
no success is claimed. Disable traction through the GUI after the test,
disarm using the existing operator procedure, and verify blade control is false.

## Full sidecar software tests (existing build environment)

In a non-root checkout/build environment that already supplies the pinned
MAVROS/MAVLink dependencies:

```bash
cd ros2
colcon build --packages-up-to mowgli_mavros_bridge --cmake-args -DBUILD_TESTING=ON
colcon test --packages-select mavros_esc_wheel_odometry mowgli_mavros_bridge
colcon test-result --verbose
```

The local integration workspace intentionally does not install missing MAVROS
dependencies. Full plugin compilation and native graph tests are pending there.
