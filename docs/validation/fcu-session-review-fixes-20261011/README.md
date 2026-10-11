# FCU adapter: post-review software corrections, 2026-10-11

Repository: `/tmp/mowglimavros-fcu-session`, branch
`feat/fcu-session-controller`, HEAD/base
`e7d516cb0adca1872dd9d10fb66412857d38b0c8`. Uncommitted worktree;
no push, merge, hardware command, deployment or production image publication.
MowgliNext read-only baseline `7748af56acd98449938750714d2286193cf6143a`;
its MowgliMAVROS gitlink remains `e7d516cb0adca1872dd9d10fb66412857d38b0c8`.

## Corrections

- Failed/expired isolated blade ON clears the temporary test flag, cancels
  queued ONs, neutralizes cutting/traction and requests HOLD/disarm. Late ARM
  reconciles to disarmed; the cancelled blade request is never replayed.
  Movement-owned ARM remains active when only blade execution fails.
- OFF cancels queued ONs before any drive function may dispatch them. Existing
  BladeControl permission transitions happen before binding the OFF response
  revision. BladeControl itself and its fault/inversion/stop proofs are unchanged.
- Emergency response is independent of HOLD availability: true if DISARM has
  been queued or the connected FCU is already observed disarmed. This does not
  confirm HOLD, physical stopping or completion. Existing diagnostics distinguish
  pending transport, accepted ACK, and matching observed target state.
- No new ROS service/message, safety policy, authorization, intention protocol
  or watchdog. Existing MowgliNext state remains the movement demand.

Implementation changes for this review are in the node source/header, sequencer
header, CMake test wiring, `test_provider_graph.py`,
`test_fcu_session_controller.cpp` and the new `test_bridge_regressions.cpp`.
`node-review-fixes.diff` shows the node-only correction against the reviewed
pre-fix worktree. `review.diff` will contain the complete final source/doc patch
against HEAD, including previously reviewed implementation and retained
checkpoints, excluding the validation archives themselves to avoid duplication.

## Validation and provenance

Final result: **PASS — 18/18 suites, 106 XML-recorded tests, zero failures,
errors or skipped cases**. Full ctest run: 191.55s. Individual graph scenarios
are grouped in one XML test per graph; the provider graph runs four combinations
(ArduPilot/PX4 × mowing_enabled true/false).

| Suite | Result |
| --- | --- |
| FCU sequencer | 25/25 cases PASS |
| Real-node deterministic OFF callback | 1/1 PASS |
| BladeControl / BladeOffControl | 16/16 + 5/5 PASS |
| SafetyState / readiness | 15/15 + 13/13 PASS |
| Provider, blade, safety ROS graphs | all PASS |
| ESC tracker / ESC graph / GNSS graph | 14/14, graph PASS, graph PASS |
| Provider conversion / MANUAL conversion | 6/6 + 2/2 PASS |
| Firmware/launch/OFF contract checks | all PASS |

Final peer-only source update replaces a deprecated ROS spin helper with
SingleThreadedExecutor; rebuilt and reran that test: PASS, 0.33s, no compiler
warning. No production source changed after the broad PASS run. The broad
build log retains the earlier deprecation warning; the final peer build log
records its correction.

Full touched C++ clang-format18 check, changed-lines check against origin/main,
Python compilation, git diff --check and README progress consistency: PASS.
38 input-file hashes match the actual copied container sources. Executable
build/install hashes are in binaries.sha256 (different install RPATH is normal).
The first run's `initial-tests.log` records an incorrect *new test* expectation
that OFF would report success after an active-session servo refusal. Existing
BladeControl correctly keeps that fault latched. The corrected regression
checks failure reporting, the emitted neutral, and preserved movement ARM.
CMake initially needed explicit `RMW_IMPLEMENTATION=rmw_fastrtps_cpp` for build;
runtime tests use the pre-existing isolated CycloneDDS library.

SDK image SHA256 `d98e8e9899e57cdcfb9b3b1aec5588bd0c64fc31956dcc726ec5c31ca6825316`;
container `mowgli-fcu-session-tests`, UID/GID1000, network none, no devices.
Inputs are copied source files. All native fake-FCU MAVLink traffic is loopback
inside this container. No robot/Pixhawk/VESC endpoint is reachable by these tests.
Final logs, JUnit XML, input/source hashes, executable hashes, build/test script
and detailed result summary are retained alongside this report.

The new graph cases cover isolated ARM refusal and confirmation timeout, late
ARM after either failure, OFF while waiting for ARM, OFF in MANUAL/GUIDED,
servo refusal with/without movement ownership, and emergency DISARM without
HOLD. The new C++ callback regression seeds a ready queued ON using confirmed
FCU state and the real BladeControl's distinct fresh zero-RPM stop proof; an OFF
must emit only neutral PWM, even in that deterministic dispatch-ready window.

## Remaining acceptance

SOFTWARE_REQUIRED in MowgliNext: genuine idle GUI ON currently becomes OFF;
[the exact failure, minimal internal correction and future tests](../../architecture/mowglinext-idle-blade-test-follow-up.md)
are documented. Direct MowerControl injection cannot validate a GUI click.
No MowgliNext source, public interface or submodule pointer was changed.

HARDWARE_REQUIRED: actual servo output in HOLD on the exact FCU/channel setup;
zero physical wheel movement during a blade test; MANUAL/GUIDED signs, scale and
confirmation order; emergency DISARM wire races and late ARM; independent
stopping on total ROS/MAVROS/companion loss. The exact baseline, safe bench
procedure and observable criteria are in the [architecture acceptance plan](../../architecture/fcu-session-controller.md).
Companion-loss stopping remains an unresolved independent safety guarantee;
this software validation does not establish production readiness.
