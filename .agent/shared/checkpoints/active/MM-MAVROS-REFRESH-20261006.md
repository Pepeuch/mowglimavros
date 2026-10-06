# MM-MAVROS-REFRESH-20261006

Disposition: ACTIVE — requested software integration implemented; real sidecar
build/graph and new target acceptance pending.

Baseline: MowgliMAVROS main `b92925b2bc969585350b824ac066004eae903e3b`
plus working patch; MowgliNext feat/mavros-refresh
`24094f32f9748628f354e03d73b106d5c7eceb07` plus working patch. No commit,
push, deployment or new physical actuation. Existing signed feedback evidence
is retained; obsolete checkouts and old patch artifacts are irrelevant.

## Fixed contracts

- MOTOR_OUTPUTS absent => Unknown, present/disabled => Engaged,
  present/enabled => Released. Hardware release clears its latch; software clear
  clears its own latch only. Physical safety cannot be bypassed by lift protection.
- WheelTick preserves the official locked IDL. Raw signed motor revolutions are
  independent of calibration. Magnitude counts use S=1000; wheel_tick_factor is
  ticks_per_meter*S. Counters remain continuous through source/epoch/segment,
  invalidity and reconnect; references are rebased. Only advancing source sample
  identity publishes. Source, scale, epochs and per-wheel segments are exposed in
  the existing source diagnostic. No ESC decoder, gearing or physical mapping rewrite.
- Launch loads dedicated runtime odometry and hardware-bridge YAML after packaged
  defaults. MowgliNext derives startup values from template + sparse overrides;
  GUI saves maintain these dedicated files too.
- Traction requires explicit boolean opt-in, connected/already armed FCU,
  released physical switch and no active/latched emergency. Rejected motion and
  disabling traction publish neutral. No auto-arm; blade control remains false.
- MowgliNext's passive RTK calibration records existing WheelTick and raw GNSS
  observations; a straight 2–10 m pass fits ticks_per_meter from independent motor
  revolutions / RTK distance. Only this scalar is applied/persisted. No PID/FF
  writes or motion commands. Freshness, repeated timestamps, wheel validity,
  reversal, source/epoch/segment, RTK Fixed, straightness and L/R consistency gate it.
- Backend-specific Drive and Safety GUI, confirmation before enabling traction
  or disabling lift protection, visible lift telemetry when protection is off,
  FR/EN strings and boolean live routes are covered by focused tests.

## Software evidence

- Pure wheel/core/projection: 82 cases PASS; pure safety/traction: 3 PASS.
- External contract 10, interface contract 3, mocked launch 11 PASS.
- MowgliNext: 107 frontend files / 787 tests, API/providers/Foxglove suites,
  tsc, lint (0 errors), Playwright system-power 7, sparse writer 4 (integral/fractional double types),
  installer matrix/idempotency/YAML 100 cases PASS.
- Diff whitespace checks and MowgliNext changed-line clang-format 18 PASS.
  Original sidecar C++ files retain their surrounding style without global reformat.

## Pending acceptance

ENVIRONMENT_PENDING: full plugin build and real native graph, including updated
provider graph and the original safety decoder suite; local MAVROS/MAVLink
dependencies are absent and must not be installed for this task.

HARDWARE_PENDING: new target calibration/safety/traction acceptance, exact Hall
GPIO/electrical levels and wiring. Use MowgliNext `sensors/mavros/VALIDATION.md`
for exact baseline capture, safety prerequisites, commands and pass/fail criteria.
Do not generalize previous bench feedback measurements to this new image/patch.

Next: apply the exported full patches to the intended checkouts, build in the
existing pinned sidecar environment, then obtain the documented target evidence
under separate physical-test authorization. No architectural rediscovery needed.
