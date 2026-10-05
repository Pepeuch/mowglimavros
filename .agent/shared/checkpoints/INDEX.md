# Shared checkpoint index

Navigation only. Not the project backlog.

## Active


- `MM-GNSS-CANONICAL-ADAPTER-20261003` — dedicated MAVROS GNSS owner, Lyrical software/graph evidence; ARM64 target acceptance pending.

- `MM-MOWGLINEXT-INTEGRATION-20260929` — live Lyrical ARM64/GUI integration evidence with temporary NEO-M9N GPS1; hardware gates pending.
- `MM-MAVROS-COMPAT` — Kilted/Lyrical + MAVROS 2.15.1 software compatibility.

## Blocked

- `MM-WHEEL-TRIAL-PREFLIGHT-20260929` — CAN1 telemetry restored; single ARM refused by Battery 1 unhealthy before propulsion.
- `MM-HERE4-CAN2-20260928` — HERE4 node 124 communicates on CAN2 but reports bootloader-like maintenance/status 13 and no GNSS/compass telemetry.
- `MM-WHEEL-TRIAL-20260928` — Rover 4.7.1 ARM refusal before wheel propulsion; neutral-only capture.
- `MM-PIXHAWK-VALIDATION` — physical Pixhawk/ArduPilot backend validation.
- `MM-PIXHAWK-PASSIVE-AUDIT-20260928` — Rock 5B/Pixhawk read-only snapshot and pre-active-test gates.
- `MM-MANUAL-CONTROL-ROUTING-20260928` — armed MANUAL preflight: MAVROS GCS identity and Rover axis blockers; no manual motor command.

## Retained

- `MM-FC-MAVLINK-PROVIDER-COMPAT-20261004` — Betaflight/INAV source sender findings, MowgliMAVROS ESC contract, selected MSP hardware evidence; MAVLink runtime capture and outdoor fix pending.
- `MM-FIRMWARE-ROCK5B-20261005` — passive auto/ArduPilot serial bootstrap passes on Pixhawk5X; telemetry and restoration evidence; GNSS transients retained for follow-up.

- `MM-FIRMWARE-SELECTION-20261005` — firmware selector defaults to auto; Lyrical launch/build and MAVLink v1/v2 evidence; Rock serial validation recorded separately.

- `MM-MAVROS-AUDIT-20260907` — canonical pre-implementation compatibility audit.
- `MM-PIXHAWK-CAPABILITY-AUDIT-20260908` — passive amd64 Pixhawk/GPS/RTCM capability evidence; ARM64 and receiver follow-up pending.

## Closed

None.