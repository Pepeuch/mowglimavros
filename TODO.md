# MowgliMAVROS TODO

Canonical work queue for `mowglimavros`.

This file answers: **what remains to be done and in what order?**

Shared checkpoints under `.agent/shared/checkpoints/` preserve evidence,
decisions and resumable state. They are not the backlog.

## Status convention

- [ ] TODO
- [~] IN PROGRESS
- [x] DONE
- [!] BLOCKED
- [-] DEFERRED

Do not mark an item DONE only because it compiles.
Use the acceptance criteria attached to the item.

---

# Phase 0 — Agent state and durable baseline

## MM-000 — Persist audit state
Status: DONE (updated after workstation capability audit, 2026-09-08)

- [x] Create/update shared retained audit checkpoint.
- [x] Create/update active compatibility checkpoint.
- [x] Create/update blocked Pixhawk validation checkpoint.
- [x] Update shared checkpoint index.
- [x] Verify `git diff --check`.

Expected checkpoints:

- `.agent/shared/checkpoints/retained/MM-MAVROS-AUDIT-20260907.md`
- `.agent/shared/checkpoints/active/MM-MAVROS-COMPAT.md`
- `.agent/shared/checkpoints/blocked/MM-PIXHAWK-VALIDATION.md`

Acceptance:

- A new agent can resume without repeating the compatibility audit.
- `MM-AUD-001` through `MM-AUD-008` are preserved.
- The audited HEAD and MAVROS pin are recorded.

---

# Phase 1 — Source compatibility Kilted + Lyrical

## MM-101 — Fix MAVROS launch integration
Related finding: `MM-AUD-001`
Status: DONE (validated on Kilted amd64 workstation, 2026-09-08)

Historical problem (resolved):

- local integration requests `apm.launch.py` / `px4.launch.py`;
- MAVROS 2.15.1 installs `apm.launch` / `px4.launch`.

Tasks:

- [x] Correct MAVROS launch inclusion.
- [x] Verify expected namespace behaviour.
- [x] Verify APM launch path.
- [x] Verify PX4 launch path if retained/supported.
- [x] Add focused isolated launch validation.

Acceptance:

- MAVROS launch resolution succeeds.
- No duplicate or unintended namespace is introduced.

### Firmware selection update — 2026-10-05

- Selection now uses only `MAVROS_FIRMWARE` (default: `auto`); the public `apm`
  alias is rejected. Explicit ArduPilot/PX4 selections only override detection.
- A single launch resolver maps ArduPilot/PX4 to their unchanged MAVROS profiles.
- Betaflight/INAV/Mowgli fail explicitly with `not implemented`.
- `auto` uses a passive, bounded libmavconn heartbeat probe for the configured FCU
  target, with no fallback. MAVLink v1/v2 UDP loopback and passive Rock 5B /
  Pixhawk5X serial bootstrap pass. Explicit ArduPilot also connects; TCP and
  USB-loss recovery remain pending. Physical observations are retained in
  `.agent/shared/checkpoints/retained/MM-FIRMWARE-ROCK5B-20261005.md`.
- Separate follow-up: canonical GNSS briefly reports invalid in the explicit
  bootstrap capture while its upstream/raw samples are valid; investigate freshness
  using the retained physical observations. No GNSS change is included here.
- Evidence: `.agent/shared/checkpoints/retained/MM-FIRMWARE-SELECTION-20261005.md`.

### Firmware provider extraction — 2026-10-05

- Bootstrap metadata/capabilities are centralized in the Python provider registry;
  one resolution selects both MAVROS profile and C++ provider.
- Providers own MANUAL_CONTROL conversion, emergency policy and explicit
  arm/disarm/mode capabilities. ArduPilot/PX4 preserve existing behavior,
  `HOLD`/disarm defaults and ROS parameter overrides; actuation validation is false.
- ROS graph/transport, readiness, diagnostics, canonical GNSS, power and ESC
  handling remain shared. MowgliNext is unchanged.
- Selection, provider and mock-service regressions accompany the extraction.
- Evidence: `.agent/shared/checkpoints/retained/MM-FIRMWARE-PROVIDERS-20261005.md`.

---

## MM-102 — Make CMake compatible with Lyrical
Related findings: `MM-AUD-003`, `MM-AUD-006`
Status: DONE (software); HARDWARE_PENDING (FCU/VESC configuration and validation)

Tasks:

- [x] Replace obsolete `ament_target_dependencies()` with imported targets.
- [ ] Raise only required `cmake_minimum_required()` values.
- [ ] Remove/replace distro-specific Boost assumptions.
- [ ] Address `CMP0167` only where required by compatibility.
- [x] Keep the same C++ source for Kilted and Lyrical.
- [x] Avoid ROS-distro `#ifdef` unless proven unavoidable.

Acceptance:

- Kilted amd64 workspace build passes (2026-09-07).
- Lyrical amd64 workspace build passes (2026-09-07).
- No compatibility regression is introduced on Kilted (same modern CMake source).

---

## MM-103 — Update ROS callback signatures
Related audit observation: rclcpp deprecation compatibility
Status: DONE (software; HARDWARE_PENDING)

Tasks:

- [ ] Replace applicable mutable `SharedPtr` callbacks with `ConstSharedPtr`.
- [ ] Limit changes to callbacks for which the message is read-only.
- [ ] Verify Kilted and Lyrical compilation.

Acceptance:

- No relevant callback deprecation remains.
- Behaviour remains unchanged.

---

# Phase 2 — NTRIP / RTCM correctness

## MM-201 — Bound RTCM publications to MAVROS limits
Related finding: `MM-AUD-004`
Status: TODO

Current problem:

- NTRIP socket reads may return up to 4096 bytes.
- MAVROS accepts at most 720 bytes in one `mavros_msgs/RTCM`.

