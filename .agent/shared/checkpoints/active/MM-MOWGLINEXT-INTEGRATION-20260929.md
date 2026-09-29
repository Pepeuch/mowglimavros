# MM-MOWGLINEXT-INTEGRATION-20260929 — ACTIVE

Repository `/workspaces/mowglimavros`, branch `main`, baseline HEAD `781946d7ba8a595d7a472781763ffac6800545da`. MowgliNext on Rock 5B at `/home/pepeuch/mowglinext`, branch `feat/mavros-refresh`, baseline HEAD `fac08421843a76881b9866be4d0c202dfadd6eaa`. Both worktrees were dirty before this task. No commit/push.

## Scope and safety

Authorized integration of canonical IMU, POWER, VESC, temporary serial GPS1, GUI diagnostics, and a neutral-only software command test. Do not arm or move motors, command mower, change CAN, widen `ARMING_SKIPCHK`, or alter unrelated FCU parameters. `manual_control_enabled=false`, `blade_control_enabled=false`. No live redeploy yet as of this checkpoint.

## FCU and runtime facts

Pixhawk 5X / ArduRover 4.7.1 on Rock `192.168.10.32`, MAVROS system ID 255 targeting FCU 1. Last observed FCU `connected=true`, `armed=false`, MANUAL. Before change: `SERIAL3_PROTOCOL=5`, `SERIAL3_BAUD=460`, `GPS1_TYPE=9`, `GPS1_RATE_MS=125`, `GPS1_GNSS_MODE=109`, `GPS_AUTO_CONFIG=3`, `GPS_AUTO_SWITCH=4`, `GPS_PRIMARY=1`, `GPS1_CAN_OVRIDE=124`; `ARMING_SKIPCHK=1039710` remains operator-provisional. Changed **only** `GPS1_TYPE` 9→1 (AUTO) and rebooted disarmed FCU as required. Subsequent FCU read confirmed value 1. Native MAVROS GPS1 raw: 3D fix, 22 satellites, ~43.9542266 / 2.2022331, HDOP 0.68, horizontal accuracy 0.967 m. Measured `gps1/raw` at 2.000 Hz over 19 intervals, 0.483–0.518 s. HERE4 CAN2 remains faulty and separate.

Physical evidence retained from prior audits: POWER1 is dock/charger (currently disconnected off dock), POWER2 traction/battery; ESC0/node1 right wheel, ESC1/node2 left wheel, ESC2/node3 mower. VESC telemetry has three real slots; slot3 empty. No movement or arm in this task.

Current deployed MowgliMAVROS is old Kilted image. `/gps/status` publisher Universal GNSS bridge has ROS type hash `RIHS01_6765…`, while old sidecar hardware_bridge subscriber has `RIHS01_eb5…`; this proves the prior IDL mismatch. Existing `/gps/fix` Universal GNSS receiver produced no data in the observed window. No source switch yet.

## Software prepared and validated

Local `mowgli_interfaces` copies current MowgliNext `GnssStatus.msg` and `Status.msg`; lock now pins current MowgliNext revision. Contract checker and focused tests updated; `mowgli_interface_contract.py check`, `test_external_backend_contract.py` (8 tests), and `test_mowgli_interface_contract.py` (3 tests) pass. Bridge adds signed POWER mapping (dock/traction, unavailable net off dock), three-slot VESC freshness by count, serial GPS1 canonical projection, IMU/traction/GNSS readiness, diagnostics, and neutral-only Rover y/z path. Kilted targeted tests: 46 pass. Lyrical overlay image `mowgli-mavros-sidecar:integration-lyrical-overlay-20260929` built on published pinned Lyrical ARM64 base and its 46 targeted tests pass. Dockerfile MAVLink pin updated to available `2026.9.9` and runtime now sets `ROS_DISTRO`; full fresh Lyrical build remains in progress and is not yet accepted.

MowgliNext GUI source has a new MAVROS diagnostics card, zero/absent/stale/error rendering, localized EN/FR, and Compose opt-ins. Initial frontend TypeScript, 3 Vitest cases, and production build passed. A small text-field stale rendering correction is now rebuilding. Thin GUI image from the latest frontend dist still needs rebuilding. No live container changes yet.

## Exact continuation

1. Finish GUI frontend validation; rebuild thin GUI image from updated dist. Poll full Lyrical image build; if it fails on its old context, resync current tree and rerun from cached layers.
2. Before live changes, confirm `armed=false` and FCU MANUAL. Use MowgliNext settings API to set `gnss_stack=disabled` (old value `universal`) so external Universal GNSS no longer owns canonical topics; update runtime image/opt-in keys via `install/lib/env.sh` producer, regenerate Compose. Stop `mowgli-gps`; recreate only `mavros`, `mowgli`, and `gui` as needed. Avoid duplicate `/gps/fix` and `/gps/status` publishers.
3. Validate Lyrical ROS hashes/publishers and compare raw MAVROS → canonical `/imu/data`, `/hardware_bridge/power`, `/hardware_bridge/status`, `/gps/fix`, `/gps/status`, diagnostics and GUI. Test only zero `/cmd_vel` to neutral MANUAL_CONTROL, then disable temporary neutral opt-in. Confirm disarmed/MANUAL after.
4. Update this checkpoint and TODO; add MowgliNext migration checkpoint; run `git diff --check` in both repos. No commit/push.

SSH to Rock uses pre-existing key `~/.ssh/id_ed25519_rock5b_mavros`; never copy credentials into checkpoints. Source commands in running Kilted sidecar with `/opt/ros/kilted/setup.bash` then `/ros2_ws/install/setup.bash`; new Lyrical overlay adds `/integration_ws/install/setup.bash` and has `ROS_DISTRO=lyrical`.
