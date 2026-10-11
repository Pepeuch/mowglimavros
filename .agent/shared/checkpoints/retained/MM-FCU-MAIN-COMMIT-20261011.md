# MM-FCU-MAIN-COMMIT-20261011

Disposition RETAINED — final safety-path audit and direct-main commit handoff.
Repository /tmp/mowglimavros-fcu-session (Pepeuch/MowgliMAVROS), branch main.
Base e7d516cb0adca1872dd9d10fb66412857d38b0c8. User explicitly authorizes a
local commit directly on existing main and removal of the local feature branch;
prior feature/no-commit instructions are superseded. No push/PR/hardware/deploy.

## Git provenance and preservation

git fetch origin main succeeded on 2026-10-11. main, refreshed origin/main,
FETCH_HEAD and feat/fcu-session-controller all matched base e7d516c.
Switched to main with the dirty worktree; all 91 modified/untracked file hashes
were identical immediately afterward. No stash/reset/discard. Earlier archives
remain historical baselines, linked from the current report.
MowgliNext HEAD 7748af56acd98449938750714d2286193cf6143a clean, gitlink e7d516c
unchanged. Only the standalone clone is modified.

## Final safety-path correction

Original request_blade_disarm(): neutral cutting + DISARM, no mode write.
Original request_hold_and_blade_disarm(): neutral cutting/traction + HOLD/DISARM.
The reviewed alias and unconditional emergency→HOLD had unintentionally changed
tilt/double-lift behavior. Restored mode preservation for these inputs and tilt
latches, including repeated sequencer ticks. Hardware switch/service emergency
retain HOLD. Both use one priority DISARM sequencer; HOLD failure/unavailability
cannot block safety DISARM. No additional safety policy or ROS interface.

Existing BladeControl and SafetyState unchanged. Temporary blade ON failure/expiry
cleanup, OFF-before-pending-ON, MANUAL/GUIDED movement ownership and confirmed
automatic ARM remain. MowgliNext remains the sole functional authority.

## Software evidence

Software PASS 18/18 suites, 112 XML tests, zero failures/errors/skips (192.94s).
Node regressions7; FCU25; Blade16/Off5; Safety15. Native/provider graphs PASS.
38 source hashes verified against final host and tested container; format18,
syntax, whitespace and progress consistency PASS. Final build clean. Evidence: docs/validation/fcu-main-commit-20261011/.
New mock cases: lift/tilt in MANUAL/GUIDED without HOLD, hardware/service with HOLD;
check request, ACK, observed DISARM and subsequent ticks. Native safety graph
also distinguishes lift-only DISARM from hardware HOLD. Full bridge suites rerun and PASS.
UID1000 pinned SDK container, network none, no devices. Initial test-harness
compile error from generic ROS lambdas corrected with explicit typed signatures.
Source hashes, XML, logs, transfer snapshot and final diff remain repository-owned.
Exact-path artifact attributes retain byte-identical archived diffs/logs/generated
XML; source whitespace checks remain unchanged. Full staged diff check PASS.

## Remaining acceptance and Git handoff

Actual idle GUI ON is SOFTWARE_REQUIRED in MowgliNext; correction documented in
docs/architecture/mowglinext-idle-blade-test-follow-up.md, no Next edits.
HARDWARE_REQUIRED: HOLD servo output, real zero wheel movement, GUIDED signs/units,
wire-level ARM/DISARM races and independent total companion-loss stopping.
Exact safe bench baseline/procedure/criteria in fcu-session-controller.md.
No production readiness claim. Commit title and result recorded in the validation
report and Git history; no push until the user approves after reviewing them.
