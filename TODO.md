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

Current problem:

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

---

## MM-102 — Make CMake compatible with Lyrical
Related findings: `MM-AUD-003`, `MM-AUD-006`
Status: TODO

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
Status: TODO

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
  production MAVROS remains pinned to 2.15.1 / 22ae5b7c.
- physical RTCM delivery remains HARDWARE_PENDING under HW-MAV-005.
---

# Phase 3 — Reproducible MAVROS container

## MM-301 — Parameterize ROS distribution
Related finding: `MM-AUD-005`
Status: IN PROGRESS

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
- Kilted and Lyrical amd64 runtime images passed on 2026-09-07 with the same modern CMake source; arm64 remains pending.
- A production image is published for each project-required Kilted/Lyrical architecture and its immutable deployment digest is recorded; an amd64 workstation image is not ARM64/RPi evidence.

---

## MM-302 — Pin MAVROS 2.15.1
Related finding: `MM-AUD-002`
Status: IN PROGRESS

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
Status: IN PROGRESS

Tasks:

- [x] Stop fetching installation logic from a moving upstream branch.
- [x] Avoid installing datasets twice.
- [x] Determine exactly which datasets/runtime files MAVROS requires.
- [x] Install/copy them once through the multi-stage build (Kilted amd64 validated against release `geographiclib-datasets-v1`).
- [ ] Preserve amd64 + arm64 compatibility.

Acceptance:

- GeographicLib setup is deterministic.
- Kilted and Lyrical amd64 image builds passed on 2026-09-07; arm64 remains pending.
- No duplicate network-heavy installation occurs.
- Release `geographiclib-datasets-v1`, all four assets, approved SHA256 values,
  and cached source-only rebuild behaviour are validated.

---

## MM-304 — Remove Noble-specific runtime ABI assumptions
Related finding: `MM-AUD-006`
Status: IN PROGRESS

Tasks:

- [ ] Remove hard-coded `libboost-system1.83.0` if unnecessary.
- [ ] Use distro-portable dependency resolution.
- [ ] Verify both ROS distributions.

Acceptance:

- Docker dependency installation succeeds on Kilted and Lyrical.

Evidence: Kilted and Lyrical amd64 runtime dependency installation passed on
2026-09-07. The Lyrical build selected `libboost-system1.83-dev` at build time
and `libboost-system1.83.0` at runtime.

---

### MM-306 — Add UG-style README progress dashboard
Status: TODO

Objective:

Reuse the Universal GNSS progress/status presentation model for MowgliMAVROS.

Tasks:

- [ ] Reproduce the same progress-bar/dashboard approach used in Universal GNSS.
- [ ] Adapt its source-of-truth mapping to the MowgliMAVROS TODO/audit structure.
- [ ] Keep generated README state deterministic.
- [ ] Avoid manually maintained duplicate progress state.
- [ ] Display the project progress prominently near the top of README.
- [ ] Add/update validation so generated progress cannot silently become stale.

Acceptance:

- Progress is derived from canonical project state.
- Re-running generation without state changes produces no diff.
- README immediately exposes current project advancement.
- Behaviour remains consistent with the Universal GNSS implementation.

---

# Phase 4 — Build tooling and CI

## MM-401 — Parameterize build.sh
Status: IN PROGRESS

Tasks:

- [ ] Accept `ROS_DISTRO`.
- [ ] Accept MAVROS version/pin where appropriate.
- [ ] Keep Kilted as default.
- [ ] Preserve multiarch support.
- [ ] Keep invocation simple for local development.

Acceptance:

- One script can build either supported ROS distribution.

---

## MM-402 — Add Kilted/Lyrical CI matrix
Status: IN PROGRESS

Target matrix:

```text
Kilted  × amd64
Kilted  × arm64
Lyrical × amd64
Lyrical × arm64
```

Tasks:

