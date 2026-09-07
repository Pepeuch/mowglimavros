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
Status: TODO

- [ ] Create/update shared retained audit checkpoint.
- [ ] Create/update active compatibility checkpoint.
- [ ] Create/update blocked Pixhawk validation checkpoint.
- [ ] Update shared checkpoint index.
- [ ] Verify `git diff --check`.

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
Status: TODO

Current problem:

- local integration requests `apm.launch.py` / `px4.launch.py`;
- MAVROS 2.15.1 installs `apm.launch` / `px4.launch`.

Tasks:

- [ ] Correct MAVROS launch inclusion.
- [ ] Verify expected namespace behaviour.
- [ ] Verify APM launch path.
- [ ] Verify PX4 launch path if retained/supported.
- [ ] Add focused static/launch validation where practical.

Acceptance:

- MAVROS launch resolution succeeds.
- No duplicate or unintended namespace is introduced.

---

## MM-102 — Make CMake compatible with Lyrical
Related findings: `MM-AUD-003`, `MM-AUD-006`
Status: TODO

Tasks:

- [ ] Replace obsolete `ament_target_dependencies()`.
- [ ] Raise only required `cmake_minimum_required()` values.
- [ ] Remove/replace distro-specific Boost assumptions.
- [ ] Address `CMP0167` only where required by compatibility.
- [ ] Keep the same C++ source for Kilted and Lyrical.
- [ ] Avoid ROS-distro `#ifdef` unless proven unavoidable.

Acceptance:

- Kilted amd64 workspace build passes.
- Lyrical amd64 workspace build passes.
- No compatibility regression is introduced on Kilted.

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

- [ ] Remove hard-coded `kilted` image references.
- [ ] Remove hard-coded `/opt/ros/kilted`.
- [ ] Remove hard-coded `ros-kilted-*`.
- [ ] Preserve Kilted as default.
- [ ] Support Lyrical through build argument.

Acceptance:

- Same Dockerfile builds for Kilted and Lyrical.

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

- [ ] Build MAVROS from the pinned upstream source.
- [ ] Verify tag resolves to expected commit.
- [ ] Fail build if expected MAVROS version differs.
- [ ] Use the same MAVROS version on Kilted and Lyrical.
- [ ] Pin or explicitly verify MAVLink dependency.

Acceptance:

- Runtime reports MAVROS 2.15.1.
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
- [~] Install/copy them once through the multi-stage build (blocked pending release assets).
- [ ] Preserve amd64 + arm64 compatibility.

Acceptance:

- GeographicLib setup is deterministic.
- No duplicate network-heavy installation occurs.

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

- [ ] Separate caches by ROS distro.
- [ ] Separate caches by MAVROS version.
- [ ] Separate caches by architecture/platform.
- [ ] Keep Kilted as primary/default image.
- [ ] Publish Lyrical using an explicit distro tag.
- [ ] Do not hide one failing matrix entry behind successful others.

Acceptance:

- All four matrix targets build successfully.

---

# Phase 5 — Runtime smoke validation

## MM-501 — Kilted runtime smoke test
Status: TODO

Verify:

- [ ] image starts;
- [ ] MAVROS 2.15.1 is active;
- [ ] backend launch succeeds;
- [ ] expected node(s) exist;
- [ ] expected topics exist;
- [ ] expected services exist;
- [ ] no accidental second hardware backend exists.

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

Acceptance:

- There is one explicit contract against which `mowgli_hardware` can be compared.
- No hardware-dependent assumption is presented as validated.

---

# Phase 7 — MowgliNext integration audit

## MM-701 — Audit MowgliNext against MAVROS backend contract
Status: TODO

Do not begin until Phase 6 provides a stable software contract.

Tasks:

- [ ] Compare `mowgli_hardware` and `mowgli_mavros_bridge`.
- [ ] Identify every consumer of hardware-facing topics/services.
- [ ] Inspect bringup.
- [ ] Inspect backend selection/configuration.
- [ ] Inspect `.env`/container integration.
- [ ] Inspect launch conditions.
- [ ] Detect hidden assumptions about `mowgli_hardware`.
- [ ] Detect duplicate/partial backend activation.
- [ ] Compare status/power/failure semantics.
- [ ] Classify gaps as `SOFTWARE_NOW`, `HARDWARE_PENDING`, or intentional differences.
- [ ] Produce ordered MowgliNext integration work.

Preferred model:
high-capability audit/reasoning model.

Do not implement broad changes during the first MowgliNext audit.

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

1. `MM-000`
2. `MM-101`
3. `MM-102`
4. `MM-103`
5. `MM-201`
6. `MM-301`
7. `MM-302`
8. `MM-303`
9. `MM-304`
10. `MM-401`
11. `MM-402`
12. `MM-501`
13. `MM-502`
14. `MM-601`
15. `MM-701`
16. `MM-801`
17. `MM-901`

Only advance to the next expensive validation layer when the previous one passes.
