# mowglimavros

Sidecar ROS 2 repository for the optional MAVROS backend used by MowgliNext.

<!-- MOWGLIMAVROS_PROGRESS_BEGIN -->
## Project Progress

![Generated MowgliMAVROS progress](docs/status/mowglimavros_progress.svg)

**Verified completion: 13 / 25 (52.00%)**. An item counts as fully complete only when its TODO status is `DONE` and it has no `HARDWARE_PENDING` qualifier.

Status: **DONE 19** · **IN PROGRESS 1** · **TODO 3** · **BLOCKED 1** · **DEFERRED 1**. Hardware-pending items: **6** (software completion is not presented as physical validation).

### Phase Progress

- **0 — Agent state and durable baseline:** 1 / 1 verified (100.00%); DONE 1
- **1 — Source compatibility Kilted + Lyrical:** 1 / 3 verified (33.33%); DONE 3
- **2 — NTRIP / RTCM correctness:** 0 / 1 verified (0.00%); TODO 1
- **3 — Reproducible MAVROS container:** 5 / 5 verified (100.00%); DONE 5
- **4 — Build tooling and CI:** 3 / 3 verified (100.00%); DONE 3
- **5 — Runtime smoke validation:** 2 / 2 verified (100.00%); DONE 2
- **6 — Backend contract before MowgliNext integration:** 1 / 7 verified (14.29%); TODO 2, DONE 5
- **7 — MowgliNext integration audit:** 0 / 1 verified (0.00%); IN PROGRESS 1
- **8 — Hardware validation:** 0 / 2 verified (0.00%); BLOCKED 1, DEFERRED 1

Generated solely from [`TODO.md`](TODO.md). Run `python3 tools/update_readme_progress.py` to update, or `python3 tools/update_readme_progress.py --check` to fail on stale output.
<!-- MOWGLIMAVROS_PROGRESS_END -->


This repository owns the standalone MAVROS-side hardware backend implementation.
MowgliNext owns the main-stack integration, installer/Compose configuration,
backend selection, GUI, navigation and deployment policy.

The current primary ROS 2 baseline is Lyrical. Historical Kilted validation is
retained in the project audit and checkpoints; it is not the current acceptance
target.

The repository contains the sidecar-specific ROS packages:

- `ros2/src/mowgli_interfaces` — pinned minimal Mowgli public interface subset
- `ros2/src/mowgli_mavros_bridge` — canonical Mowgli hardware bridge
- `ros2/src/mowgli_ntrip_client` — RTCM/NTRIP support
- `ros2/src/mavros_battery_observer` — MAVLink battery/power observation
- `ros2/src/mavros_esc_wheel_odometry` — wheel-only odometry from ESC/RPM telemetry
- `ros2/src/mavros_gnss_adapter` — canonical GNSS adapter for Universal GNSS over MAVROS

Runtime/container tooling includes `ros2/Dockerfile`, `ros2_entrypoint.sh`,
`build.sh`, and `build-arm64-remote.sh`.

It does not carry MowgliNext bringup, GUI, Nav2, simulation, FusionCore, or the
main-stack installer. The sidecar is intended to run alongside MowgliNext over
ROS 2/DDS, with backend and GNSS ownership selected by the MowgliNext deployment
configuration.


Firmware selection uses `MAVROS_FIRMWARE` (default: `auto`). The only public
values are `ardupilot`, `px4`, `betaflight`, `inav`, `mowgli`, and `auto`.
Explicit `ardupilot` and `px4` selections override detection and retain their
existing MAVROS profiles and launch settings.
`betaflight`, `inav`, and `mowgli` are recognized but fail with `not implemented`.
Legacy firmware selection variables and the public alias `apm` are unsupported;
`apm_*` remains the upstream MAVROS profile filename for ArduPilot.

`auto` passively reads a valid MAVLink HEARTBEAT on `MAVROS_FCU_URL` from
`MAVROS_TGT_SYSTEM` / `MAVROS_TGT_COMPONENT` (both default to `1`). It recognizes
ArduPilot or PX4, releases the transport, then starts MAVROS with that profile.
The probe sends no MAVLink messages and waits up to ten seconds for the target
heartbeat. Unknown firmware, unavailable transport, or missing heartbeat stops
startup with an explicit error; there is no default-profile fallback.
Automatic detection is covered by MAVLink v1/v2 loopback UDP tests and a passive
serial bootstrap test on Rock 5B / Pixhawk5X (ArduRover 4.7.1). The probe-to-MAVROS
handover and explicit ArduPilot override both connect on that setup. TCP startup,
other hardware and recovery after USB loss still require validation. See the
[physical evidence](.agent/shared/checkpoints/retained/MM-FIRMWARE-ROCK5B-20261005.md).

The [firmware provider layer](ros2/src/mowgli_mavros_bridge/README.md) owns profile
metadata, MANUAL_CONTROL mapping, emergency policy and explicit command
capabilities. ArduPilot/PX4 retain their current profiles and runtime behavior;
ROS transport, telemetry, readiness, GNSS, power and ESC handling remain shared.