- [x] Separate layer-cache scopes by ROS distro, MAVROS version and architecture/platform.
- [~] Configure ccache cache-mount persistence with Cache Dance. The key is `ccache-${runner.os}-${ROS_DISTRO}-${architecture}-${MAVROS_VERSION}-${MAVROS_COMMIT}-${Dockerfile hash}` and the mount id is `ccache-${ROS_DISTRO}-${TARGETARCH}-${MAVROS_VERSION}`. Static YAML/key checks pass; two real CI runs remain pending until commit/push.
- [ ] Keep Kilted as primary/default image.
- [ ] Publish Lyrical using an explicit distro tag.
- [ ] Do not hide one failing matrix entry behind successful others.

Acceptance:

- All four matrix targets build successfully.
- Required production image manifests and immutable deployment digests are recorded for Kilted amd64/arm64 and, while project policy requires it, Lyrical amd64/arm64.

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
Status: TODO

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

- [x] Consume the required interface subset from pinned MowgliNext revision `3b0974809809ace589a567b6c32e5bed6e599489` using `tools/mowgli_interface_contract.py sync`.
- [x] Replace the uncontrolled local definitions with the pinned generated subset and lock its source/content in `ros2/src/mowgli_interfaces/interface-contract.lock.json`.
- [x] Add a deterministic SHA-256 canonical-content fingerprint gate, executed before the Docker workspace build, plus focused hardware-free tests.

Evidence: `python3 tools/mowgli_interface_contract.py check`, `python3 tools/test_mowgli_interface_contract.py`, and `git diff --check` passed on 2026-09-08. The exact covered direct bridge contract is `Emergency`, `HighLevelStatus`, `Power`, `Status`, `EmergencyStop`, and `MowerControl`; it includes the required `Status.msg` and `HighLevelStatus.msg`.

Acceptance:

- The external image and pinned MowgliNext revision have identical required `Status` and `HighLevelStatus` IDL/type fingerprints.
- Validation fails closed when either interface contract changes.

## MM-603 — Provide canonical public GNSS adapter
Related migration finding: `MN-MAV-004`
Status: TODO

Tasks:

- [ ] Publish current MowgliNext `/gps/fix` and `/gps/status` contracts; raw MAVROS topic remaps are not receiver availability.
- [ ] Make GPS1/GPS2 and selected-receiver selection explicit and configurable.
- [ ] Preserve `source_id`, `source_incarnation`, and `position_observation_sequence`; increment only for a genuine new selected-receiver sample.
- [ ] Define fix/status pairing, cached delivery, freshness/liveness, invalid-input, RTK, reconnect, and FCU-reboot invalidation semantics.
- [ ] Do not fabricate receiver-native diagnostics unavailable from MAVROS.

Acceptance tests:

- [ ] receiver selection; observation identity and new-versus-cached delivery; reconnect/source-incarnation invalidation; RTK mapping; invalid and stale GNSS inputs.

## MM-604 — Establish wheel-only odometry contract
Related migration finding: `MN-MAV-005`
Status: TODO

Tasks:

- [ ] Remove `/mavros/local_position/odom -> /wheel_odom` as a production assumption.
- [ ] Identify and provide a proven wheel-only source, or document intentional absence and affected consumer contract.
- [ ] Prevent GPS/EKF-fused local position from feeding back as wheel odometry.

Acceptance: no production path labels fused MAVROS local position as wheel-only odometry.

## MM-605 — Map POWER1 and POWER2 by configured MAVROS instances
Related migration finding: `MN-MAV-006`
Status: TODO

Target installation semantics: `POWER1 = dock/charger`; `POWER2 = traction`.

Tasks:

- [ ] Add configurable dock and traction `BatteryState.location=idN` instance IDs; never infer meaning from message arrival order or BATT numbering.
- [ ] Route dock state to `Power.v_charge`, charger state, `Status.is_charging`, and docking-compatible current semantics.
- [ ] Route traction state to `Power.v_battery`, SoC, and the sole battery-failsafe source.
- [ ] Define missing/stale instance behavior and current sign convention. Dock POWER1 disappearance while undocked must not appear as traction failure.
- [ ] Do not change ArduPilot failsafe parameters in this software item.

Acceptance tests:

