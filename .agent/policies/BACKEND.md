# MowgliMAVROS — Backend Compatibility Policy v1.0

Load when work affects backend selection, `mowgli_mavros_bridge`,
`mowgli_hardware` compatibility, bringup integration, remaps, or backend-facing
semantics.

Root `AGENTS.md` remains authoritative.

## Contract

`mowgli_mavros_bridge` is a replacement backend, not an overlay.

When selected through configuration/environment, the rest of the MowgliNext
stack should not need to know whether hardware is served by `mowgli_hardware`
or the MAVROS backend except where an intentional documented backend capability
difference exists.

## Review matrix

Compare the MAVROS backend against the expected external behaviour of
`mowgli_hardware` for every affected surface:

- launch/bringup participation;
- node responsibilities;
- published topics;
- subscribed topics;
- topic names/remaps;
- message types;
- QoS where externally relevant;
- command semantics;
- status/power semantics;
- diagnostics/operator visibility;
- failure/degraded behaviour;
- startup/shutdown/reconnect behaviour;
- configuration/env selection.

Classify findings as either:

- `SOFTWARE_NOW` → deterministic software integration issue that should be fixed now;
- `HARDWARE_PENDING` → behaviour depends on unvalidated Pixhawk/ArduPilot/wiring/feedback;
- `INTENTIONAL_DIFFERENCE` → documented backend difference explicitly accepted.

Do not hide a software incompatibility behind `HARDWARE_PENDING`.

## Implementation

Prefer minimal compatibility patches.

Keep provisional hardware mappings configurable.

Do not hard-code final actuator, feedback, parameter, or wiring assumptions until
physical evidence exists.

Do not run both backends simultaneously unless a test explicitly requires it and
the topology is proven safe.

## Completion

A backend-swap claim requires evidence that selection through configuration gives
the expected ROS contract and that the inactive backend does not remain partially
active.