Tasks:

- [ ] Segment input before ROS publication.
- [ ] Ensure every published RTCM message is `<= 720` bytes.
- [ ] Preserve byte order exactly.
- [ ] Ensure no byte is lost.
- [ ] Ensure no byte is duplicated.
- [ ] Avoid unnecessary buffering/refactor outside this requirement.

Acceptance tests:

- [ ] 1 byte
- [ ] 179 bytes
- [ ] 180 bytes
- [ ] 181 bytes
- [ ] 719 bytes
- [ ] 720 bytes
- [ ] 721 bytes
- [ ] 4096 bytes

For every case:

`concat(output_chunks) == input`

Evidence: the 2026-09-08 passive Pixhawk capability audit confirmed MAVROS 2.15.1 accepts at most 720 bytes per ROS RTCM message. No RTCM was injected; this remains a software prerequisite and hardware delivery acceptance remains `HARDWARE_PENDING` under MM-801.

Validation blocker (2026-09-08): the focused `mowgli_ntrip_client` build cannot configure in the current environment because CMake cannot find Boost headers/system (`Boost_INCLUDE_DIR`, `system`). No dependency installation or broader build is authorized; MM-201 remains TODO until the focused gtest runs.

Status: DONE (focused Kilted amd64 validation, 2026-09-08)

Evidence:
- mowgli_ntrip_client builds successfully.
- focused RTCM chunking gtest passes.
- required boundary inputs are covered.
- every emitted ROS RTCM chunk is <=720 bytes.
- concatenation preserves the original input byte-for-byte.
- local test compiled against ros-kilted-mavros-msgs 2.15.0;
  at that validation point production MAVROS was pinned to 2.15.1 / 22ae5b7c;
  the current baseline is MAVROS 2.16.0 / 5c68b905ab30de6ce630822dc46c33467e8f23ea.
- physical RTCM delivery remains HARDWARE_PENDING under HW-MAV-005.
---

# Phase 3 — Reproducible MAVROS container

## MM-301 — Parameterize ROS distribution
Related finding: `MM-AUD-005`
Status: DONE

Target:

```dockerfile
ARG ROS_DISTRO=kilted
```

Tasks:

- [x] Remove hard-coded `kilted` image references.
- [x] Remove hard-coded `/opt/ros/kilted`.
- [x] Remove hard-coded `ros-kilted-*`.
- [x] Preserve Kilted as default.
- [x] Support Lyrical through build argument.

Acceptance:

- Same Dockerfile builds for Kilted and Lyrical.
- [x] Kilted and Lyrical amd64/arm64 images were built and pushed by GitHub Actions run 34483209217 for commit `0fcf7444e020e22cd0be11ddab008f277be03f2a`.
- [x] Production Kilted manifest (also `latest`) is `ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar@sha256:04e4eb17b0f5ce38f882f68346b1694774fa87e1945b38b57c94f90da34dd560`; Lyrical is `ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar@sha256:9fa1ab1652d10c50433198f19ced13d7f70e1549ae3a98dcc1184b06fbfe2cb6`. Both are multi-architecture (amd64 and arm64).

---

## MM-302 — Pin MAVROS 2.15.1
Related finding: `MM-AUD-002`
Status: DONE

Target:

```text
MAVROS_VERSION=2.15.1
MAVROS_COMMIT=22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e
```

Tasks:

- [x] Build MAVROS from the pinned upstream source.
- [x] Verify tag resolves to expected commit.
- [x] Fail build if expected MAVROS version differs.
- [x] Use the same MAVROS version on Kilted and Lyrical.
- [x] Pin or explicitly verify MAVLink dependency.

Acceptance:

- Runtime build verifies MAVROS 2.15.1 and its pinned commit on Kilted and Lyrical amd64.
- Kilted and Lyrical use the same MAVROS release.
- Serial URL at 921600 uses the corrected MAVROS implementation.

---

## MM-303 — Make GeographicLib reproducible
Related finding: `MM-AUD-005`
Status: DONE

Tasks:

- [x] Stop fetching installation logic from a moving upstream branch.
- [x] Avoid installing datasets twice.
- [x] Determine exactly which datasets/runtime files MAVROS requires.
- [x] Install/copy them once through the multi-stage build (Kilted amd64 validated against release `geographiclib-datasets-v1`).
- [x] Preserve amd64 + arm64 compatibility.

Acceptance:

- GeographicLib setup is deterministic.
- Final-tree Kilted/Lyrical amd64 and native ARM64 image builds passed on 2026-09-10.
- No duplicate network-heavy installation occurs.
- Release `geographiclib-datasets-v1`, all four assets, approved SHA256 values,
  and cached source-only rebuild behaviour are validated.

---

## MM-304 — Remove Noble-specific runtime ABI assumptions
Related finding: `MM-AUD-006`
Status: DONE

Tasks:

- [x] Remove hard-coded `libboost-system1.83.0` if unnecessary.
- [x] Use distro-portable dependency resolution.
- [x] Verify both ROS distributions.

Acceptance:

- Docker dependency installation succeeds on Kilted and Lyrical.

Evidence: Final-tree Kilted amd64 and Lyrical amd64 full Docker builds pass with generic `libboost-dev`, header-only `Boost::headers`, and modern explicit rosidl C++ typesupport linkage. Native ARM64 Kilted and Lyrical builds also pass; sourced runtime NTRIP linker closures contain neither missing libraries nor `libboost_system`.

---

### MM-306 — Add UG-style README progress dashboard
Status: DONE

Objective:

Reuse the Universal GNSS progress/status presentation model for MowgliMAVROS.

Tasks:

