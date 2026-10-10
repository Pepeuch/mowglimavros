# MM-DOCKER-EXPERIMENTAL-20261010

Disposition ACTIVE — integration and manual experimental publication authorized.
Standalone MowgliMAVROS worktree /tmp/mowglimavros-session-recovery-20261010,
branchrecovery/full-session-20261010, base/official main445d3e9 reverified.
Original stash/recovery/audits retained; no product/ROS/FCU/VESC/GUI edits.

Fix chicken-and-egg blocker: manual main workflow publication_mode=development
needs no acceptance records or production switches; only test-<SHA> manifest
and test-<SHA>-lyrical-{amd64,arm64} architecture tags. No prod metadata aliases.
Automatic main/dev builds and existing contracts continue; mode none/default
or ordinary push never publishes. Shared publish_images output guards GHCR
login, architecture writes and manifest job; production/dev outputs distinguish
which tag route executes. Source tags now SHA/mode-isolated, avoiding collisions
with concurrent release or other test runs. No robot deployment step.

Production remains default-off with explicit manual modeproduction +three
switches +existing tracked baseline-bound operator PASS/evidence verifier.
Indicators/record-format checks are not physical evidence. Manual publication
of an experimental image neither validates it nor authorizes physical trials.

Validation:workflow contract tests (actual shell mode selection, dev
success without records, production failure without records, modes/conflicts,
shared guards, experimental tag isolation, baseline/evidence checker).
actionlint1.7.12/YAML/diffcheck PASS; interface3/backend10 regressions PASS.
No image build/publication or release dispatch executed here.
Product SHA remains dbd60f82…; no full ROS rebuild needed for unchanged code.

Diff artifact: /tmp/mowglimavros-session-validation-20261010/experimental-publication-fix.diff.
Preparation was initially review-only; the operator has now authorized
integration and the manual experimental publication. No deploy implicit.

Operator now authorizes commit/pushmain, CI checks and manual development run.
Local GHCR inspection lacks read:packages (403); use authenticated Actions
manifest runner to snapshot latest/lyrical, verify test manifestamd64/arm64 and
assert both production digests unchanged. These additional checks are read-only,
experimental-only. No credential scope changed and no robot action authorized.
Actual publication result must be acquired, not inferred from offline tests.
