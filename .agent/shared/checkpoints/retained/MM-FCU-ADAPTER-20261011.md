# MM-FCU-ADAPTER-20261011

Historical software baseline; current Git strategy and final safety-path audit
are recorded in MM-FCU-MAIN-COMMIT-20261011. Earlier no-commit/feature-branch
restrictions were superseded by explicit user authorization.

Disposition RETAINED — software implementation and evidence; physical gates open.
Repository /tmp/mowglimavros-fcu-session, feat/fcu-session-controller,
HEAD/base e7d516cb0adca1872dd9d10fb66412857d38b0c8 (origin/main at branch creation).
No commit/staging in the real index/push/merge/publication/deployment.
MowgliNext 7748af56acd98449938750714d2286193cf6143a remains clean;
gitlink remains e7d516cb0adca1872dd9d10fb66412857d38b0c8.

Prior validation below is the pre-review baseline; affected acceptance is
superseded by MM-FCU-REVIEW-FIXES-20261011 and its evidence archive.

## Approved architecture

MowgliNext sole functional authority. No extra service, schema, authorization
protocol, intent tokens/generations or session watchdog. The real idle GUI path
is blocked in MowgliNext; see the review-fixes checkpoint and follow-up document.
Minimal FCU MODE/ARM sequencer: demand from existing HighLevelStatus and MowerControl,
observed State, one pending RPC, accepted ACK + matching FCU state, deduplication,
refusal/confirmation-timeout diagnostics without tick retries. RPC serials are
callback correlation only. Late actual FCU confirmation can fulfill the target.
ArduPilot idle/base HOLD/disarmed; recording/manual MANUAL/ARM; autonomous GUIDED/ARM.
Direct backend MowerControl ON at idle temporarily ARM in HOLD with zero traction; OFF ends temporary ARM
only without a movement session. Safety DISARM supersedes pending MODE/ARM and
survives prior failure/timeout; late observed ARM causes corrective DISARM.
Existing SafetyState and BladeControl unchanged. Reconnect/emergency release
follow the current MowgliNext movement demand; no new user authorization step.
Neutral traction initialized before ARM; preconfirmation velocities are not cached.
GUIDED PositionTarget BODY_NED mask0x05E7; MANUAL mapping preserved.

Authoritative implementation/acceptance notes:
docs/architecture/fcu-session-controller.md.
HOLD servo source review: ArduPilot dbe792162d06cab66c3475fd5556bf7a120f119e,
Rover common command fallback plus ServoRelay channel-function/PWM dispatch;
no mode prohibition observed. Not proof of actual Pixhawk/VESC output.

## Software validation PASS

Pinned local SDK d98e8e9899e57cdcfb9b3b1aec5588bd0c64fc31956dcc726ec5c31ca6825316.
Every build/test/install command UID1000, container networknone, no devices;
isolated ROS domains. SDK build6/6 packages, bridge17/17 suites, ESC5/5,
battery1/1, GNSS1/1. Latest XML24 files,210 registered cases, zero failures,
errors or skips; sequencer24 cases included. Final changed-source graph reruns
provider+blade PASS; provider covers ArduPilot/PX4 x mowing-enabled true/false.
Safety graph, ESC graph and GNSS graph PASS. Interface contracts3 + backend10,
blade delegate/off contracts2, Python syntax, dashboard parity, diffcheck,
clang-format18 full-file + merge-base changed-line checks PASS.
Installed SDK ArduPilot provider graph PASS; ldd closure complete, .text identical
between build and installed binaries (CMake adjusts RPATH).
Build binary SHA256 3cfcc80d2772936a2412e39e17659919d8c22c723e67a19c67c127f68052bca6.
Installed binary SHA256 9ab08c37807c51e6ead04f46b35d02fd72102ab7ccb4d679c4ea6fc84c69d213.
Source manifest SHA256 dbf918a0cf9990b781beb02e30bed88833c571eae0535cc4ada2a8639238ec81.
Repository-owned evidence: docs/validation/fcu-session-controller-20261011/README.md.
That directory retains source-manifest.sha256, results-summary.json, all24 XMLs,
validation.log, binary-proof.txt, RPATH/ldd proofs and the archived review.diff.
The review diff predates this documentation-only archive/link update; validated
source manifest remains unchanged. No required evidence is stored only outside
this repository.
Earlier discarded-architecture compile/graph failures are retained in the archived log;
current simplified architecture suites all PASS. Known historic ESC graph
intermittence did not occur in the simplified-architecture validation.

## HARDWARE_REQUIRED

Actual HOLD/servo channel output, wheel/cutting neutrality and direction,
GUIDED velocity/yaw-rate signs/units, FCU confirmation and safety DISARM races,
complete companion-loss independent failsafe and external FCU command ownership.
Architecture doc records required baseline, bench procedure, observable criteria,
independent E-stop/blades-removed/constraint prerequisites. No readiness claim.
No hardware/SSH/FCU writes/parameter change/flash/production restart authorized.
Next step: review the software branch; any hardware run/deploy/push needs separate
user authorization. No required changes remain in authorized software scope.
