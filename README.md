# mowglimavros

Sidecar ROS 2 repository for the optional MAVROS backend used by MowgliNext.

<!-- MOWGLIMAVROS_PROGRESS_BEGIN -->
## Project Progress

![Generated MowgliMAVROS progress](docs/status/mowglimavros_progress.svg)

**Verified completion: 11 / 25 (44.00%)**. An item counts as fully complete only when its TODO status is `DONE` and it has no `HARDWARE_PENDING` qualifier.

Status: **DONE 17** · **IN PROGRESS 2** · **TODO 3** · **BLOCKED 2** · **DEFERRED 1**. Hardware-pending items: **6** (software completion is not presented as physical validation).

### Phase Progress

- **0 — Agent state and durable baseline:** 1 / 1 verified (100.00%); DONE 1
- **1 — Source compatibility Kilted + Lyrical:** 1 / 3 verified (33.33%); DONE 3
- **2 — NTRIP / RTCM correctness:** 0 / 1 verified (0.00%); TODO 1
- **3 — Reproducible MAVROS container:** 4 / 5 verified (80.00%); IN PROGRESS 1, DONE 4
- **4 — Build tooling and CI:** 2 / 3 verified (66.67%); IN PROGRESS 1, DONE 2
- **5 — Runtime smoke validation:** 2 / 2 verified (100.00%); DONE 2
- **6 — Backend contract before MowgliNext integration:** 1 / 7 verified (14.29%); TODO 2, DONE 5
- **7 — MowgliNext integration audit:** 0 / 1 verified (0.00%); BLOCKED 1
- **8 — Hardware validation:** 0 / 2 verified (0.00%); BLOCKED 1, DEFERRED 1

Generated solely from [`TODO.md`](TODO.md). Run `python3 tools/update_readme_progress.py` to update, or `python3 tools/update_readme_progress.py --check` to fail on stale output.
<!-- MOWGLIMAVROS_PROGRESS_END -->


This repository intentionally contains only:

- `ros2/src/mowgli_interfaces` (minimal interface package required to build the sidecar alone)
- `ros2/src/mowgli_mavros_bridge`
- `ros2/src/mowgli_ntrip_client`
- `Dockerfile`
- `ros2_entrypoint.sh`
- `build.sh`
- `README.md`

It does not carry MowgliNext bringup, GUI, Nav2, simulation, FusionCore, or any
other main-stack package.

The sidecar is expected to run next to MowgliNext over ROS 2/DDS with
`network_mode: host`, while MowgliNext itself only switches backend selection
through `HARDWARE_BACKEND=mavros`.
