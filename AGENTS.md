# MowgliMAVROS — Agent Rules v1.0

Repository-local execution policy for automated or AI-assisted contributors.

This file is the **always-loaded core**. Detailed rules live under `.agent/policies/` and are loaded only when their trigger applies.

Objective: **validated progress per unit of resource**. Never trade correctness, backend compatibility, hardware safety, recoverability, or public ROS contract integrity for speed or context savings.

---

## 1. Evidence and instruction roles

- `AGENTS.md` → stable always-on execution policy.
- `.agent/policies/*.md` → conditional detailed policy.
- `.agent/checkpoints/` → local working checkpoints; never versioned.
- `.agent/shared/checkpoints/` → intentionally versioned resumable finding memory.
- durable docs / TODO / architecture notes → reusable project knowledge and planning.
- source code → authoritative current implementation.
- tests → executable behavioural evidence.
- Git → exact repository and implementation state.

Do not duplicate large bodies of information between these layers.
If sources conflict, determine whether one is stale or refers to another repository state; prefer current executable evidence where appropriate; stop on unresolved contradictions; never silently guess.
A context compaction, model switch, or new session is not new evidence.

---

## 2. Session startup

Before substantial work:

1. read this `AGENTS.md`;
2. read `.agent/shared/checkpoints/INDEX.md` when present, then only the relevant checkpoint;
3. establish repository path, branch, `HEAD`, dirty state, remotes, and submodule state;
4. identify the owning component and affected ROS/backend contracts;
5. load only the smallest applicable policy module(s);
6. reuse established evidence unless a relevant dependency changed.

Do not begin with repository-wide rediscovery.

### Conditional policy loading

Load `.agent/policies/BACKEND.md` when work affects backend replacement, compatibility with `mowgli_hardware`, bringup, remaps, status/power semantics, or backend selection.

Load `.agent/policies/ROS2.md` when modifying ROS2 nodes, launch files, topics, services, actions, parameters, QoS, messages, remaps, diagnostics, or lifecycle behaviour.

Load `.agent/policies/HARDWARE.md` when correctness depends on Pixhawk, ArduPilot, MAVLink/MAVROS runtime behaviour, serial/USB/CAN/network transport, electrical wiring, power, actuator feedback, timing, or physical validation.

Load `.agent/policies/LONGTASK.md` when the task spans several dependent steps, requires architecture/lifecycle reasoning, uses an applicable checkpoint, or must survive interruption/compaction.

Load `.agent/policies/MIGRATION.md` when using historical PRs, forks, old branches, patches, or old MowgliNext/MAVROS implementations as guidance.

Load `.agent/policies/AUDIT.md` for repository audits, backlog reconciliation, stable finding IDs, duplicate handling, or durable audit ledgers.

Modules are cumulative and never weaken this core.

---

## 3. Core architecture invariants

`mowgli_mavros_bridge` is an **alternative hardware backend** to `mowgli_hardware`, not an additional helper node.

Backend selection is controlled by configuration/environment. When MAVROS is selected, it must behave as a full replacement rather than a partial overlay.

Unless an intentional compatibility change is explicitly authorized, preserve the externally expected contract currently supplied by `mowgli_hardware`, including as applicable:

- ROS graph shape and node responsibilities;
- topic names and remaps;
- message types;
- command semantics;
- status and power semantics;
- failure/degraded behaviour;
- bringup integration;
- observable operator behaviour.

Do not assume hardware mappings are final. Pixhawk pins, ArduPilot parameters, feedback paths, actuator mappings, and other hardware-specific details may stay configurable/provisional until bench or robot validation proves them.

Prefer coherent software-side compatibility fixes over hard-coded assumptions that depend on unvalidated hardware.
Never claim merge/release readiness for behaviour whose acceptance still depends on unvalidated Pixhawk, ArduPilot, wiring, actuator, or feedback behaviour.

---

## 4. Scope control

Audit/review/investigation requests are read-only unless implementation is explicitly requested.

Before editing, identify requested behaviour, owning component, affected external contract, consumers, acceptance evidence, and explicit out-of-scope neighbours.

Prefer minimal patches. Do not opportunistically fix unrelated defects. Record them separately unless they block correctness or safety.

Once implementation begins, freeze broad exploration unless new evidence contradicts an assumption or a required dependency is genuinely missing.
Never weaken a compatibility contract, safety boundary, or test expectation merely to fit scope.

