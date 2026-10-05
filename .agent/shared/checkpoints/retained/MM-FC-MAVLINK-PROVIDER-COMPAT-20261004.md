# FC MAVLink provider and ESC/GPS investigation — 2026-10-04

Repository: `Pepeuch/mowglimavros`
Audited branch/ref: `main`
Audited HEAD: `82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492` (source findings refreshed after this commit landed during preparation)
Disposition: `RETAINED`
Canonical evidence bundle: [`docs/investigations/2026-10-04-mavlink-first/`](../../../../docs/investigations/2026-10-04-mavlink-first/)

## Objective

Record source-grounded Betaflight/INAV MAVLink sender and MowgliMAVROS compatibility findings, with bounded FC runtime data, to help decide whether to converge these FC families through standard MAVLink/MAVROS.

## Established findings — do not rediscover without new source/runtime evidence

- Inspected Betaflight 2026.6.1 and INAV 9.1 source has shared navigation/state/battery sender paths but no sender for common ESC_INFO (#290), ESC_STATUS (#291), RPM (#226), WHEEL_DISTANCE (#9000), or ESC_TELEMETRY_* in the examined modules. No FC-to-MAVROS MAVLink runtime capture was made.
- Current MowgliMAVROS ESC wheel odometry consumes RPM plus legacy ESC telemetry freshness counters; common ESC_STATUS alone will not feed the current path. MAVROS ESC status sizes its array from ESC_INFO and exposes empty status if INFO is absent.
- INAV F722 target on firmware 9.1.0 detected UBLOX10/protocol 34.09 and parsed valid UBX frames during a 180 s indoor poll (2,520/2,520 selected read requests; packet counter 181→3091), but had no fix. Its GPS receiver serial link/protocol path worked; outdoor fix and GPS MAVLink delivery remain unverified.
- Betaflight HDZERO_HALO on firmware 2026.6.1 returned per-motor RPM from bidirectional DShot/MSP #139. A temporary EDT-ON trial produced no temperature, voltage, current, or consumption fields. `dshot_edt=OFF` was restored and verified; all motor outputs were verified neutral. ESC firmware/version remains unknown.
- GPS MAVLink producer semantics differ: Betaflight source copies MSL altitude to `alt_ellipsoid`; INAV uses zero there and has accuracy/DOP semantic differences. Current MowgliMAVROS main at 82a1e39 preserves explicit 2D/3D/DGPS canonical fix types and adds synthetic graph coverage; this is not live FC evidence and does not change covariance/ellipsoid requirements. Details/revisions are in the evidence bundle.

## Evidence boundaries

- Runtime captures are MSP over each FC's USB CDC, not MAVLink packets; source audit is not runtime sender confirmation.
- INAV had no indoor fix; coordinates were all zero. No outdoor test, mower load/reverse test, optical tachometer, ESC identification, code modification, firmware flash, ESC setting change, or MAVLink routing change occurred.
- FC MCU core temperature reached 92 °C after the brief final test; this is not ESC temperature.
- Public-copy config dumps, machine USB serial identifiers, aircraft profile names, and incomplete CLI captures are omitted. The checksums cover the selected public files.

## Validation performed

- Reports and companion findings reviewed for source/runtime boundary and current final EDT state.
- Confirmed public INAV GPS_RAW_GPS records have `fix=0`, `satellites=0`, and zero latitude/longitude.
- Sanitized public copy for local absolute paths, USB serial identifiers, and Betaflight pilot/craft/rate-profile names.
- No implementation tests or builds apply; changes are documentation and captured evidence only.

The source audit initially used MowgliMAVROS `main` a1fe22c and Universal GNSS 6f0eb09. Final review refreshed these to main 82a1e39 and Universal GNSS 383caba3; the newer source adds explicit canonical 2D/3D/DGPS fix preservation.

## Exact next step

Review the documentation-only pull request and decide whether MowgliMAVROS should add common ESC_STATUS freshness ingestion alongside its legacy ESC telemetry counters, then coordinate a sender prototype with Betaflight/INAV/ArduPilot maintainers. Any live MAVLink, outdoor GPS, or wheel behavior claim needs new hardware evidence.
