# MM-FIRMWARE-SELECTION-20261005
Disposition: RETAINED (software evidence; Rock 5B serial bootstrap validated separately)
Repository: /workspaces/mowglimavros
Branch: refactor/firmware-providers
Baseline HEAD: 82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492
Commit/push: none in this session.

## Objective and scope
Replace firmware environment selection, remove its legacy variable/alias, centralize
firmware-to-MAVROS profile resolution. User explicitly requested automatic detection
in this change. GNSS compatibility, C++ bridge behavior, actuator logic and external
MowgliNext deployment files are outside scope.

## Verified implementation
- `ros2_entrypoint.sh` defaults/exports `MAVROS_FIRMWARE=auto`.
- `launch/mavros_backend.launch.py::resolve_mavros_profile` accepts only ardupilot,
  px4, betaflight, inav, mowgli, auto (case-insensitive, as before).
- Default auto invokes detection; explicit ArduPilot/PX4 values override it.
- ArduPilot still loads apm_pluginlists.yaml/apm_config.yaml; PX4 still loads
  px4_pluginlists.yaml/px4_config.yaml. Public apm is rejected. Complete explicit
  MAVROS Node configuration is checked in `test_gnss_launch.py`.
- Betaflight/INAV/Mowgli fail with NotImplementedError and "not implemented".
- auto runs installed `detect_mavros_firmware` via bounded subprocess. It uses
  libmavconn, passively waits up to ten seconds for a valid target HEARTBEAT,
  identifies ARDUPILOTMEGA/PX4, releases transport before MAVROS startup. Invalid
  framing, peripheral heartbeat and other targets are ignored. Unknown identity,
  unavailable device or timeout fail without fallback. Probe process capped at
  twelve seconds, including connection startup.
- libmavconn 2.16 exports include/library variables rather than imported targets;
  MAVLink headers require an explicit mavlink dependency. Reuses existing overlays.

## Validation PASS
Lyrical amd64, installed MAVROS/libmavconn 2.16.0 and MAVLink overlay:
- Affected bridge CMake configure/build; install to temporary prefix includes probe.
- CTest: test_gnss_launch, test_firmware_detection, test_readiness_state,
  test_esc_telemetry_tracker, test_rover_manual_control (5/5 suites).
- Launch suite: 10 cases; detector suite: 5 cases with subcases including v1/v2,
  target filtering, CRC, timeout, unknown identity, invalid transport/arguments,
  absence of emitted packets and listener release.
- tools/test_external_backend_contract.py: 10/10 cases.
- tools/update_readme_progress.py --check; Python AST/XML syntax; bash -n;
  git diff --check; hidden-worktree search for previous firmware variable: no hits.
- Initial checks were software-only. Physical serial follow-up is recorded in
  `MM-FIRMWARE-ROCK5B-20261005.md`.

## Physical evidence and remaining acceptance
Rock 5B/Pixhawk5X passive serial bootstrap auto and explicit ArduPilot tested;
see `MM-FIRMWARE-ROCK5B-20261005.md` for topology, measurements and limits.
TCP, other FCUs/firmware, USB-loss recovery and repeated reconnect acceptance are
not generalized from this test. No propulsion or actuator acceptance is claimed.

## Preserved user state
Pre-existing INDEX.md magnetometer entry and untracked active magnetometer checkpoint
were preserved. No commit or push; local checkpoint remains ignored.

## Next step
Review and commit selected patch files. Retain separate transport/recovery and
physical actuation acceptance gates; see the Rock validation checkpoint.
