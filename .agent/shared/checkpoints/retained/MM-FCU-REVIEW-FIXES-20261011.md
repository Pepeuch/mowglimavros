# MM-FCU-REVIEW-FIXES-20261011

Historical software baseline; current Git strategy and final safety-path audit
are recorded in MM-FCU-MAIN-COMMIT-20261011. Earlier no-commit/feature-branch
restrictions were superseded by explicit user authorization.

Disposition RETAINED — post-review corrections; software PASS; physical acceptance remains open.
Repository /tmp/mowglimavros-fcu-session; feat/fcu-session-controller;
HEAD/base e7d516cb0adca1872dd9d10fb66412857d38b0c8. Worktree uncommitted.
MowgliNext HEAD 7748af56acd98449938750714d2286193cf6143a and gitlink e7d516c
remain unchanged/clean. No hardware, deployment, parameter change, push or merge.

## Current architecture and corrections

Minimal MODE/ARM RPC sequencing only; MowgliNext sole functional authority.
Existing isolated-test flag is released on FCU activation refusal/timeout,
queued blade expiry, or BladeControl cancellation/failure. Neutral cutting and
traction; target HOLD/disarmed. Cancelled ON cannot run after late ARM.
The helper cannot end a movement-owned ARM. No new authorization/generation,
interface or safety policy. Existing BladeControl header is unchanged.

OFF examined before any queued ON may dispatch; existing permission edge applied
before binding its response revision. Emergency success means queued DISARM or
already observed disarmed, independent of HOLD endpoint. Diagnostics expose
pending transport, ACK and target state confirmation separately.

## Confirmed MowgliNext blocker

forOperatorCommand currently forwards OFF for idle GUI ON. Default IdleSequence
also sends periodic OFF; StopHoldSequence shares IDLE but must retain STOP
priority. Future minimal internal distinction required, no new public service.
Exact baseline, call sites and tests:
docs/architecture/mowglinext-idle-blade-test-follow-up.md.
Do not treat direct backend MowerControl injection as true GUI evidence.

## Evidence and next step

New archive: docs/validation/fcu-session-review-fixes-20261011/.
Contains final source/doc review.diff, node-only pre-review delta, exact hashes,
JUnit XML and build/test logs. Earlier archive remains historical baseline.
First new graph failure was a test expecting OFF success through an active-session
BladeControl servo fault; fixed expectation preserves that existing fault,
checks actual neutral and retained wheel ARM. Matrix then PASS.
Final broad bridge run PASS 18/18 suites, 106 XML tests, no failures/skips (191.55s).
Controller 25 cases; BladeControl16/BladeOff5; SafetyState15; readiness13.
Provider/blade/safety/ESC/GNSS graphs PASS. Final peer API-warning cleanup
rebuild + targeted test PASS (0.33s), no production-source change afterward.
clang-format18 (full files + changed lines), Python compile, diff whitespace,
README progress consistency PASS. 38 input-file hashes verified against the
container's actual sources. Real Git index remains empty.
No commit until user has reviewed final diff/results. Hardware gates remain
HARDWARE_REQUIRED with exact bench baseline/procedure/criteria/prerequisites in
docs/architecture/fcu-session-controller.md. Independent companion-loss stopping
remains unresolved; no production readiness claim.
