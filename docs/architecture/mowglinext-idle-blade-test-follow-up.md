# MowgliNext follow-up: explicit idle blade test

Status: SOFTWARE_REQUIRED in MowgliNext; documented only, no source/gitlink edit.
Read-only baseline: MowgliNext 7748af56acd98449938750714d2286193cf6143a,
MowgliMAVROS gitlink e7d516cb0adca1872dd9d10fb66412857d38b0c8.

## Confirmed failure and scope of current tests

The real path is GUI → BladeControlService → BladeDirection → MowerControl →
MowgliMAVROS. In `ros2/src/mowgli_behavior/include/mowgli_behavior/blade_direction.hpp`,
`forOperatorCommand(true, direction)` updates direction and removes the operator
inhibit, but outputs `requested_enabled_ && !operator_inhibit_`. At idle,
`requested_enabled_` is false: the forwarded MowerControl contains OFF. The
bridge cannot distinguish that OFF from a real stop and must obey it.

`blade_control_service.hpp` already validates inputs and bridge availability,
and serializes the service with BT callbacks. Its `success` means local handling;
`forwarded` means an asynchronous request was queued, not that the backend or FCU
confirmed execution. No service or message change is needed for idle ON.

The backend graph tests call MowerControl directly. Their successful HOLD/ARM/ON
sequence is not evidence that an actual GUI click starts a blade.
`test_blade_direction.cpp::MenuCannotStartIdleBladeAndEndSessionClearsOverride`
currently asserts the opposite requirement and will need updating.

## Minimum necessary functional correction

Keep the existing public service and direction semantics. In the existing
BladeDirection/BT integration, distinguish the default idle OFF housekeeping
from an explicit BT OFF (STOP, coverage halt, transit, docking, safety guard).
Allow the explicit operator ON to emit `enabled=1` in the former case. Retain
BT OFF priority in the latter case, even when an operator subsequently chooses
a direction. Do not set `requested_enabled_=true` merely to defeat the current
condition, and do not replace the result by unconditional `enabled`: either
would discard the reason the BT last stopped cutting.

This distinction is internal to the existing functional authority. A narrowly
scoped idle-test flag/idle housekeeping entry point in BladeDirection is enough;
no token, generation, external authorization service or MAVROS protocol is
needed. Clear that explicit idle test on operator OFF, endSession, and any
non-idle BT OFF. Preserve the operator OFF inhibit across periodic BT ON until
the existing explicit session-start path clears it.

The relevant call sites are `SetMowerEnabled::tick()` in `src/utility_nodes.cpp`
and coverage's `setBladeEnabled()` in `src/coverage_nodes.cpp`. Both currently
use `forMowerCommand(false, ...)` for real stops. Those paths must keep recording
OFF even when the bridge service is unavailable.

There is also a periodic cancellation trap: `trees/main_tree.xml` default
`IdleSequence` sends SetMowerEnabled(false), then waits 0.5s and can restart.
Only that default idle housekeeping must preserve an explicit idle test. The
`StopHoldSequence` (command 8), also publishing IDLE, must continue forcing OFF
every tick. Therefore allowing ON based only on HighLevelStatus==IDLE is
insufficient. A small internal idle-specific call/BT node input can express this
at the default IdleSequence, while all other existing SetMowerEnabled(false)
call sites retain their present stop semantics. This is an existing BT decision,
not a new safety decision in the bridge.

## Required future software acceptance

- Invoke the existing BladeControlService with ON FWD/REV from genuine default
  idle; assert the actual forwarded MowerControl is ON in the chosen direction.
- Tick default IdleSequence for several complete wait cycles; the explicit test
  remains active until OFF or a real BT stop, rather than being silently erased.
- STOP/coverage halt/transit/docking/guard OFF followed by operator FWD/REV still
  forwards OFF. Test STOP and default idle separately despite their shared IDLE
  status. Preserve `MenuOverridesRandomChoiceAndOffSurvivesTicksAndCoverage`.
- Operator OFF survives BT ticks/automatic continuation; explicit session start
  retains the existing inhibit-reset behavior. endSession clears the test.
- Keep invalid-input, outage and callback-group serialization regressions.
- Integrate this service path with the bridge mocks: forwarded ON → temporary
  HOLD/ARM → FCU confirmation → blade; no nonneutral traction; OFF cancels it.

No MowgliNext or GUI changes were performed here. Actual HOLD servo output and
complete companion-loss stopping remain separate HARDWARE_REQUIRED acceptance
items in `fcu-session-controller.md`; this software defect must be fixed before
claiming the GUI test path works end to end.