- [x] Reproduce the same progress-bar/dashboard approach used in Universal GNSS.
- [x] Adapt its source-of-truth mapping to the MowgliMAVROS TODO/audit structure.
- [x] Keep generated README state deterministic.
- [x] Avoid manually maintained duplicate progress state.
- [x] Display the project progress prominently near the top of README.
- [x] Add/update validation so generated progress cannot silently become stale.

Acceptance:

- Progress is derived from canonical project state.
- Re-running generation without state changes produces no diff.
- README immediately exposes current project advancement.
- Behaviour remains consistent with the Universal GNSS implementation.

Evidence: `tools/update_readme_progress.py` derives a bounded README dashboard and SVG directly from current `TODO.md` status fields. It fails closed on missing, duplicate, unknown, or malformed immediate item statuses; `--check` rejects stale generated output. `HARDWARE_PENDING` remains orthogonal to software `DONE`, so it does not count as verified complete.

---

# Phase 4 — Build tooling and CI

## MM-401 — Parameterize build.sh
Status: DONE

Tasks:

- [x] Accept `ROS_DISTRO`.
- [x] Accept MAVROS version/pin where appropriate.
- [x] Keep Kilted as default.
- [x] Preserve multiarch support.
- [x] Keep invocation simple for local development.

Acceptance:

- One script can build either supported ROS distribution.

---

## MM-402 — Add Kilted/Lyrical CI matrix
Status: DONE

Target matrix:

```text
Kilted  × amd64
Kilted  × arm64
Lyrical × amd64
Lyrical × arm64
```

Tasks:

- [x] Separate layer-cache scopes by ROS distro, MAVROS version and architecture/platform.
- [x] Configure ccache cache-mount persistence with Cache Dance. The key is `ccache-${runner.os}-${ROS_DISTRO}-${architecture}-${MAVROS_VERSION}-${MAVROS_COMMIT}-${Dockerfile hash}` and the mount id is `ccache-${ROS_DISTRO}-${TARGETARCH}-${MAVROS_VERSION}`; all four real matrix jobs restored and injected their isolated cache mounts in run 34483209217.
- [x] Keep Kilted as primary/default image.
- [x] Publish Lyrical using an explicit distro tag.
- [x] Do not hide one failing matrix entry behind successful others (the real `fail-fast: false` matrix passed all four independent jobs).

Acceptance:

- [x] All four matrix targets build successfully in GitHub Actions run 34483209217 for `0fcf7444e020e22cd0be11ddab008f277be03f2a`.
- [x] Required production manifests and immutable deployment digests are recorded: Kilted `sha256:04e4eb17b0f5ce38f882f68346b1694774fa87e1945b38b57c94f90da34dd560`, Lyrical `sha256:9fa1ab1652d10c50433198f19ced13d7f70e1549ae3a98dcc1184b06fbfe2cb6`; each contains amd64 and arm64.

---

## MM-403 — Add reproducible development container
Status: DONE (Kilted validation; Lyrical remains an anticipated target)

Tasks:

- [x] Keep the development container separate from the runtime image.
- [x] Default to Kilted while exposing `ROS_DISTRO` for a future Lyrical build.
- [x] Provide ROS development tooling, GitHub CLI, SSH client, Docker CLI and buildx.
- [x] Use the host Docker socket; do not run a nested daemon.
- [x] Persist GitHub CLI configuration in the `mowglimavros-gh-config` named volume.
- [x] Validate Kilted Docker host access, Git, gh, ROS and colcon.

Notes:

- The ROS base already owns UID/GID 1000 as `ubuntu`; use that user.
- Native `colcon` has no `--version`; validate with
  `colcon --log-base /tmp/colcon-log version-check` when the workspace mount
  is not writable.
- Authenticate with `gh auth login` from the opened devcontainer before the
  controlled GeographicLib release publication.

---

# Phase 5 — Runtime smoke validation

## MM-501 — Kilted runtime smoke test
Status: DONE on amd64 workstation (ARM64/RPi4 deployment validation remains pending)

Verify:

- [x] image starts;
- [x] MAVROS 2.15.1 is active;
- [x] backend launch succeeds;
- [x] expected node(s) exist;
- [x] expected topics exist;
- [x] expected services exist;
- [x] no accidental second hardware backend exists in the isolated smoke test.

No physical actuator operation is required.

---

## MM-502 — Lyrical runtime smoke test
Status: DONE (Lyrical amd64 no-FCU workstation smoke, 2026-09-09; ARM64 and hardware remain pending)
Evidence: Current-tree image `mowgli-mavros-sidecar:lyrical-amd64-mm502` (`sha256:c37c31d0c92c5edcbd4f4e6a01b3dcfca9c18b3cabdb55e6379737c6711a45d7`, amd64) passed an isolated no-network/no-FCU launch. MAVROS, Universal GNSS, ESC wheel, and battery plugins loaded; physical wheel mapping remained safely disabled; diagnostics reported FCU disconnected and backend not ready without Emergency synthesis. Canonical static/event-driven contracts remain locked by MM-607; `/imu/data` and `/battery_state` each had one bridge producer. This is not ARM64, RPi, FCU, VESC, GNSS, or battery hardware validation.

Same acceptance criteria as MM-501.

---

# Phase 6 — Backend contract before MowgliNext integration

## MM-601 — Freeze external mowgli_mavros contract
Status: TODO

Document the software contract that MowgliNext may depend on:

