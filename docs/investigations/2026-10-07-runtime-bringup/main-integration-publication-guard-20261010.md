# Main integration and fail-closed publication — 10 October 2026

Operator authorizes separated commits and a non-force direct push to official
Pepeuch/mowglimavros main. No deployment, FCU write/command, physical test or
production restart. Original stash and recovery/audit worktrees remain intact.

Pre-integration GitHub main and git ls-remote both equal9f62ba4b, the tested base.
All47 stash files remain represented, six adaptations documented, historical
Oct9 and fresh Oct10 audit evidence preserved. Product node SHA remains
dbd60f82dd350ce4390f13e50845eca1a7d2f6062d3bcf280f9605ffee95d4b9.
The prior pinned build/mocks validation applies unchanged to that product code.

## Publication protection

Both architecture-image writes AND the manifest job share one pins output.
Ordinary pushmain/dev: build and contracts run, GHCR login skipped, pushfalse,
publish-manifest skipped. No build-lyrical-amd64/arm64, latest, lyrical,
branch or SHA image tag is pushed. BuildKit CI caches are not runtime images.

Only manualworkflow_dispatch onmain with publish_productiontrue and all three
repository switches exactlytrue can proceed to acceptance-record verification.
Missing switches are false; none is currently defined on the repository.
Two tracked PASS/operator-confirmed acceptance records plus separate evidence
are mandatory and must match the current ROS2 Git tree. Missing/failed/stale
records fail before enabledtrue is emitted. No acceptance record is fabricated.
The verifier checks attestations/provenance, not the technical truth of physical
tests; operator review of the evidence remains required. Unresolved failsafe
and exclusive authority are NOT marked accepted by these changes.

See docs/production-image-publication.md for the exact release procedure.

## Checks before commit/push

- Publication contract6 tests PASS: both tiers, fail-closed default/ordinary push,
  shell output, missing records even with flags true, record baseline/evidence.
- actionlint1.7.12 PASS; YAML parse/matrix PASS; existing interface3/backend10
  contracts PASS and now also run explicitly in the pins job.
- clang18 changed-line and diffcheck PASS; product unchanged from6-package
  build/23 final XML reports PASS, known intermittent ESC test WARN retained.
- gitleaks8.30.1 full directory scan: two generic-api-key false positives,
  both the SAME historical ROS /bench/can_rescue_lease correlation token.
  The verified decoder matches counters/token to a bench run; it grants no
  API/authentication access. Decoder SHA matches the original passive checkpoint.
  Exact two finding fingerprints documented in .gitleaksignore; original raw
  evidence remains byte-identical. Re-scan must show zero unsuppressed findings.
- No .env, private keys, saved secret config, binaries, object files, archives,
  __pycache__, generated build/install/log files selected for commit. Historical
  tools/docs containing /tmp paths are intentional evidence, not installed tools.
- SDK/actionlint/gitleaks and build artifacts remain outside the repository.
- Staged whitespace review identified literal blank context lines in the archived
  unified diff and original blank EOF lines in four frozen audit artifacts.
  Narrow per-file .gitattributes exceptions preserve their exact bytes; no product
  formatting rule or general documentation whitespace check is weakened.

Commit plan: CI fail-closed guard first, recovered product/tests second,
documentation/audit/evidence third. One final non-force push to refs/heads/main,
only after rechecking remote ancestry. No intermediate unguarded product push.
Prepared commits: e369c55 (CI guard) and55b02b5 (product/tests). The documentation
commit preserves these findings, all47 stash files and the later audit evidence.

Push/GitHub confirmation is recorded in the ignored local handoff checkpoint
after the operation; this committed document records the preparation and policy,
not a pre-emptive claim that a push or hardware acceptance has happened.
