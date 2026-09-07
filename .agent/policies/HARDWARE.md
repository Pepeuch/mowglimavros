# MowgliMAVROS — Hardware Validation Policy v1.0

Load when correctness depends on Pixhawk/ArduPilot/MAVLink/MAVROS runtime
behaviour, physical transport, wiring, power, actuators, feedback, timing, or
real robot behaviour.

Root `AGENTS.md` remains authoritative.

## Hardware boundary

Stop software-only proof when the missing fact is inherently physical.

Examples:

- Pixhawk output/feedback behaviour;
- ArduPilot firmware/parameter-specific runtime behaviour;
- MAVLink transport timing/reconnect;
- serial/USB/CAN/network physical transport;
- actuator enable/failsafe behaviour;
- wheel/blade hardware mappings;
- power/status feedback;
- reboot/power-cycle semantics;
- real mower motion or safety behaviour.

Do not replace missing physical evidence with speculative source archaeology.

## Required baseline

Record:

```text
Hardware:
Firmware:
Host/OS/kernel:
Transport/topology:
ArduPilot parameters/profile:
Backend configuration:
Test procedure:
Acceptance criterion:
Result:
What would invalidate this result:
```

Do not generalize one result across different Pixhawk models, firmware,
parameters, transports, wiring, or actuator topology without evidence.

## Safety

No mower motion, blade activation, actuator energization, destructive parameter
reset, firmware flash, or persistent hardware change unless explicitly authorized
for the current test.

Prefer bench/no-motion/read-only validation first.

Before persistent/destructive actions, require a known current state, reversible
procedure, verification method, and rollback.

## Blocking

If physical proof is unavailable, record the exact missing test, acceptance
criterion, blocker, unblock condition, and next hardware action; then stop that
dependent branch.

Independent software work may continue.