- [ ] published topics;
- [ ] subscribed topics;
- [ ] message types;
- [ ] remaps;
- [ ] parameters;
- [ ] status semantics;
- [ ] power/battery semantics;
- [ ] command semantics;
- [ ] startup behaviour;
- [ ] shutdown behaviour;
- [ ] reconnect/degraded behaviour;
- [ ] backend selection expectations.
- [ ] exact interface revision and type-fingerprint gate (MM-602).
- [ ] canonical GNSS, wheel-odometry, power, and readiness semantics (MM-603 through MM-606).
- [ ] RTCM publication bound and byte-perfect tests (MM-201).
- [ ] supported image architecture/digest requirements (MM-301 through MM-304 and MM-402).
- [ ] Explicitly classify backend capability parity/absence for:
      `/wheel_ticks`, `/imu/mag_raw`,
      `reboot_board`, `set_firmware_debug`,
      and dig-safety inputs/behaviour.
- [ ] Define truthful MAVROS-backend semantics for
      `Status.firmware_compatible` / preflight compatibility.
      Do not publish `true` merely to bypass the native STM32 guard.
      If the field is intrinsically native-backend-specific,
      record the required MowgliNext backend-aware handling.

Acceptance:

- There is one explicit contract against which `mowgli_hardware` can be compared.
- No hardware-dependent assumption is presented as validated.
- The contract records every intentional capability absence rather than fabricating parity.


## MM-602 — Synchronize exact Mowgli interface contract
Related migration finding: `MN-MAV-003`
Status: DONE

Tasks:

- [x] Consume the required interface subset from pinned MowgliNext revision `e789ccc2ecc249377c385e79b34fb48ff5a90927` using `tools/mowgli_interface_contract.py sync`.
- [x] Replace the uncontrolled local definitions with the pinned generated subset and lock its source/content in `ros2/src/mowgli_interfaces/interface-contract.lock.json`.
- [x] Add a deterministic SHA-256 canonical-content fingerprint gate, executed before the Docker workspace build, plus focused hardware-free tests.

Evidence: `python3 tools/mowgli_interface_contract.py check`, `python3 tools/test_mowgli_interface_contract.py`, and `git diff --check` passed on 2026-09-08. The exact covered direct bridge contract is `Emergency`, `HighLevelStatus`, `Power`, `Status`, `EmergencyStop`, and `MowerControl`; it includes the required `Status.msg` and `HighLevelStatus.msg`.

Acceptance:

- The external image and pinned MowgliNext revision have identical required `Status` and `HighLevelStatus` IDL/type fingerprints.
- Validation fails closed when either interface contract changes.

## MM-603 — Provide canonical public GNSS adapter
Related migration finding: `MN-MAV-004`
Status: DONE (Lyrical software); HARDWARE_PENDING (ARM64/real target runtime acceptance)

Tasks:

- [x] Dedicated MowgliMAVROS `mowgli_gnss` plugin adapts private Universal GNSS status/fix to canonical `/gps/status` and `/gps/fix`; no GNSS publisher or raw projection remains in the hardware bridge, which only consumes status for readiness.
- [x] `GNSS_SOURCE=mavros` enables canonical ownership; `GNSS_MAVROS_SOURCE=gps1|gps2` selects the private receiver. `direct` creates no canonical endpoints. `MAVROS_GPS1_CANONICAL` is only a deprecated consistency guard.
- [x] Map fix/RTK/baseline enums and capability/value flags symbolically; preserve rich available fields and `position_observation_sequence`. Source identity/incarnation invalidate adapter queues internally; the public Mowgli IDL has no corresponding fields.
- [x] Preserve ellipsoidal canonical altitude with a selected-receiver raw altitude side channel inside the plugin. Never remap private MSL altitude silently; unavailable/ambiguous height is NaN. Document correlation limits in `ros2/src/mavros_gnss_adapter/README.md`.
- [x] Invalidate stale positions without advancing their sequence; RTK enrichment cannot refresh position lifetime. Leave unavailable NTRIP/MSM fields for downstream diagnostics enrichment.

Acceptance policy: ROS 2 Lyrical is the current primary/runtime baseline for
MowgliMAVROS and MowgliNext, fully replacing Kilted. The validated Lyrical
software tests satisfy current distro acceptance; no Kilted revalidation is
required. Humble support is planned only after the repository/refactor policy
is completed and is outside MM-603's current scope.

Acceptance tests:

- [x] Lyrical build with MAVROS 2.16.0 and exact Universal GNSS `v0.7.2-rc4` pin `383caba3de94e16167764393d5a4ef046078b015`; symbolic mapping/altitude/freshness tests, unchanged bridge readiness tests, interface lock and external contracts.
- [x] Explicit solution-type refresh preserves MAVLink 2D/3D/DGPS through Universal GNSS into canonical Mowgli `FIX_TYPE_2D_FIX` / `FIX_TYPE_3D_FIX` / `FIX_TYPE_DGPS`; mappings are symbolic because the UG and Mowgli enum numeric values intentionally differ.
- [x] Lyrical rc4 adapter/bridge suite: 48 tests, 0 errors, 0 failures, 0 skipped. Targeted `test_gnss_graph`: 1/1 PASS (29.69 s), covering GPS1/GPS2/direct ownership plus MAVLink fix types 2/3/4 -> canonical 2D/3D/DGPS.
- [ ] `HARDWARE_PENDING`: target ARM64 build and passive deployment acceptance of source selection, ellipsoid pairing under load, receiver RTK/correction diagnostics, stale/no-fix, FCU reconnect/reboot and canonical DDS delivery. No robot/configuration/deployment changes in the 2026-10-03 software task.

Evidence: `.agent/shared/checkpoints/active/MM-GNSS-CANONICAL-ADAPTER-20261003.md`.

## MM-604 — Establish wheel-only odometry contract
Related migration finding: `MN-MAV-005`
Status: TODO

Tasks:

