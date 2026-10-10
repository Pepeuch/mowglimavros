# Shared checkpoint index

Navigation only. Not the project backlog.

## Active

- `MM-SESSION-RECOVERY-20261010` — complete47-file stash recovery reconciled in
  standalone repository; pinned build/mocks PASS, full review diff available,
  audits preserved, no commit/deploy and physical/failsafe gates still open.

- `MM-MAVROS-REFRESH-20261006` — WheelTick unit/provenance, Safety latches,
  dedicated runtime configuration, RTK calibration GUI and traction opt-in;
  software tests pass, full sidecar graph and target acceptance pending.

- `MM-SAFETY-INPUTS-20261006` — Pixhawk hardware-safety and AP_Button wheel-lift
  ROS contract validated; physical polarity/GPIO/authority acceptance pending.

- `MM-ESC-ODOMETRY-20261005` — COMMON/legacy ESC and configurable wheel sources; review A/B/C and STATUS-authoritative ownership fixes, raw motor ticks/ticks_per_meter refactor validated (76 wheel/source cases, all10 bridge suites, power/contracts/build pass); bench signed feedback/short powered F-R pairing verified, RTK metric calibration/odometry acceptance pending.


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

- `MM-BLADE-HISTORY-RECOVERY-20261010` — Oct9 session remains in stash c93548f;
  no published feature commit; original recovery review retained.

## Session recovery in progress

- Complete47-file recovery is prepared in the standalone MowgliMAVROS recovery
  worktree; report/session inventory under runtime-bringup. Current production
  and audit source worktrees remain untouched; no new physical acceptance.

## Retained historical session findings

- `MM-MAVLINK-RATES-20261009` — passive60s receive audit +30s instance/topic
  verification; periodic streams1Hz, ESC republication3Hz/acquisition1Hz,
  MAV1/MAV2 configuration and source-qualified freshness audited. Per-message
  rate proposals only, no FCU/interval writes or motors.

- `MM-BLADE-PRODUCT-20261008` — OFF/FWD/REV software patch, neutral-before-inversion,
  deferred ACKs, safety/startup neutral and source-qualified stop proof; complete
  pinned build/bridge/ESC/provider/safety validation PASS. ARM64 built/deployed,
  startup/OFF target PASS; first FWD refused before any nonneutral output,
  STOP, REV/inversion/safety not run (2026-10-09);
  native bench baseline retained, companion-loss failsafe and exclusive authority open.

- `MM-FIRMWARE-PROVIDERS-20261005` — bootstrap and C++ command/emergency providers; capabilities and ArduPilot/PX4 software equivalence; nine suites and installed graph pass.

- `MM-FC-MAVLINK-PROVIDER-COMPAT-20261004` — Betaflight/INAV source sender findings, MowgliMAVROS ESC contract, selected MSP hardware evidence; MAVLink runtime capture and outdoor fix pending.
- `MM-FIRMWARE-ROCK5B-20261005` — passive auto/ArduPilot serial bootstrap passes on Pixhawk5X; telemetry and restoration evidence; GNSS transients retained for follow-up.

- `MM-FIRMWARE-SELECTION-20261005` — firmware selector defaults to auto; Lyrical launch/build and MAVLink v1/v2 evidence; Rock serial validation recorded separately.

- `MM-MAVROS-AUDIT-20260907` — canonical pre-implementation compatibility audit.
- `MM-PIXHAWK-CAPABILITY-AUDIT-20260908` — passive amd64 Pixhawk/GPS/RTCM capability evidence; ARM64 and receiver follow-up pending.

## Closed

None.
