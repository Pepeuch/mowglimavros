# MAVLink-first Betaflight / INAV / MowgliMAVROS investigation

**Investigation date:** 2026-10-04
**Scope:** Source review plus bounded USB/MSP bench observations; no implementation changes.

This bundle consolidates two investigations into one handoff because their findings meet at the same proposed FC-to-Mowgli interface.

## Findings for the backend

- The inspected Betaflight 2026.6.1 and INAV 9.1 MAVLink senders emit shared navigation/state/battery messages conditionally, but neither has a sender for common `ESC_INFO` (#290) / `ESC_STATUS` (#291), `RPM` (#226), `WHEEL_DISTANCE` (#9000), or legacy `ESC_TELEMETRY_*`. These are source findings; no MAVLink packet capture was performed on either test FC.
- MAVROS can receive common ESC info/status, but its status plugin needs ESC_INFO first to size the ESC vector. The current Mowgli wheel-odometry plugin relies on RPM plus legacy ESC telemetry counters, so adding only common ESC_STATUS output upstream will not yet feed its existing path. Current MowgliMAVROS main also preserves explicit 2D/3D/DGPS GNSS solution types through synthetic graph-tested mappings; this does not supply live FC/MAVLink evidence.
- INAV's connected GPS spoke UBX and its packet counter advanced, but no indoor fix was obtained. Its MAVLink GPS semantics and Betaflight's differ, including `alt_ellipsoid` handling and accuracy/DOP fields; the detailed reports identify the source locations and limitations.
- On the Betaflight/AM32 quad, bidirectional DShot produced per-motor RPM. A final temporary EDT-ON trial still produced no ESC temperature/voltage/current/consumption in MSP #139. The setting was restored to OFF. The actual ESC firmware build is unknown.

## Files

- `betaflight/REPORT.md` — full ESC/MAVLink/MAVROS investigation and final EDT trial.
- `inav/REPORT.md` — full connected INAV GPS/USB/MAVLink comparison.
- `betaflight/SOURCE_NOTES.md`, `inav/SOURCE_NOTES.md`, and `inav/MOWGLI_MAVROS_SOURCE_FINDINGS.md` — pinned source evidence and mappings.
- `betaflight/TEST_MATRIX.md` and `betaflight/restoration.json` — bench procedure, results, and final restoration status.
- `evidence-README.md` — included runtime files and exclusions. Selected raw MSP/JSONL/CSV evidence is under `betaflight/captures/` and `inav/`. Checksums are in `checksums.sha256`.

Full CLI config dumps and failed/intermediate CLI captures are excluded from the public copy. They are unnecessary for the findings and may contain unrelated aircraft/profile configuration or machine-specific identifiers. Public copies omit host USB serial identifiers and local absolute paths.

## Architecture question

The source evidence supports using MAVLink/MAVROS for common navigation/state, while the shared ESC/odometry contract needs upstream sender work and a Mowgli receiver extension. A focused follow-up is to decide whether Mowgli should consume `ESC_STATUS` freshness alongside legacy counters, then prototype standard sender support without removing legacy ArduPilot compatibility.