- [ ] interleaved `id0`/`id1`; missing dock; missing traction; stale power source; current sign; traction-only failsafe-source selection.

## MM-606 — Define observation freshness and backend readiness
Related migration finding: `MN-MAV-008`
Status: TODO

Tasks:

- [ ] Separate FCU connection, transport liveness, last genuine observation, observation freshness, cached publication, and backend readiness.
- [ ] Stop timer callbacks from assigning a new observation stamp to cached status or power data.
- [ ] Require the FCU and all required fresh streams for readiness; a running container alone is liveness, not readiness.
- [ ] Invalidate cached observations across reconnect and FCU reboot.

Acceptance: cached publication cannot appear newly observed; readiness becomes false for stale/disconnected required inputs.

## MM-607 — Add focused external-backend contract tests
Status: TODO

Tasks:

- [ ] Consolidate deterministic external tests for MM-602 through MM-606 and MM-201 without relying only on topic discovery.
- [ ] Assert exact type/interface fingerprints, semantics, ownership, and failure behavior at the external backend boundary.
- [ ] Keep hardware-dependent delivery/actuator proof in MM-801 rather than substituting software fixtures for it.
- [ ] Validate the public `/imu/data` contract:
      frame, axis convention, timestamp provenance,
      covariance/calibration semantics and required QoS.
- [ ] Prove that relayed MAVROS IMU data is compatible with
      current MowgliNext consumers before declaring contract parity.

Acceptance: interface, GNSS, wheel-odometry, power, freshness/readiness, and RTCM acceptance criteria owned by MM-201 and MM-602 through MM-606 pass.

---

# Phase 7 — MowgliNext integration audit

## MM-701 — MowgliNext integration dependency and execution plan
Related migration findings: `MN-MAV-001` through `MN-MAV-008`
Status: BLOCKED

Audit provenance: the MowgliNext migration audit is complete and retained at `.agent/shared/checkpoints/retained/MAVROS_EXTERNAL_BACKEND_MIGRATION.md`. Operational enablement must not begin until the external prerequisites below are accepted.

Tasks:

- [x] Complete the MowgliNext migration audit: consumers, bringup, selection, topology, status/power/failure, and ownership gaps are retained in the imported audit.
- [x] Record the eventual integration plan: suppress native `mowgli_hardware` only when `backend=mavros`; use one external sidecar; remove separate historical NTRIP; select validated Pixhawk USB `if00`; and prove exclusive backend ownership.
- [ ] Pass MM-602 interface fingerprint, MM-603 canonical GNSS, MM-604 wheel-only odometry, MM-605 power-instance mapping, MM-606 freshness/readiness, MM-201 RTCM, and MM-607 focused tests.
- [ ] Provide a suitable validated multiarch image/digest under MM-301 through MM-304 and MM-402.
- [ ] Only then begin no-motion MowgliNext enablement and DDS/graph validation.

Preferred model:
high-capability audit/reasoning model.

Blocked by: MM-201, MM-301 through MM-304, MM-402, and MM-602 through MM-607.

Unblocks when: the listed software prerequisites pass. MM-801 remains a separate `HARDWARE_PENDING` operational gate.

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

Hardware acceptance gates retained from the migration audit:

- [ ] `HW-MAV-001` — ARM64/RPi4 USB `if00` deployment, reconnect, and FCU reboot recovery (`HARDWARE_PENDING`).
- [ ] `HW-MAV-002` — steering/throttle plus zero, HOLD, disarm, DDS, and USB-loss stop semantics (`HARDWARE_PENDING`).
- [ ] `HW-MAV-003` — physical POWER1 dock and POWER2 traction behavior (`HARDWARE_PENDING`).
- [ ] `HW-MAV-004` — blade command/feedback and emergency authority (`HARDWARE_PENDING`; separate blade-on authorization required).
- [ ] `HW-MAV-005` — outdoor GNSS/RTCM and HERE4 CAN1/VESC coexistence (`HARDWARE_PENDING`).

Follow `.agent/policies/HARDWARE.md`.

Do not infer these results from software tests.

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
