# Production image publication — closed by default

`.github/workflows/docker.yml` retains pinned CI builds for Lyrical amd64 and
arm64 on pushes to main/dev and manual runs. Ordinary pushes NEVER publish
registry images or manifests, including latest, lyrical and main-lyrical.
GHCR login is skipped while publication is closed. BuildKit CI caches remain
enabled; they are not deployable production image tags.

Publication requires ALL of:

1. A manual workflow_dispatch on refs/heads/main.
2. Explicit boolean input publish_production=true (default false).
3. Repository variable ENABLE_PRODUCTION_IMAGE_PUBLICATION exactly 'true'.
4. Repository variable BLADE_FAILSAFE_VALIDATED exactly 'true'.
5. Repository variable BLADE_EXCLUSIVE_AUTHORITY_VALIDATED exactly 'true'.
6. Two explicit manual input paths to tracked docs/acceptance/*.md records,
   each confirmed PASS for the current ROS2 Git tree, with separate tracked
   evidence records. Missing, stale, failed or untracked records block the run
   before enabled=true can be emitted.

Missing variables or any other spelling/value mean false. Both architecture
image writes and the multi-architecture manifest job use the same resolved
gate. No repository variable or manual release input is enabled by the recovery
push. The helper test runs in the pins job before the build/publication jobs.

The two validation variables are release-owner attestations, not physical
evidence themselves. Set them only after recording acceptance of FCU failsafe
on total companion loss and exclusive blade command authority for the exact
release baseline. Current unresolved items are not waived by software CI PASS.
Required record fields (no actual acceptance record is fabricated here):

```text
Acceptance-Result: PASS
Acceptance-Kind: companion-loss-failsafe OR blade-exclusive-authority
Validated-ROS2-Tree: <git rev-parse HEAD:ros2 of the physically validated release>
Operator-Acceptance: CONFIRMED
Evidence-Record: docs/<separate versioned hardware results file>
```

Each record uses its own exact Acceptance-Kind, not the illustrative OR above.
The ROS2 tree includes source/config and Docker dependency pins; acceptance
documents live outside it, avoiding a self-referential commit hash. The checker
verifies attestation presence and matching baseline, NOT the truth of physical
observations or adequacy of the evidence. The release owner must review that
evidence before confirming the variables and the manual publication input.
Do not enable publication merely to deploy the recovered code for a bench test.

Offline contract test: python3 tools/test_docker_publication_guard.py.
It verifies workflow wiring and the fail-closed truth table; actual GitHub CI
execution must be checked after push. No automatic robot deployment is added.