- [x] Remove `/mavros/local_position/odom -> /wheel_odom` as a production assumption.
- [x] Identify the signed `RPM` (#226) path and per-ESC `ESC_TELEMETRY_* .count[]` observation sequence. The latter distinguishes genuine zero-speed VESC telemetry from the stale zeroes synthesized by `AP_RPM_ESC_Telem`.
- [x] Provide the canonical signed wheel-odometry producer as generic external MAVROS plugin `esc_wheel_odometry`. It consumes MAVLink RPM plus selected `ESC_TELEMETRY_* .count[]` slots directly, rejects output until both counters advance and a subsequent RPM message arrives, and publishes `/wheel_odom` without using local position.
- [x] Prevent GPS/EKF-fused local position from feeding back as wheel odometry.

Acceptance: no production path labels fused MAVROS local position as wheel-only odometry.

Architecture: use signed RPM #226 for wheel values and selected
`ESC_TELEMETRY_* .count[]` fields solely as per-ESC genuine-observation
sequences. The stock MAVROS 2.15.1 wheel-odometry plugin remains unsuitable;
the external plugin must not use its unsigned ESC telemetry RPM field.

Software evidence: 20 focused pairing/kinematics tests, Kilted sidecar build,
pluginlib discovery, safe-disabled and synthetic-config no-FCU startup, and
canonical plugin remap passed on 2026-09-09. `git diff --check` passed.

`HARDWARE_PENDING`: VESC command-index configuration and routing (CAN1
`Status.esc_index` 0/1/2 was passively reconfirmed on 2026-09-28;
no disarmed `RawCommand` was observed), `CAN_D1_ESC_OFFSET`,
`ESC_TELEM_MAV_OFS=0`, RPM1/RPM2 masks, installation
signs, gear-ratio calibration, wheel radii, track width, telemetry cadence,
one-VESC loss/reboot, FCU reconnect/reboot, forward/reverse sign, and measured
distance validation.

## MM-605 — Map POWER1 and POWER2 by configured MAVROS instances
Related migration finding: `MN-MAV-006`
Status: DONE (software; HARDWARE_PENDING)

Target installation semantics: `POWER1 = traction`; `POWER2 = dock/charger`.

Tasks:

- [x] Add configurable dock and traction MAVLink instance IDs; never infer meaning from message arrival order or BATT numbering.
- [x] Route dock state to `Power.v_charge`, explicit MAVLink charging state, `Status.is_charging`, and docking-compatible current semantics.
- [x] Route traction state to `Power.v_battery`, SoC, and the sole `/battery_state` producer.
- [x] Define missing/stale instance behavior and current sign convention. Dock-source disappearance while undocked must not appear as traction failure.
- [x] Do not change ArduPilot failsafe parameters in this software item.

Acceptance tests:

- [x] interleaved configured instances; missing dock; missing traction; stale power source; current sign; traction-only `/battery_state` selection.

Software evidence: external `battery_observer` consumes MAVLink `BATTERY_STATUS`
without the stock `sensor_msgs/BatteryState` sentinel/charge-state loss; focused
mapping tests and Kilted sidecar build passed on 2026-09-09. The canonical
producer is `/hardware_bridge/power`; it is published only from genuine
`BATTERY_STATUS` observations. `HARDWARE_PENDING`: actual POWER1/POWER2 IDs,
monitor direction/calibration, charge-state behavior, freshness cadence,
disconnect/reconnect, and physical charge/discharge validation.

## MM-606 — Define observation freshness and backend readiness
Related migration finding: `MN-MAV-008`
Status: DONE (software; HARDWARE_PENDING)

Tasks:

- [x] Separate FCU connection, transport liveness, last genuine observation, observation freshness, cached publication, and backend readiness.
- [x] Stop timer callbacks from assigning a new observation stamp to cached status or power data.
- [x] Require the FCU and all required fresh streams for readiness; a running container alone is liveness, not readiness.
- [x] Invalidate cached observations across reconnect and FCU reboot.

Acceptance: cached publication cannot appear newly observed; readiness becomes false for stale/disconnected required inputs.

Software evidence: an internal current-FCU-generation readiness state requires
MAVROS connection, a new valid Universal GNSS observation sequence, fresh
canonical wheel odometry, and a fresh valid configured traction observation.
Dock power is surfaced only as non-blocking `/diagnostics` information.
Focused readiness/power tests, pinned Kilted sidecar build, MM-602 interface
contract check, and no-FCU diagnostics smoke passed on 2026-09-09.

`HARDWARE_PENDING`: live MAVROS reconnect/reboot timing; Universal GNSS,
VESC-wheel, and battery observation cadence/loss behavior on the target FCU;
and target DDS diagnostics delivery.

## MM-607 — Add focused external-backend contract tests
Status: DONE (software scope; physical IMU calibration and FCU timing remain HARDWARE_PENDING)

Tasks:

- [x] Consolidate deterministic external tests for MM-602 through MM-606 and MM-201 without relying only on topic discovery.
- [x] Assert exact type/interface fingerprints, semantics, ownership, and failure behavior at the external backend boundary.
- [x] Keep hardware-dependent delivery/actuator proof in MM-801 rather than substituting software fixtures for it.
- [x] Validate the public `/imu/data` contract against exact MowgliNext
      `e789ccc2ecc249377c385e79b34fb48ff5a90927` consumers and exact MAVROS
      2.16.0 `5c68b905ab30de6ce630822dc46c33467e8f23ea` semantics.
- [x] Prove that relayed MAVROS IMU data is compatible with all relevant
      current MowgliNext consumers before declaring contract parity.

Software evidence: `tools/test_external_backend_contract.py` locks the exact
external pins, MM-602 lock linkage, canonical remaps/types/frames, plugin
exports, fail-closed defaults, ownership, readiness boundaries, direct IMU
relay/remap, and forbidden legacy paths. It runs in the Kilted workspace
Docker stage. No-FCU launch confirms the external plugins instantiate; focused
C++ regression tests cover wheel, power, readiness, and RTCM.

IMU evidence: MAVROS publishes `/mavros/imu/data` as BEST_EFFORT/volatile/
KEEP_LAST(5), with synchronized FCU timestamps and aircraft-FRD to base-link
FLU/ENU conversion; the bridge relays the message unchanged and republishes
reliable/volatile/KEEP_LAST(10) `/imu/data`. Exact MowgliNext consumers use
the header timestamp and `angular_velocity.z`, or linear acceleration in the
base frame; they do not require an identity orientation or a particular
covariance. Their BEST_EFFORT subscriptions are compatible with the reliable
relay. Runtime inspection of the no-FCU sidecar confirms both QoS endpoints.

HARDWARE_PENDING: verify installed IMU orientation/mount, accelerometer
calibration/gravity direction, FCU time synchronization, and live FCU cadence
under Rover operation. These are not inferred from the software contract.

Acceptance: interface, GNSS, wheel-odometry, power, freshness/readiness, and RTCM acceptance criteria owned by MM-201 and MM-602 through MM-606 pass.

---

# Phase 7 — MowgliNext integration audit

## MM-701 — MowgliNext integration dependency and execution plan
Related migration findings: `MN-MAV-001` through `MN-MAV-008`
Status: IN PROGRESS (bounded no-motion Lyrical integration deployed; production acceptance blocked)

Audit provenance: the MowgliNext migration audit is complete and retained at `.agent/shared/checkpoints/retained/MAVROS_EXTERNAL_BACKEND_MIGRATION.md`. Operational enablement must not begin until the external prerequisites below are accepted.

Tasks:

- [x] Complete the MowgliNext migration audit: consumers, bringup, selection, topology, status/power/failure, and ownership gaps are retained in the imported audit.
- [x] Record the eventual integration plan: suppress native `mowgli_hardware` only when `backend=mavros`; use one external sidecar; remove separate historical NTRIP; select validated Pixhawk USB `if00`; and prove exclusive backend ownership.
- [ ] Pass MM-602 interface fingerprint, MM-603 canonical GNSS, MM-604 wheel-only odometry, MM-605 power-instance mapping, MM-606 freshness/readiness, MM-201 RTCM, and MM-607 focused tests.
- [ ] Provide a suitable validated multiarch image/digest under MM-301 through MM-304 and MM-402.
- [x] Authorized bounded no-motion MowgliNext Lyrical graph/GUI validation with temporary NEO-M9N GPS1, exclusive canonical topic ownership and neutral-only wire capture (2026-09-29).
- [ ] Replace temporary `GNSS_STACK=disabled` cutover with nominal `GNSS_STACK=universal` plus a Universal GNSS `mavros` backend, preserving exclusive `/gps/fix` and `/gps/status` ownership.
- [ ] Validate the production multiarch image/digest, final RTCM path and physical safety/actuation gates before operational enablement.

Preferred model:
high-capability audit/reasoning model.

Current evidence: `.agent/shared/checkpoints/active/MM-MOWGLINEXT-INTEGRATION-20260929.md`. Full Kilted amd64 and Lyrical ARM64 images, hashes/QoS, NEO-M9N fix, POWER/VESC/IMU, GUI diagnostics and zero MANUAL_CONTROL passed live. No ARM or movement. This does not close MM-201, calibrated wheel odometry, production publication, HERE4 or MM-801.

Blocked for operational enablement by: nominal Universal GNSS MAVROS transport/RTCM, wheel calibration, power-current/SoC validation, hardware E-stop and MM-801 guarded physical tests.

---

# Phase 8 — Hardware validation

## MM-801 — Pixhawk / ArduPilot bench validation
Related finding: `MM-AUD-007`
Status: BLOCKED

Blocked by:

- stable software backend;
- controlled Pixhawk/ArduPilot test availability.

Validate individually:

- [ ] `manual_control` mapping;
- [ ] throttle;
- [ ] steering;
- [ ] blade behaviour;
- [ ] charging behaviour;
- [ ] HOLD behaviour;
- [ ] disarm behaviour;
- [ ] status feedback;
- [ ] power feedback;
- [ ] failsafe behaviour;
- [ ] transport reconnect;
- [ ] reboot/power-cycle recovery.

2026-09-29 no-motion Lyrical follow-up: POWER1 id0 reports +0.09 to +1.48 A off dock with `not_charging`; canonical charge current is therefore negative. POWER2 id1 reports ~28.8 V and ~0.47 A discharge while SoC fell to ~3%. Preserve the physical POWER1/POWER2 mapping; verify sensor zero/direction, dock backfeed and SoC calibration with independent measurements before any active test. Three ESC counters progress at zero RPM; slot3 remains empty. GPS1 NEO-M9N is a temporary standard-GNSS source, not RTK. See the active integration checkpoint for exact image and topic evidence.

2026-09-30 contract correction (source only, not deployed): `Power.charge_current` is computed from fresh raw POWER1 minus fresh raw POWER2, otherwise NaN; only the resultant is displayed, while raw paths remain internal diagnostics. Per operator, fresh POWER1 voltage >0 V indicates dock/charging; the MAVLink charge-state enum remains raw diagnostic data (live `1` means OK). MowgliNext battery percent is an approximate estimate from filtered POWER2 voltage; GUI shows unavailable without a valid voltage, while preserving a real 0%. On-dock capture: POWER1 id0 ~+1.5 A, POWER2 id1 ~+0.5 A. Historical off-dock positive POWER1 voltage conflicts with the new rule and needs four-state physical verification: off dock, dock attached without charge, active charge, POWER1 disconnected. No FCU parameter changes.

Hardware acceptance gates retained from the migration audit:

- [ ] `HW-MAV-001` — ARM64/RPi4 USB `if00` deployment, reconnect, and FCU reboot recovery (`HARDWARE_PENDING`).
- [ ] `HW-MAV-002` — steering/throttle plus zero, HOLD, disarm, DDS, and USB-loss stop semantics (`HARDWARE_PENDING`).
- [ ] `HW-MAV-003` — physical POWER1 traction and POWER2 dock behavior (`HARDWARE_PENDING`).
- [ ] `HW-MAV-004` — blade command/feedback and emergency authority (`HARDWARE_PENDING`; separate blade-on authorization required).
- [ ] `HW-MAV-005` — outdoor GNSS/RTCM and HERE4 CAN1/VESC coexistence (`HARDWARE_PENDING`).

Follow `.agent/policies/HARDWARE.md`.

Do not infer these results from software tests.

Passive Rock 5B/Pixhawk snapshot (2026-09-28):
`.agent/shared/checkpoints/blocked/MM-PIXHAWK-PASSIVE-AUDIT-20260928.md`.
The 1,014 freshly read FCU parameters and runtime show a possible right-wheel
DroneCAN command/telemetry index mismatch, `RPM=-1/-1` despite masks `1/2`
with `RPMx_TYPE=7`, Battery 1 unhealthy, no valid GPS, and backend
`not_ready`. These are pre-active-test gates; this passive audit closes no
MM-801 hardware acceptance checkbox.

2026-09-28 POWER1 follow-up: after the operator restored its controller,
`BATTERY_STATUS id0` again had valid 24.78–24.81 V and 1.51–1.52 A readings;
MAVROS Battery and FCU battery/pre-arm diagnostics were OK. This supersedes
the earlier POWER1-unavailable observation, but current direction, charging
semantics, bridge instance configuration and all active hardware gates remain
pending. See the follow-up in the same checkpoint.

2026-09-28 CAN1 follow-up: an authorized, temporary 22 s MAVLink forwarding
capture reconfirmed VESC CAN nodes 1/2/3 with Status indexes 0/1/2, zero RPM
and current, and no disarmed RawCommand. Forwarding was explicitly disabled
and a later 7 s check saw no CAN_FRAME. The historical right-wheel
Status-index-1 association was superseded by a later isolated manual-wheel
capture (current right wheel = index 0); command routing still needs proof.
See the follow-up in the same checkpoint.

2026-09-28 MANUAL_CONTROL preflight on the operator-secured stand: no new
motor command was sent. FCU `SYSID_MYGCS=255` versus MAVROS `system_id=1`
causes exact Rover 4.6.3 manual-control source rejection; Rover reads `y`
steering and `z` throttle while the current disabled bridge writes `x` and
`r`. Reconcile identity and correct bridge axes before the bounded active
manual-control test, without altering either while armed. Earlier disarmed
direct-CAN index-1 trials at 5%/20% produced zero ESC RPM/current and no
observed motion, but physical CAN delivery is unproven. See
`.agent/shared/checkpoints/blocked/MM-MANUAL-CONTROL-ROUTING-20260928.md`.

2026-09-28 correction: the operator confirmed DISARMED/MANUAL and authorized
sidecar rebuild/restart. The ARM64 sidecar now emits MAVROS source sysid 255
(observed outgoing heartbeat) with FCU target 1; Pixhawk `SYSID_MYGCS` remains
255. The bridge maps Rover steering/throttle to `y/z`; its focused test and
all 25 bridge GTests pass. `manual_control_enabled=false` remains in effect,
and no motor command was sent. Physical neutral/stop, CAN RawCommand and motor
routing are still `HARDWARE_PENDING`; request fresh authorization before the
first bounded MANUAL motor test. See the same checkpoint's deployed follow-up.

2026-09-28 first authorized 5% MANUAL attempt aborted before positive throttle:
60 neutral MANUAL_CONTROL messages were observed on wire as 255:191 to target 1.
The capture script rejected short MAVLink 2 CAN_FRAME payloads; its guard
stopped the run, sent neutral for 2 s, and disabled forwarding. The decoder
was corrected and a separate passive 5 s capture verified 741 ESC Status
packets from nodes 1/2/3. A 7 s postcheck saw zero CAN forwarding frames.
No propulsion was sent; physical routing remains untested and requires a fresh GO.

2026-09-28 bounded MANUAL follow-up: separate operator GOs allowed 5% then 20%
ArduRover throttle (`MANUAL_CONTROL.z=50/200`, `y=0`) for 10 s each, with
observed source 255/target 1, explicit 2 s neutral and acknowledged CAN
forwarding disable. The operator saw **no motor move** in either trial. At 20%,
all three ESC Status sources and MAVROS telemetry stayed 0 RPM/0 A. No
RawCommand appeared in the forwarded CAN receive stream, which does not prove
absence of CAN TX. The 5% node-1 RPM arose amid operator manual wheel turns;
its corrected maximum was 630 RPM, not the initial erroneous ±130k decode.
An independent 7 s check after each trial found zero CAN_FRAME/CANFD_FRAME,
FCU still armed/MANUAL. A later passive 20 s capture, with operator-confirmed
**right wheel only** manual rotation, gave ROS ESC slot 0: 26/60 samples
nonzero up to 1,038 RPM, while slots 1/2 stayed 0. Thus **current right-wheel
telemetry is ESC1 / CAN node 1 / esc_index 0**, contradicting the older ESC2 /
index 1 association. Treat the old mapping as superseded for current hardware;
reason for the change is unknown. A separate operator-confirmed left-wheel-only hand-rotation at comparable
speed had 60 samples per slot over 20 s, all 0 RPM/0 A, so its ESC identity
remains unresolved.
Physical command routing and Rover output response remain open. Do not increase throttle again merely to probe this.
See `.agent/shared/checkpoints/blocked/MM-MANUAL-CONTROL-ROUTING-20260928.md`.


2026-09-28 Rover 4.7.1 wheel-only follow-up: the operator authorized one
new bounded trial on a stand, with separate POWER1 dock/charger and POWER2
traction capture. Live parameters are now SERVO1/2/3 functions 74/73/35;
the older 4.6.3 servo order above is historical. The FCU refused the single
arming request (MAVLink result 4), explicitly reporting "Arm: Compass 2 not
found" and "Arm: DroneCAN: Node 124 unhealthy!". The guard sent 43 neutral
MANUAL_CONTROL messages over 2.21 s; **zero positive propulsion messages**
were sent. Final state: connected, MANUAL, disarmed. Wheel response, direction,
traction draw under load, and mower isolation under command remain untested.
Diagnose the FCU pre-arm conditions before a separately authorized retry;
do not bypass checks. See the new blocked wheel-trial checkpoint.

2026-09-28 HERE4 CAN2 passive follow-up: node 124 responded consistently at
1 Hz on Pixhawk CAN2, with six complete GetNodeInfo responses, so CAN
communication is functioning at the FCU's 1 Mbit/s setting. The node stayed
in `MODE_MAINTENANCE` with software version `2.0`, status code `13`, and no
GNSS/compass broadcasts. ArduPilot's DroneCAN bootloader reports version 2.0
and uses code 13 for a failed application CRC check, making an application
boot/firmware-image problem the leading diagnosis; the exact CubePilot build
and failure history remain unconfirmed. The capture found no 30 s node reboot.
Receive forwarding was explicitly stopped. No parameter or persistent hardware
configuration was changed. HERE4 recovery and sensor output must be proven
before HW-MAV-005 or another ARM attempt. See `MM-HERE4-CAN2-20260928`.

2026-09-29 wheel-only preflight: live Rover 4.7.1/FCU parameters, MANUAL/disarmed
state, RC neutral and both POWER instances were captured before any active
command. All three ESC telemetry slots were absent for 10 s; a separate 12 s
raw FCU MAVLink capture contained zero `ESC_TELEMETRY_1_TO_4` despite normal
heartbeats, battery and servo messages. The trial was held before ARM because
the mower ESC could not be monitored. The operator confirmed all three VESCs
are powered; inspect the CAN1 physical path and VESC diagnostics, then repeat
the complete preflight before the one authorized wheel trial.
See `MM-WHEEL-TRIAL-PREFLIGHT-20260929`.

2026-09-29 CAN1 reconnection and single wheel-trial retry: three ESC telemetry
slots and advancing counts returned; POWER1 was unavailable off dock as
reported by the operator, while POWER2 remained valid. The single ARM request
was refused with exact FCU text `Arm: Battery 1 unhealthy` (MAVLink result 4).
The guard sent 44 neutral MANUAL_CONTROL messages over 2.243 s; no positive
throttle was sent, and the FCU remained MANUAL/disarmed. `BATT_FS_LOW_ACT=0`
already disables the low-battery action, but the configured POWER1 monitor
still fails the separate arming-health check. Choose an off-dock POWER1
monitoring strategy that preserves POWER2 protection before a separately
authorized parameter change and wheel-trial retry. See the same checkpoint.

2026-10-01 POWER failover resolution: repeated live tests showed that the
previous wiring, POWER1=dock/charger and POWER2=traction, caused a full Pixhawk
5X reboot when the dock/POWER1 source was removed while traction remained
available on POWER2. Linux observed USB disconnect, Pixhawk5X-BL enumeration,
then a normal Pixhawk5X restart. The physical inputs were therefore swapped.
The validated production wiring is now POWER1 / BATTERY_STATUS id0 = traction
and POWER2 / BATTERY_STATUS id1 = dock/charger. With this wiring, removing the
dock/POWER2 source leaves the FCU and MAVLink transport uninterrupted. The
battery-observer semantic mapping is correspondingly dock_battery_instance=1
and traction_battery_instance=0. Earlier dated POWER1=dock / POWER2=traction
observations above are retained as historical evidence and are superseded by
this hardware validation.

---

# Deferred / low priority

## MM-901 — Remove tracked Python bytecode
Related finding: `MM-AUD-008`
Status: DEFERRED

- [ ] Remove tracked `.pyc`.
- [ ] Ensure generated Python bytecode is ignored.

Do not include this in the primary compatibility patch unless it becomes relevant.

---

# Current execution order

1. Finish source-compatibility and build-tooling implementation
   required to support subsequent work...: `MM-102`, `MM-103`, `MM-301` through `MM-304`, `MM-401`, and `MM-402`.
2. Synchronize exact Mowgli interfaces and pass the fingerprint gate: `MM-602`.
3. Provide canonical GNSS observation identity and receiver semantics: `MM-603`.
4. Establish the wheel-only odometry contract: `MM-604`.
5. Implement POWER1/POWER2 instance mapping: `MM-605`.
6. Define freshness and readiness semantics: `MM-606`.
7. Implement and test bounded RTCM chunking: `MM-201`.
8. Run focused external contract tests: `MM-607`.
9. Re-run the completed build pipeline against the finalized
   external contract and close multiarch publication acceptance: `MM-301` through `MM-304`, `MM-401`, and `MM-402`.
10. Run external runtime smoke/contract validation: `MM-501` and `MM-502`.
11. Freeze the external backend contract: `MM-601`.
12. Return to MowgliNext integration and no-motion DDS/graph validation: `MM-701`.
13. Execute physical hardware gates: `MM-801` / `HW-MAV-001` through `HW-MAV-005`.
14. Deferred cleanup only when separately prioritized: `MM-901`.

Only advance to the next expensive validation layer when the previous one passes.
