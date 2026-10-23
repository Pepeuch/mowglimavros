# MM-GNSS-DOCKER-20261023 — ACTIVE, release v1.6 blocker

Standalone MowgliMAVROS review/gnss-interface-20261023@61d3e37b4bc2f7081b317e955877bafcf2f0b5db.
Local7a2a1a0963886dd19f151681ccc4eae445600fc8 committed the14 approved paths.
61d3e37 followed without rewriting history: explicit Docker interface-root and
its regression (2 lines). The initial real AMD64 image build proved the missing
path; RED/GREEN and old/cancelled attempts are retained.

Both local images built from the same source/pins; generated eight-type hashes
match actual Mowgli image7489a4a6 on both architectures. GNSS RIHS01_ca48e4f1...
Both Docker C++/ROS suites224 casesPASS (17GNSS each), runtimeCyclonePASS,
175existing GUI tests and6 actual Docker-wire classificationsPASS. Actual image
Mowgli59051b41 consumes22 statuses from candidateMAVROS61 in isolated DDS.
ELF verifiedx86-64 andAArch64. No physical GNSS or browser-on-robot acceptance.

Evidence: docs/validation/gnss-docker-20261023/ including review.md, images.json,
interface-comparison.json, test-summary.json, recipes/diffs, logs and manifests.
Images local only; no Git push or GHCR publication. IDs are not registry digests.
ARM used isolated built-in QEMU; compiler/tests UID1000 via explicit temporary
ownership recipe. Original Dockerfile root execution was NOT silently certified.

MowgliNext9686b375,136 historicals,1827 previous evidence files and three clean
submodules/gitlinks unchanged. No update to its MAVROS pin applied. Proposed
source/image gates are tested artifacts only, pending integration and real
published digests. TODO MM-602 carries current status.

PR1017 / v1.6 remains BLOCKED: new GO for source/image publication, immutable
sidecar digest in installer/bundle and paired source+generated gates, exact
final MowgliNext release image, then passive target acceptance. HARDWARE_REQUIRED
and independent Fleet reservation remain. Procedure and safety prerequisites in
release-plan.md and initial GNSS proposal. No SSH, robot, firmware flash, motor,
CI trigger/polling or merge. STOP review; do not publish/merge from softwarePASS.

Publication addendum 2026-10-23: both code commits and local validations accepted.
GO for review-branch push and PR to main only, plus metadata finalization.
See docs/validation/gnss-publication-20261023/checkpoint.md and review-artifacts.json.
Do not inspect or trigger CI in this publication operation. No image publication,
MowgliNext gitlink change, merge, deployment or hardware acceptance authorized.
PR1017/v1.6 remains BLOCKED; HARDWARE_REQUIRED and Fleet reservation unchanged.
