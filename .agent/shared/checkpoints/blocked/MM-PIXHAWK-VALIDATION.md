# MM-PIXHAWK-VALIDATION

Status: BLOCKED
Classification: HARDWARE_PENDING

## Scope

Physical validation of the mowgli_mavros backend against Pixhawk/ArduPilot.

## Still unproven

- manual_control motor mapping
- steering/throttle semantics
- blade command/feedback
- charging semantics
- HOLD behaviour
- disarm behaviour
- real power/status feedback
- failsafe behaviour
- transport reconnect behaviour
- reboot/power-cycle recovery

## Blocked by

Bench/Pixhawk hardware validation has not yet been performed for the final
software integration.

## Unblocks when

The corresponding software compatibility work is stable and a controlled
Pixhawk/ArduPilot bench session is available.

## Safety

No mower motion, blade activation, persistent parameter reset, firmware flash,
or destructive hardware action without explicit authorization.

Prefer read-only / no-motion bench validation first.

## Important

This checkpoint does not block deterministic SOFTWARE_NOW fixes.

Do not infer hardware correctness from successful compilation or ROS graph tests.

## Passive capability audit update — 2026-09-08

A workstation amd64 passive audit validated MAVROS/APM connection and natural
USB reconnect, but did not exercise actuators or safety behaviour. The RPi4 USB
path separately showed unstable Pixhawk enumeration/boot looping and still
blocks final ARM64 deployment validation.

GPS2 (operator-identified F9P) was detected without an indoor fix. GPS1 was
stale-configured for a DroneCAN HERE4 at node 124; after reconnect and power
cycle the powered HERE4 was still not detected. VESC 6 Pro devices historically
shared CAN1. CAN wiring, termination, coexistence and node selection remain a
dedicated HARDWARE_PENDING test. No parameter was changed.
