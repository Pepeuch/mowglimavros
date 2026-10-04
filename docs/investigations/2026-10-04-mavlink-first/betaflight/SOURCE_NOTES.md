# Source notes and pinned references

This file separates source facts from runtime observations. Betaflight analysis is pinned to the firmware actually installed on the test FC. INAV is pinned to the 9.1.0 tag, whose official docs identify themselves as version 9.1. MAVROS, ArduPilot and AM32 links are pinned to the revisions shown below. Message rates are configured or conditional; where runtime capture was not done, no measured rate is claimed.

## Betaflight 2026.6.1 — installed target source

**Repository/revision:** `betaflight/betaflight`, commit `6dbc4218fd6bc33bf16ea32c670304d4f89321d5` (target CLI reports build key prefix `6dbc4218f`, firmware version 2026.6.1).

| File / function | Evidence and fields | Rate / limitations |
|---|---|---|
| [`src/main/telemetry/mavlink.c`](https://github.com/betaflight/betaflight/blob/6dbc4218fd6bc33bf16ea32c670304d4f89321d5/src/main/telemetry/mavlink.c): `mavTelemetryOutputMessages[]`, `configureMAVLinkOutputMessagesIntervals()`, `processMAVLinkTelemetry()` | **SOURCE VERIFIED:** active stream senders for `SYS_STATUS` #1, `RC_CHANNELS_RAW` #35, `GPS_RAW_INT` #24, `GLOBAL_POSITION_INT` #33, `GPS_GLOBAL_ORIGIN` #49, `HOME_POSITION` #242, `ATTITUDE` #30, `HEARTBEAT` #0, `VFR_HUD` #74, `BATTERY_STATUS` #147. GPS messages are build/sensor conditional. Periods come from stream-group settings or a MAVLink message-interval command. | No active sender or schedule entry for ESC_INFO #290, ESC_STATUS #291, ArduPilot ESC_TELEMETRY_*, RPM #226, WHEEL_DISTANCE #9000, HIGHRES_IMU #105 or SCALED_IMU #26. No telemetry schedule rate was runtime-captured. Separate periodic SYSTEM_TIME and request/response paths exist. |
| Same file: `mavlinkSendGpsRaw()` | **SOURCE VERIFIED:** `fix_type` 1 when GPS present without `GPS_FIX`; 2 when GPS_FIX but satellites below BF minimum; otherwise 3. `alt = llh.altCm*10` (MSL mm), and `alt_ellipsoid` is assigned that same MSL value. `eph=hdop`, `epv=vdop`; h/v accuracy copied in mm; `hdg_acc=UINT32_MAX`, `yaw=0`. | Sends only with detected GPS sensor; GPS was disabled on test quad, so source only. `cog` is ground course, not heading. The ellipsoid field is numerically populated but semantically wrong. |
| Same file: `mavlinkSendGpsGlobalPosition()` / `getHeadingCentidegrees()` | **SOURCE VERIFIED:** MSL altitude from GPS, relative altitude from BF estimate, north/east/down speed from GPS, `hdg` from BF attitude yaw. | GPS-conditioned; no runtime GPS evidence here. |
| [`src/main/drivers/dshot.c`](https://github.com/betaflight/betaflight/blob/6dbc4218fd6bc33bf16ea32c670304d4f89321d5/src/main/drivers/dshot.c): `erpmToHz` setup, bidirectional decoder/update path, `getDshotRpm()`, `erpmToRpm()` | **SOURCE VERIFIED:** DShot eRPM is converted to mechanical RPM using `ERPM_PER_LSB` and configured pole pairs: mechanical RPM = raw eRPM × `ERPM_PER_LSB` / (motor poles / 2). Per-motor DShot telemetry type/data and RPM are stored in DShot state / `dshotRpm[]`. The value is unsigned; direction is not represented. | Internal update is per DShot decode/loop, but no ESC sample timestamp is exported in MSP #139. The host's MSP polling cadence is not the ESC update cadence. |
| [`src/main/msp/msp.c`](https://github.com/betaflight/betaflight/blob/6dbc4218fd6bc33bf16ea32c670304d4f89321d5/src/main/msp/msp.c): MSP_MOTOR_TELEMETRY (#139) serializer | **SOURCE VERIFIED:** count + 13-byte per-motor record: uint32 mechanical RPM; uint16 invalid percent in hundredths of percent; uint8 temperature °C; uint16 voltage/current/consumption. DShot EDT values use whole-degree C, whole A and voltage quantized in 0.25 V steps (reported whole V); serial ESC sensor fields use centi-units. | Cached values; no timestamp/sequence. At the initial baseline EDT and `FEATURE_ESC_SENSOR` were OFF. A final EDT-ON trial still returned no auxiliary values through #139; see `REPORT.md`. Invalid packet percent is not an ESC-sourced fault counter. |
| Same file: MSP_SET_MOTOR (#214) | **SOURCE VERIFIED:** accepts at least two bytes per configured motor and writes `motor_disarmed[]`; mixer uses those values while disarmed. | Used only for the short test; final 1000 neutral command sent repeatedly and output verified by MSP_MOTOR #104. No stored config change. |
| [`src/main/flight/mixer.c`](https://github.com/betaflight/betaflight/blob/6dbc4218fd6bc33bf16ea32c670304d4f89321d5/src/main/flight/mixer.c): disarmed-motor branch | **SOURCE VERIFIED:** while not armed, motor outputs are assigned from `motor_disarmed[]`. | Explains why disarmed MSP motor test is possible on the observed build; it is not a MAVLink output path. |
| [`src/main/msp/msp_protocol.h`](https://github.com/betaflight/betaflight/blob/6dbc4218fd6bc33bf16ea32c670304d4f89321d5/src/main/msp/msp_protocol.h) | Defines MSP #139 as per-motor RPM/packet stats/ESC temp etc. | Definitions describe protocol intent, not availability of every field. Runtime capture established this quad's fields. |

## INAV 9.1

**Repository/revision:** `iNavFlight/inav`, tag [`9.1.0`](https://github.com/iNavFlight/inav/tree/9.1.0), source tree `e519b69b02e27c8bdc03b4a0889f1baaae211a54`.

| File / function | Evidence and fields | Rate / limitations |
|---|---|---|
| [`src/main/telemetry/mavlink.c`](https://github.com/iNavFlight/inav/blob/9.1.0/src/main/telemetry/mavlink.c): `processMAVLinkTelemetry()`, stream send functions | **SOURCE VERIFIED:** `SYS_STATUS`, RC channels, `GPS_RAW_INT` + `GLOBAL_POSITION_INT` + origin, `ATTITUDE`, `VFR_HUD`, `HEARTBEAT`, `BATTERY_STATUS`, `SCALED_PRESSURE`, and pending `STATUSTEXT`; separate `SYSTEM_TIME`. No ESC_INFO/STATUS, ESC_TELEMETRY_*, RPM, WHEEL_DISTANCE, HIGHRES_IMU or SCALED_IMU sender found in this module. | Official docs state stream group rates: EXT_STATUS 2 Hz, RC 1 Hz, POSITION 2 Hz, EXTRA1 3 Hz, EXTRA2 2 Hz, EXTRA3 1 Hz by default; configurable, per-group maximum poll 50 Hz. No runtime rate capture here. |
| Same file: `mavlinkSendPosition()` | **SOURCE VERIFIED:** `GPS_RAW_INT.fix_type` maps NONE/2D/3D to 1/2/3; MSL `alt=llh.alt*10`; `alt_ellipsoid=0`; `eph`/`epv` get `gpsSol.eph`/`.epv`; h/v accuracy get eph/epv ×10 mm; `yaw=0`, `hdg_acc=0`; `cog` is GPS course. `GLOBAL_POSITION_INT` uses MSL altitude, estimated relative altitude/velocity, attitude yaw for heading. | Runs with GPS or configured estimated fix. INAV's MAVLink docs describe only partial MAVLink compatibility. |
| [`src/main/io/gps.h`](https://github.com/iNavFlight/inav/blob/9.1.0/src/main/io/gps.h): `gpsSolutionData_t` | **SOURCE VERIFIED:** `eph` and `epv` are horizontal/vertical accuracy in centimetres; `hdop` is a separate generic HDOP scaled by 100. | This confirms the current MAVLink implementation's `eph`/`epv` assignment is a semantic mismatch: it sends cm accuracy where common MAVLink defines HDOP/VDOP ×100. |
| [`src/main/io/gps.c`](https://github.com/iNavFlight/inav/blob/9.1.0/src/main/io/gps.c): estimated-fix path | **SOURCE VERIFIED:** estimated GPS can set 3D fix and synthetic values (`eph=100`, `epv=100`, satellites=99 in the relevant code path). | Consumers should distinguish sensor fix from estimated fix; a reported 3D `fix_type` alone can overstate GNSS provenance. |

## MAVROS ROS 2

**Repository/revision:** `mavlink/mavros`, commit `5c68b905ab30de6ce630822dc46c33467e8f23ea`.

| File / function | Evidence and fields | Rate / limitations |
|---|---|---|
| [`mavros_extras/src/plugins/esc_status.cpp`](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros_extras/src/plugins/esc_status.cpp): `handle_esc_info()`, `handle_esc_status()` | Subscribes to common #290/#291 and publishes `~/info`, `~/status`. Uses `ESC_INFO.count` to size vectors; zero-based `index+i` vector mapping. Copies signed RPM int32 and voltage/current floats directly. | Status without prior INFO sees count=0 and publishes an empty array. Send INFO first/periodically. Last-batch publish logic is index-based. |
| [`mavros_msgs/msg/ESCInfoItem.msg`](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros_msgs/msg/ESCInfoItem.msg) | ROS temperature field is int32 and has no unit comment. | Plugin's `temperature = mavlink_temperature * 100`; MAVLink input is centi-degrees C. Unit contract needs correction/documentation. |
| [`mavros_extras/src/plugins/esc_telemetry.cpp`](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros_extras/src/plugins/esc_telemetry.cpp): `get_subscriptions()`, group handlers | Subscribes to legacy ArduPilotMega ESC groups 1–4, 5–8, 9–12 only. Converts centiV→V, centiA→A, mAh→Ah; direct-copies RPM/temp/count. | No 13–32 handlers today; add dialect types, five handlers, offsets/bounds and tests. |
| [`mavros_extras/src/plugins/wheel_odometry.cpp`](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros_extras/src/plugins/wheel_odometry.cpp) | Subscribes to ArduPilotMega RPM #226 and common WHEEL_DISTANCE #9000; publishes raw RPM/distance and calculated wheel odometry. | Plugin is separate from `esc_status`; it does not derive RPM or wheel distance from ESC_STATUS. |

## ArduPilot Rover 4.7.1

**Repository/revision:** `ArduPilot/ardupilot`, stable commit `dbe792162d06cab66c3475fd5556bf7a120f119e`.

| File / function | Evidence and fields | Rate / limitations |
|---|---|---|
| [`libraries/AP_ESC_Telem/AP_ESC_Telem.cpp`](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_ESC_Telem/AP_ESC_Telem.cpp): `send_esc_telemetry_mavlink()` | Emits ArduPilotMega `ESC_TELEMETRY_*` four-ESC batches; temperature °C, voltage cV, current cA, consumption mAh, telemetry count. Uses absolute RPM (`fabsf`) in unsigned legacy field. Can iterate through configured ESC groups up to compile-time maximum. | Triggered by ArduPilot's ESC telemetry stream scheduling; not a dedicated high-rate message per ESC. Some groups compile gated. RPM-only updates need not advance the telemetry data counter used for freshness. |
| [`libraries/GCS_MAVLink/GCS_Common.cpp`](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/GCS_MAVLink/GCS_Common.cpp): `send_rpm()`, interval/scheduler table | Emits `RPM` #226 from AP_RPM's two sensor instances when enabled; signed float values can be retained here. Schedules `ESC_TELEMETRY_1_TO_4` stream and legacy ESC group output. | RPM sensors are conceptually separate from AP_ESC_Telem per-ESC records. No common ESC_INFO/STATUS sender or message scheduler mapping found. |
| [`libraries/AP_ESC_Telem/AP_ESC_Telem.h`](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_ESC_Telem/AP_ESC_Telem.h) and backend headers | Internal list has per ESC and supports telemetry beyond the fields in legacy MAVLink: signed RPM source, measurement validity/freshness, voltage, current, temperature and consumption. | Availability/sign depends on backend (e.g. DShot eRPM has no sign). Mapping to standard messages is feasible, but source/validity needs careful handling. |
| [`Rover/GCS_MAVLink_Rover.cpp`](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/Rover/GCS_MAVLink_Rover.cpp): `send_wheel_encoder_distance()` | Emits common `WHEEL_DISTANCE` #9000 with sensor count and cumulative metres when Rover wheel encoder has sensors. | Requires configured wheel encoder; no observed runtime rate here. |

## AM32 firmware

**Repository/revision:** `am32-firmware/AM32`, commit `c232767910751fafd8b71f26c7a9a6bcdb4bf89d`.

| File / function | Evidence and fields | Rate / limitations |
|---|---|---|
| [`Src/kiss_telemetry.c`](https://github.com/am32-firmware/AM32/blob/c232767910751fafd8b71f26c7a9a6bcdb4bf89d/Src/kiss_telemetry.c): telemetry packet builder | **SOURCE VERIFIED:** KISS telemetry serializes temperature °C, voltage centivolts, current centiamps, consumed mAh and eRPM. | Requires serial telemetry wire/protocol, which was not enabled/verified on this FC. |
| [`Src/dshot.c`](https://github.com/am32-firmware/AM32/blob/c232767910751fafd8b71f26c7a9a6bcdb4bf89d/Src/dshot.c): EDT scheduler/reply path | **SOURCE VERIFIED:** EDT reply types include current, voltage and temperature in addition to eRPM. | ESC version/config dependent. A final runtime trial with Betaflight EDT ON still exposed no temperature/voltage/current in #139; exact ESC firmware/configuration is unknown. Upstream AM32 repo documentation advertises BiDi DShot and KISS telemetry. |
| MAVLink common `ESC_STATUS` definition | [MAVLink common message reference](https://mavlink.io/en/messages/common.html#ESC_STATUS) currently labels ESC_STATUS work in progress; it uses signed RPM, float voltage/current, recommended around 10 Hz. `ESC_INFO` is the lower-rate metadata/failure message, recommended around 1 Hz. | Upstream maturity is a compatibility risk for a new common sender. |

## Mowgli MAVROS bridge

**Repository/revision:** `Pepeuch/mowglimavros`, `main` commit `a1fe22c11171b0074a6a1771e249a0bba6c6c1b9` as fetched for this report.

| File / function | Evidence and fields | Rate / limitations |
|---|---|---|
| [`ros2/src/mavros_esc_wheel_odometry/src/esc_wheel_odometry_plugin.cpp`](https://github.com/Pepeuch/mowglimavros/blob/a1fe22c11171b0074a6a1771e249a0bba6c6c1b9/ros2/src/mavros_esc_wheel_odometry/src/esc_wheel_odometry_plugin.cpp) | **SOURCE VERIFIED:** receives RPM #226, reads chosen rpm1/rpm2 for wheels, gates freshness using ESC_TELEMETRY count fields, outputs `/wheel_odom`; subscribes legacy message groups 1–12. | Does not accept common ESC_STATUS; no direct WHEEL_DISTANCE handler in this plugin. |
| [`ros2/src/mowgli_mavros_bridge/include/mowgli_mavros_bridge/esc_telemetry_tracker.hpp`](https://github.com/Pepeuch/mowglimavros/blob/a1fe22c11171b0074a6a1771e249a0bba6c6c1b9/ros2/src/mowgli_mavros_bridge/include/mowgli_mavros_bridge/esc_telemetry_tracker.hpp) | **SOURCE VERIFIED:** advances freshness only when per-ESC telemetry counter progresses (including wrap handling); RPM alone does not refresh the sample. | ArduPilot's RPM-only DShot updates may fail this count freshness gate unless full telemetry data also advances. |

## Web documentation reviewed

- [INAV MAVLink guide, version 9.1](https://inavflight.github.io/docs/advanced-features/mavlink/) — partial MAVLink implementation, message list, stream rate groups/defaults, and known compatibility limits.
- [MAVROS `esc_status` plugin docs](https://mavros.readthedocs.io/en/stable/plugins/extras/esc_status/) and [`esc_telemetry` plugin docs](https://mavros.readthedocs.io/en/stable/plugins/extras/esc_telemetry/) — current ROS publishers and supported plugin message set.
- [MAVROS `wheel_odometry` plugin docs](https://mavros.readthedocs.io/en/stable/plugins/extras/wheel_odometry/) — ROS topics/parameters and #226/#9000 subscriptions.
- [MAVLink common ESC messages](https://mavlink.io/en/messages/common.html#ESC_INFO) — field definitions, units, recommended rates and current WIP notice for ESC_STATUS.
- [AM32 repository](https://github.com/am32-firmware/AM32) — upstream firmware feature summary. Source links above identify the packet details used in this report.
