# FCU adapter — direct main commit and final safety-path audit

Repository `/tmp/mowglimavros-fcu-session`, Pepeuch/MowgliMAVROS.
User explicitly authorizes transferring the work to existing `main`, deleting
the unnecessary local feature branch, and a local commit directly on main.
No push, PR, hardware access, parameter change, deployment or production-image
publication is authorized or performed.

## Git verification and transfer

On 2026-10-11, `git fetch origin main` succeeded against
https://github.com/Pepeuch/mowglimavros.git. Local main, refreshed origin/main,
FETCH_HEAD and feat/fcu-session-controller all equal
`e7d516cb0adca1872dd9d10fb66412857d38b0c8`, the recorded work-branch base.
`git switch main` retained the uncommitted worktree. SHA256 comparison proved
all 91 modified/untracked files unchanged immediately after switching;
`transfer-before.json` records that snapshot. No stash/reset/file deletion was
needed. The prior validation archives remain intact, with their own historical
source/binary baselines; this archive records the final corrected implementation.

MowgliNext is read-only at `7748af56acd98449938750714d2286193cf6143a`;
its MowgliMAVROS gitlink remains `e7d516cb0adca1872dd9d10fb66412857d38b0c8`.

## Final safety-path finding and correction

Comparison against the original node at e7d516c found that
`request_blade_disarm()` originally neutralized the blade and sent DISARM only.
Its tilt/double-lift callers did not request HOLD. The reviewed implementation
had inadvertently aliased it to `request_hold_and_blade_disarm()`, and the
periodic sequencer also translated all emergency projections into HOLD.

Restored the distinction in both direct callbacks and periodic reconciliation:
- `request_blade_disarm()` → neutral outputs + priority DISARM, preserve the
  observed FCU mode for tilt/double-lift (including existing tilt latch).
- `request_hold_and_blade_disarm()` → neutral outputs + priority DISARM and HOLD,
  for the hardware switch and explicit emergency service, as before.
- Both share the same RPC owner. Unavailable/refused/timed-out HOLD cannot block
  DISARM. Repeated callbacks/ticks still do not retry a refused operation.

The helper only preserves existing dispatch semantics; no new safety decision,
watchdog, authorization, public ROS interface or MowgliNext requirement is added.
Existing BladeControl and SafetyState code is unchanged. Functional motion
requests continue to be owned by MowgliNext. Existing emergency release behavior
reconciles its current demand. Isolated blade ON/OFF cancellation and automatic
confirmed ARM remain as validated in the preceding review.

## Validation

**PASS: 18/18 suites, 112 XML-recorded tests, zero failures/errors/skips.**
Full run 192.94s. FCU sequencer25, BladeControl16, BladeOff5, SafetyState15,
readiness13, ESC tracker14, providers/conversion6+2, node callback regressions7;
provider/blade/safety/ESC/GNSS graphs and static firmware/launch/OFF checks PASS.
The provider graph covers ArduPilot/PX4 × mowing_enabled true/false. Graph
scenarios are grouped as one XML test per graph, not counted individually.
38 input-file hashes match the final host and actual tested container sources.
clang-format18 full-file and changed-lines checks, Python syntax, git whitespace
and README progress consistency checks PASS; final build has no warnings.

The local commit directly on main is titled
`feat(bridge): adapt MowgliNext sessions to confirmed FCU mode and arming`.
Its hash is available in Git history; this report and the retained main checkpoint
are committed with the implementation. The old local feature name is removed
only after the commit succeeds (its history is included in main). No push.
`review.diff` contains the complete source/doc/checkpoint patch against e7d516c,
excluding validation archives to avoid duplicating them. `modified-files.txt`
lists its scope; archived validation files themselves are also included in the
commit. Earlier archives are retained unchanged. Exact-path `.gitattributes`
entries preserve the trailing spaces already present in unified-diff context,
compiler output and generated JUnit XML, without altering their bytes/hashes.
These exceptions apply only to 33 named evidence artifacts; source and ordinary
documentation keep their whitespace checks. The full staged whitespace check
passes with these artifact attributes.

 New deterministic mock regressions exercise
lift and tilt in MANUAL/GUIDED, and hardware/service HOLD paths, across the
DISARM request, accepted ACK, observed disarm and repeated bridge ticks.
The native MAVROS safety graph now checks that lift adds no HOLD while the
hardware switch still does. All 18 bridge suites are rerun after the correction.

Non-root UID1000 container `mowgli-fcu-session-tests`, SDK image SHA256
`d98e8e9899e57cdcfb9b3b1aec5588bd0c64fc31956dcc726ec5c31ca6825316`,
network none, no devices; native MAVLink fake FCU endpoints are loopback only.
Runtime CycloneDDS, ROS domain203 (graph tests use their existing isolated domains).
Build script, logs, XML and source/binary hashes are retained here.
Initial compilation of the new regression file rejected generic service lambdas;
explicit Request/Response shared-pointer signatures fix the test harness. The
initial compiler log is retained; no production behavior or protection was
changed to address that test build error.

## Open acceptance

The [MowgliNext idle-GUI ON blocker](../../architecture/mowglinext-idle-blade-test-follow-up.md)
remains SOFTWARE_REQUIRED, documented only. Direct backend service tests are
not GUI end-to-end proof. No MowgliNext source or gitlink changes.
HARDWARE_REQUIRED: HOLD servo output, physical zero wheel movement, GUIDED units
and signs, wire-level ARM/DISARM races, and independently guaranteed stopping on
total companion/ROS/MAVROS loss. Exact bench baseline, safety prerequisites,
procedure and criteria are in the [architecture plan](../../architecture/fcu-session-controller.md).
No production readiness claim.
