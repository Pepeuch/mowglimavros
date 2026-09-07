
### `.agent/policies/MIGRATION.md`

```md
# MowgliMAVROS — Migration Policy v1.0

Load when historical PRs, forks, old branches, patches, or legacy MowgliNext /
MAVROS implementations are used as guidance.

Historical material is evidence, not automatically mergeable code.

Before reuse:

1. state the behavioural question;
2. identify the old contract/defect;
3. inspect current ownership and architecture;
4. compare interfaces, lifecycle, configuration, and tests;
5. separate verified behaviour from assumptions.

Classify each historical candidate with exactly one:

- `PORT`
- `ADAPT`
- `ALREADY_PRESENT`
- `SUPERSEDED`
- `REJECT`

Do not cherry-pick or mechanically restore an old implementation merely because it
once worked.

When the old behaviour concerns `mowgli_hardware` compatibility, also load
`BACKEND.md`.