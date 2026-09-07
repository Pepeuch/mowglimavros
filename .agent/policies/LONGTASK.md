# MowgliMAVROS — Long-Task Policy v1.0

Load only through the trigger in root `AGENTS.md`.

## Working checkpoint

Use `.agent/checkpoints/` for compact local resumable state.

Create/update a checkpoint before substantial edits, after meaningful milestones,
before broad builds/tests, before crossing repository/submodule boundaries, and
before ending unfinished work.

Recommended structure:

```markdown
# Agent checkpoint

Repository:
Branch:
HEAD:
Base/upstream:

## Objective
## Authorized scope
## Established facts — DO NOT REDISCOVER
## Semantic decisions — DO NOT RE-LITIGATE WITHOUT NEW EVIDENCE
## Files/symbols involved
## Validation — PASS / FAIL / BLOCKED
## Validation pending
## Known blockers
## Do not touch
## Exact next step
```

Keep checkpoints compact. Reference existing files/evidence instead of copying
logs, diffs, source, or long reasoning.

## Mandatory persistence

When this policy is loaded because the task is substantial, creating or updating
a checkpoint is part of the task itself, not an optional documentation step.

Do not conclude a long task without either:

1. updating an existing applicable checkpoint; or
2. creating a new checkpoint.

If established findings are reusable beyond the current session, classify and
promote them to `.agent/shared/checkpoints/` before handoff.

The final response must state which checkpoint was created or updated.

## Resume

On resume:
1. read the applicable checkpoint;
2. verify path/branch/HEAD/worktree/remotes/submodules;
3. compare with the recorded baseline;
4. invalidate only conclusions affected by changed dependencies;
5. resume from the exact next step.

Do not repeat expensive analysis because a different model/session produced it.

## Validation economy

Progress from focused to broad. Do not rerun an expensive successful suite unless
relevant code, dependency, build/runtime environment, generated interface, or
contradictory evidence changed.

## Durable disposition

At handoff/closure classify reusable checkpoint state as:

`LOCAL_ONLY | ACTIVE | BLOCKED | RETAINED | CLOSED | DELETE`

Promote only reusable verified state to `.agent/shared/checkpoints/`.
