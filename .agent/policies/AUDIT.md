# MowgliMAVROS — Audit Policy v1.0

Load for repository audits, backlog reconciliation, stable finding IDs, duplicate
handling, or durable audit ledgers.

Root `AGENTS.md` remains authoritative.

## Audit checkpoint

A completed repository audit that produces reusable findings must create or
update a shared checkpoint before the audit is considered complete.

At minimum preserve:

- repository / branch / audited HEAD;
- stable finding IDs;
- established facts that must not be rediscovered;
- important decisions;
- validation already performed;
- blockers and unblock conditions;
- invalidation conditions;
- exact next step.

Do not store full logs or transcripts.

## Finding discipline

Use stable IDs for durable findings once the repository adopts them. Never reuse
an ID for another issue.

Keep dimensions separate:

- status: `IMPLEMENTED | PARTIAL | OPEN | BLOCKED | SUPERSEDED | OBSOLETE | DUPLICATE`;
- scope: component/area owning the work;
- validation: for example `HARDWARE_REQUIRED | HARDWARE_PENDING`.

Do not use a scope or validation label as a status.

`BLOCKED` must record:

```text
Blocked by:
Unblocks when:
```

## Accounting

Do not silently drop findings to make counts look correct.

Every durable audited item must remain present, intentionally removed, or
explicitly superseded/replaced.

A duplicate must point to one canonical non-duplicate finding.

## Evidence

Current executable evidence outranks stale narrative notes.

Missing hardware proof is not evidence staleness; load `HARDWARE.md`.

Generated dashboards/status views, if introduced later, must derive from one
versioned canonical backlog/manifest source, never from checkpoint directories.

## Completion

Before declaring an audit reconciled, verify counts, stable ID uniqueness,
duplicate directionality, classification consistency, and required deterministic
checks.