---

## 5. Evidence discipline

Distinguish verified behaviour from assumptions.
For consequential conclusions, prefer current implementation, deterministic tests, current runtime evidence, documented external contracts, then historical evidence when migration policy is active.

Cite exact files, symbols, tests, commits, launch files, parameters, topics, or runtime observations when conclusions matter.
Do not repeat expensive analysis merely because the model/session changed.

---

## 6. Resource and reasoning economy

Use the least expensive reasoning level that can reliably perform the current step.

Prefer lower-cost reasoning for deterministic edits, formatting, routine tests, documentation synchronization, Git inventory, and established mechanical work.
Use higher-cost reasoning for architecture, compatibility semantics, concurrency, timing, ownership, lifecycle, provenance, difficult defect isolation, hardware boundaries, or final review of high-impact changes.

Investigate from narrowest to broadest: repository identity/current diff → exact symbol/file/test/contract → relevant implementation → producer/consumer/lifecycle boundary → neighbouring components only when required → repository-wide exploration only when targeted evidence is insufficient.

Do not reread large files or rerun expensive tests without a dependency-based reason.

---

## 7. Checkpoints and agent state

### Mandatory checkpoint creation

A checkpoint is mandatory when any of the following is true:

- an audit or investigation produces reusable findings;
- the task spans multiple dependent steps;
- substantial architecture or compatibility conclusions are established;
- broad builds, integration tests, or hardware validation are performed;
- work will continue in another session, model, repository, or context window;
- rediscovering the established evidence would consume meaningful time or tokens.

For such work, do not finish the task or hand off to another agent without
materializing the established state in a checkpoint.

Use `.agent/checkpoints/` while evidence is still provisional or local.

Promote verified reusable state to `.agent/shared/checkpoints/` before handoff
or completion when another session or contributor can reasonably reuse it.

For repository audits, the default durable disposition is:

- `ACTIVE` when implementation/follow-up work remains;
- `BLOCKED` for conclusions waiting on an external or hardware condition;
- `RETAINED` for completed audit evidence worth preserving;
- `CLOSED` only for compact anti-rediscovery records after the work is complete.

A chat response, terminal transcript, or model context is not a checkpoint.

Use `.agent/checkpoints/` for local resumable work. It must remain ignored by Git.
Use `.agent/shared/checkpoints/` only for intentionally reviewed reusable state.

Shared lifecycle: `active/`, `blocked/`, `retained/`, `closed/`.
`INDEX.md` is navigation only; it is never the TODO/backlog source of truth.

Never stage local `.agent/checkpoints/`.
Before staging shared agent state, remove secrets, credentials, transcripts, large logs, copied diffs, stale chronology, and local-only noise.

---

## 8. Git safety

Never commit or push unless explicitly authorized.
Do not discard unrelated user work, delete unrelated untracked files, mix generated/environment noise into a patch, rewrite history/force-push without authorization, or update submodules/gitlinks silently.

Before broad changes, verify repository identity and baseline. Before concluding substantial work, revalidate repository state.

---

## 9. Validation

Validate from the cheapest sufficient layer upward:

1. static/contract check;
2. focused regression;
3. affected component tests;
4. affected build targets;
5. ROS2/integration validation;
6. hardware validation only when required.

Investigate an early relevant failure before widening validation.
Always verify formatting/syntax for touched files and run `git diff --check`.
Do not weaken a test merely to make an implementation pass.
A task is complete only when required acceptance is passed or explicitly identified as pending/blocked.

---

## 10. Documentation and completion

Update `TODO.md`, roadmap, architecture docs, or compatibility documentation only when proven project state actually changes.
Documentation must describe current behaviour, not intended behaviour.

Before stopping, report concisely: files changed and why; validation executed; important evidence; known limitations/open items; repository/branch/`HEAD`; commit status; push status; exact next step if incomplete.

```md
## Canonical work queue

`TODO.md` is the canonical ordered backlog for repository work.

At session startup, after loading the applicable checkpoint, read only the
relevant part of `TODO.md`.

Checkpoints preserve evidence and resumable state; they do not replace the TODO.

When verified work changes project state, update `TODO.md` before handoff.
Do not mark an item complete without satisfying its stated acceptance criteria.