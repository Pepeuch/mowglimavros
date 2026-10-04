# INAV USB/GPS/MAVLink investigation for Pepeuch

**Investigation date:** 2026-10-04
**Purpose:** Establish what this INAV aircraft actually exposes to a host, and how that maps to current Betaflight and MowgliNext/MAVROS source.

## Executive summary

- **Hardware detected:** INAV target `MAMBAF722_2022A`, firmware 9.1.0, running at 216 MHz with MPU6000 gyro/accelerometer. USB exposes one `STM32 Virtual ComPort` (serial identifier withheld; VID:PID `0483:5740`); INAV VCP is configured for MSP. The matching Betaflight DIAT target exists. The exact physical retail board marking and bootloader version were not read.
- **GPS setup and connection:** Configured UBLOX on serial identifier 1 (INAV USART2/UART2), 115200 baud, 8 Hz requested navigation rate, auto-baud/config enabled, dynamic model AIR_2G. In the repeat scan after reconnecting the forgotten cable, INAV identifies `UBLOX10 Proto: 34.09`, reports GPS sensor `OK`, and receives valid UBX packets continuously. The first scan was made before that cable was connected and is superseded for GPS-link conclusions.
- **Observed USB protocol:** The VCP answers MSP v1 read requests, and the same port supports INAV CLI mode. GPS_RAW_GPS (106), COMP_GPS (107), GPSSTATISTICS (166), and SENSOR_STATUS (151) all returned checksum-valid MSP v1 frames. No MAVLink was configured on USB: CLI reports `serial 20 1 ...` (VCP function MSP only). No settings were changed to expose MAVLink.
- **Observed GPS data:** In the repeat 180-second scan, all 2520 selected MSP requests succeeded with valid checksums. GPS packet count rose 181→3091 (about 16.2 valid UBX frames/s); solution/update interval observations were about 100–141 ms, consistent with the configured ~8 Hz, but no navigation fix was obtained indoors. MSP 106 stayed at zero fix/satellites and zero coordinates/speed/course; HDOP/EPH/EPV remained unknown sentinels. Thus the FC-to-receiver serial link and UBX parsing work; actual position, fix, accuracy, and outdoor performance remain unverified.
- **Source comparison:** INAV 9.1 and Betaflight 2026.6.2 both send MAVLink `GPS_RAW_INT` and `GLOBAL_POSITION_INT` when their GPS telemetry is active. Both also implement MSP command 106, but its final 16-bit field differs: INAV writes HDOP; Betaflight 2026.6.2 writes PDOP. INAV has richer MSP GPS statistics (166); this Betaflight release does not serialize that command. Both have a shared GPS-related MSP prefix, but this is not full wire/semantic identity.
- **MowgliNext consequence:** `feat/mavros-refresh` is an integration branch; the actual backend source is now in `Pepeuch/mowglimavros`. Its canonical GNSS path consumes `GPS_RAW_INT` into `/gps/fix` and `/gps/status`, not `GLOBAL_POSITION_INT`. Its current Universal GNSS adapter needs MAVLink2 accuracy extensions (`h_acc` and `v_acc`) for a usable covariance and needs `alt_ellipsoid` for canonical altitude. MAVLink1 lacks those extension fields, and INAV 9.1 currently sends zero for `alt_ellipsoid`; the canonical fix may therefore be visible but not accepted by fusion, and altitude becomes NaN. This is a concrete integration gap even when coordinates are present.
- **Compatibility estimate:** For a passive GPS decoder, expect roughly **70–80% reuse** if it handles MAVLink2 extensions and firmware-specific capability/freshness semantics. MSP's shared command-106 base is even more directly reusable through the ground-course field, but HDOP/PDOP and GPS statistics differ. The current complete Mowgli MAVROS backend is **not drop-in compatible**: no INAV/Betaflight launch profile exists, and the backend also expects health, power, IMU, and other contracts beyond GPS.
- **Restoration:** `RESTORATION STATUS: NOT VERIFIED`. Neither scan sent configuration writes, `save`, or a reboot command. In the repeat scan, stable port/GPS settings and mappings are unchanged; `diff all`/`dump all` differ only in gyro-zero X/Y and gravity calibration values. The cause and persistence of these calibration changes have not been proven, so they were not overwritten. No arming, motor, or deliberate servo command was issued.

Evidence labels used below: **OBSERVED** on the connected FC; **SOURCE** verified in the pinned source; **INFERRED** engineering interpretation; **UNKNOWN** not established.

## 1. Hardware

| Item | Finding | Evidence / confidence |
|---|---|---|
| INAV target | `MAMBAF722_2022A` | OBSERVED in `version`, MSP `FC_VARIANT`/`BOARD_INFO`; source confirms target name. |
| Commercial board | Likely DIATONE Mamba F722 2022A | INFERRED from matching DIAT Betaflight target `MAMBAF722_2022A`; no physical board label or silkscreen was inspected. |
| MCU | F722-class; status clocks 216 MHz | INAV target name identifies F722. Betaflight target metadata lists STM32F722. Exact MCU marking was not read from hardware. |
| Sensors | MPU6000 gyro/accelerometer; no compass; barometer unavailable; virtual pitot | OBSERVED in `status`. |
| USB device | Manufacturer string `INAV`; product `STM32 Virtual ComPort in FS Mode`; VID:PID `0483:5740` (`1155:22336` decimal); USB 2.0 full speed, 12 Mbit/s | OBSERVED from macOS IORegistry/USB descriptor properties. Device-level class is CDC (class 2). Host exposes the following FC serial nodes. |
| Serial devices | One FC USB CDC device; host serial identifier withheld | Other host ports are not enumerated in this public copy. |
| Bootloader | Not queried | UNKNOWN. No bootloader/DFU transition was attempted. |
| Betaflight target | `MAMBAF722_2022A` exists in the official DIAT target configuration | SOURCE. Target file and target explorer are linked in `SOURCE_NOTES.md`; the exact target-config revision bundled into Betaflight 2026.6.2 was not established. |

The corresponding Betaflight target metadata maps UART2 to PA2/PA3, UART3 to PB10/PB11, UART4 to PA0/PA1, UART5 to PC12/PD2, UART6 to PC6/PC7, and UART1 to PB6/PB7. That is target-source evidence; the INAV target's pin map and physical GPS wiring were not inspected at pin level.

## 2. Firmware and baseline configuration

`version` returned:

```text
INAV/MAMBAF722_2022A 9.1.0 Jul  8 2026 / 02:16:05 (15317503)
GCC-13.2.1 20231009
```

The installed build matches the INAV 9.1.0 release tag `e519b69b02e27c8bdc03b4a0889f1baaae211a54`. The source comparison below uses that release, rather than silently substituting the later 10.0.0 release candidate. Betaflight source is pinned to release 2026.6.2, commit `e0b7bb0`.

The first pass was captured before the forgotten GPS cable was connected. Its `GPS unavailable` state and zero UBX packets document that setup at that moment, but are superseded by the repeat scan. The repeat scan has selected raw MSP frames and a polling CSV in `runs/2026-10-04-gps-connected/raw/`; full CLI configuration dumps and capture scripts are retained locally but omitted from this public evidence bundle. Its initial status reports `GPS=OK`, `UBLOX10 Proto: 34.09`, and zero satellites; the device-reported current time remains `2041-06-28T01:04:00Z`, so treat its RTC value as untrusted.

Notable configuration captured in the backup:

| Setting | Initial value |
|---|---|
| `gps_provider` | `UBLOX` |
| GPS serial port | `serial 1 2 115200 115200 0 115200` (port identifier 1, function mask 2 = GPS) |
| `gps_auto_config` / `gps_auto_baud` | ON / ON |
| `gps_auto_baud_max_supported` | 230400 |
| `gps_ublox_nav_hz` | 8 |
| `gps_dyn_model` | AIR_2G |
| `gps_min_sats` | 6 |
| `gps_sbas_mode` | EGNOS |
| Galileo / BeiDou / GLONASS | ON / ON / ON |
| USB VCP | `serial 20 1 115200 115200 0 115200` (MSP only) |
| UART serial identifier 2 | function mask 33554432 = `FUNCTION_MSP_OSD` / `FUNCTION_VTX_MSP`, not GPS |
| ELRS receiver | CRSF configured on port identifier 3 (`FUNCTION_RX_SERIAL`, mask 64) |

Source confirms the meaning of the function masks: `FUNCTION_GPS=2`, `FUNCTION_MSP=1`, `FUNCTION_MSP_OSD=1<<25`, and MAVLink telemetry uses `FUNCTION_TELEMETRY_MAVLINK=256`. INAV serial identifier 1 is USART2, so the configured GPS interface is logically UART2. The target source pin map is not proof of which physical plug/wires carry the receiver.

## 3. GPS receiver and live state

### Configured receiver path

**OBSERVED configuration:** UBLOX provider, serial port identifier 1 / USART2, 115200 baud, auto-baud and auto-config on, max auto-baud 230400, 8 Hz requested navigation rate, AIR_2G model, minimum six satellites, EGNOS SBAS, and Galileo/BeiDou/GLONASS enabled.

**OBSERVED live state after the cable was connected:** `status` reports `GPS=OK`, hardware `UBLOX10`, protocol `34.09`, baud 115200, and MSP GPS-statistics packet counter increases steadily. `MSP_SENSOR_STATUS` marks GPS ready. This is direct evidence that the receiver is powered, connected to the configured FC port, and speaking a protocol INAV parses.

**OBSERVED navigation state:** Both before and after the 180-second repeat poll, `GPS_RAW_GPS` reported fix=0, satellites=0, zero coordinates/ground speed/course, and HDOP raw 9999 (99.99 unknown sentinel). GPS statistics report unknown HDOP/EPH/EPV. The craft was indoors during the scan, so no fix is unsurprising; the result does not demonstrate a configuration fault. It also does not prove the receiver can acquire a fix outdoors.

**UNKNOWN:** Receiver chipset/vendor and antenna condition; fix state and acquisition time outdoors; coordinates, valid altitude/speed/course, per-satellite data, and receiver-origin accuracy. Configured UBLOX is now corroborated by runtime UBLOX10 detection, but does not identify the exact receiver model.

The device did not produce a fix during either scan. Coordinates are deliberately omitted from this report. The original no-cable poll is in `raw/`; the repeat connected records are in `runs/2026-10-04-gps-connected/raw/`.

## 4. USB VCP behavior observed

The host sees an STM32 USB CDC virtual COM interface. With no settings changed, the same VCP was usable for the CLI backup and for MSP v1 request/reply. INAV's CLI was entered with the standard `#` command and left with `exit`; the port's configured function remains MSP. CLI and MSP are mode-switched on the same endpoint, not two simultaneous byte streams.

The initial serial row for VCP is identifier 20, function mask 1 (MSP). There is no MAVLink telemetry function on USB in the captured configuration. The source supports MAVLink telemetry on a serial port configured with `FUNCTION_TELEMETRY_MAVLINK`; this board's USB descriptor alone does not prove that MAVLink is compiled in or enabled there. Assigning MAVLink to the current VCP would risk losing the known MSP/CLI recovery channel, so no such change was attempted.

The MSP script sent only zero-payload read requests. For MSP v1 command 106, for example, request bytes are `$M<`, length `0`, command `0x6A`, checksum `0x6A`; the response begins `$M>`, length, command, payload, XOR checksum. Selected captures are in `raw/msp-readonly-frames.bin` and `raw/msp-readonly-poll.csv`.

## 5. MSP protocol results

### Runtime polling

The original scan ran 180 seconds with no GPS cable and is preserved as a diagnostic baseline. After the cable was connected, a fresh 180-second read-only poll requested each of 14 selected IDs 180 times (2520 total requests). All requests succeeded with valid XOR checksums in that repeat. Do not interpret the old scan's zero packet counter as the current receiver state.

| Command | MSP v1 ID | Replies | Median host request→reply | Runtime result |
|---|---:|---:|---:|---|
| API_VERSION | 1 | 180/180 | — | API payload `00 02 05` (MSP protocol 2.5, API 2) |
| FC_VARIANT / FC_VERSION / BOARD_INFO | 2 / 3 / 4 | 180/180 each | — | INAV, firmware version 9.1.0, board identifier `M72A`, target string `MAMBAF722_2022A` |
| STATUS | 101 | 180/180 | — | Valid; legacy status packet |
| RAW_GPS | 106 | 180/180 | ~9.9 ms | In the repeat scan payload is invariant: no fix/satellites; zero location/altitude/speed/course; HDOP raw 9999 = unknown sentinel |
| COMP_GPS | 107 | 180/180 | ~9.9 ms | Distance/bearing/home heartbeat fields stayed zero |
| SENSOR_STATUS | 151 | 180/180 | ~9.7 ms | GPS health byte reports ready; matches `status` |
| GPSSTATISTICS | 166 | 180/180 | ~9.8 ms | In repeat scan `lastMessageDt` ranged about 100–141 ms (median ~120 ms); errors=1; timeouts=0; packetCount 181→3091; HDOP/EPH/EPV remain 9999; hwVersion 74 (`UBLOX10`) |
| Unknown probe | 122 | error 180/180 in original scan | — | `$M!` error in the original no-cable scan; excluded from the connected repeat |

For 106, the repeat scan's response latency remains about 10 ms. This is host request/reply latency on this VCP, not GPS measurement latency. The rising packet count proves valid UBX frames reach and parse at the FC; `lastMessageDt` reflects solution/update timing and is consistent with approximately 8 Hz, but is not evidence of a valid fix or outdoor performance. Packet count counts valid UBX frames, not one navigation solution per packet.

### Useful MSP payload layouts in INAV 9.1

All multibyte values are little-endian. The documented framing below is source-verified at the pinned INAV release; actual runtime response was observed for 101, 106, 107, 118, 151, and 166.

| Message | ID / protocol | Request | Reply fields and units | INAV/BF note |
|---|---|---|---|---|
| `MSP_RAW_GPS` | 106 / v1 | No payload | 18 bytes: `u8 fix`, `u8 satellites`, `i32 lat`, `i32 lon`, `u16 altitude m`, `u16 ground speed cm/s`, `u16 ground course` in 0.1° units, `u16 HDOP` (0.01). Lat/lon use 1e-7 degrees. | Runtime response observed. Betaflight 2026.6.2 has the same first 16 bytes but calls the final `u16` **PDOP**, not HDOP. Do not decode the tail identically. |
| `MSP_COMP_GPS` | 107 / v1 | No payload | 5 bytes: `u16 distance_home`, `u16 direction_home`, `u8 gps_heartbeat/update`. | Runtime response observed. Same broad purpose/5-byte shape in BF; check bearing scale in the implementation before reuse. |
| `MSP_GPSSTATISTICS` | 166 / v1 | No payload | 21 bytes: `u16 lastMessageDt`, `u32 errors`, `u32 timeouts`, `u32 packetCount`, `u16 HDOP`, `u16 EPH`, `u16 EPV`, `u8 hwVersion`. Units for distance-accuracy fields are not assumed here. | Runtime observed and useful on INAV. No corresponding serializer was found in BF 2026.6.2. |
| `MSP_GPSSVINFO` | 164 / v1 | No payload | INAV compatibility stub, five bytes, not actual per-satellite data. | Betaflight serializes actual satellite channel/SVID data when available; not semantically interchangeable. |
| `MSP_SENSOR_STATUS` | 151 / v1 | No payload | Nine `u8`: overall sensor health, gyro, accelerometer, compass, barometer, GPS, rangefinder, pitot, optical flow health. | Runtime observed; GPS slot says ready in the connected repeat. |
| `MSP_STATUS` | 101 / v1 | No payload | Deprecated status summary. | Runtime observed; INAV's extended `MSP2_INAV_STATUS` is the richer INAV status path. |
| `MSP_WP` | 118 / v1 | Waypoint index `u8`; waypoint 0 is home | 21-byte waypoint structure (number/action, lat/lon, altitude cm, parameters and flags) when navigation home/waypoint is set. | Runtime response was 21 bytes with zero position/altitude fields; no home coordinates were established. |
| `MSP2_SENSOR_GPS` | `0x1F03` / MSP v2 | Sensor input message, not a read request | Packed 51-byte rich GPS input: instance, GPS week/TOW, fix/sats, h/v position and horizontal-velocity accuracy, HDOP, lat/lon, MSL altitude cm, N/E/down velocity, course/yaw, date/time. | INAV input/provider path; not observed as a USB reply. Distinct from legacy inbound `MSP_SET_RAW_GPS` 201. |

The exact INAV serializer/source links, and the Betaflight serializer comparison, are in `SOURCE_NOTES.md`. A decoder should inspect frame length and firmware/API capability rather than assume every ID has the same trailing fields.

## 6. MAVLink findings

### Connected board

No MAVLink packets were captured from the FC. The captured serial table assigns VCP to MSP and no port to MAVLink telemetry. No temporary configuration was applied: GPS was unavailable, and changing the VCP function could have removed the current control/configuration path without adding useful GPS runtime evidence.

### Source capability (not runtime proof)

INAV 9.1 has a MAVLink telemetry sender when `USE_TELEMETRY_MAVLINK` is compiled and a serial port is assigned MAVLink telemetry. INAV's `mavlink_version` supports MAVLink 1 or 2 and defaults to 2. Telemetry stream rates are configurable; a zero stream rate disables that stream. GPS position output is suppressed unless GPS sensor state or an estimated fix is active.

| Message | INAV 9.1 source | Betaflight 2026.6.2 source | Practical Mowgli relevance |
|---|---|---|---|
| HEARTBEAT (#0) | Present; independent 1 Hz scheduling | Present in EXTRA2 stream | MAVROS `/mavros/state`; target system/component must match. |
| SYS_STATUS (#1) | Present in EXTENDED_STATUS | Present in EXTENDED_STATUS | Not sufficient for Mowgli's canonical battery/power observer by itself. |
| GPS_RAW_INT (#24) | Present in position stream; GPS gate | Present in position stream; GPS gate | Feeds MAVROS raw GNSS and Mowgli Universal GNSS. |
| GLOBAL_POSITION_INT (#33) | Present in position stream; GPS gate | Present in position stream; GPS gate | MAVROS global topic; not required by current canonical raw-GPS adapter. |
| LOCAL_POSITION_NED (#32) | No sender in pinned telemetry source | No sender in pinned telemetry source | Do not assume local odometry from these FCs. |
| ATTITUDE (#30) | Present in EXTRA1 | Present in EXTRA1 | Can feed MAVROS IMU orientation/rates; actual values must be checked on target. |
| VFR_HUD (#74) | Present in EXTRA2 | Present in EXTRA2 | Telemetry summary; not a replacement for the raw GNSS contract. |
| HOME_POSITION (#242) | No sender found; INAV emits GPS_GLOBAL_ORIGIN (#49), a different message | Present, requires home fix | Incompatible assumptions if client treats GPS_GLOBAL_ORIGIN and HOME_POSITION as interchangeable. |
| SYSTEM_TIME (#2) | Present, independently scheduled at 1 Hz | Present, coupled to SYS_STATUS output cadence | Device RTC validity matters; this FC reported a nonsensical 2041 UTC timestamp. |

INAV GPS_RAW_INT packs `time_usec`, fix, lat/lon, MSL altitude in mm, EPH/EPV, ground speed, course, satellite count, plus MAVLink2 extensions. INAV sets `alt_ellipsoid=0`, maps h/v accuracy fields from its EPH/EPV values, and sends no GPS_RAW_INT until GPS/estimated fix state is active. Its time is RTC epoch if available, else local micros; this device's RTC readout is suspect.

Betaflight 2026.6.2 uses MAVLink2 in the inspected sender and sends GPS_RAW_INT with h/v accuracy from GPS solution accuracy fields. It sets `alt_ellipsoid` numerically equal to its MSL altitude field. That is a source fact about the number sent, not evidence that the number is a true WGS84 ellipsoid altitude. Betaflight also sends HOME_POSITION when its home fix is set. INAV 9.1 does not send HOME_POSITION in the pinned sender.

## 7. INAV versus Betaflight compatibility

### MSP

- **Transport framing:** MSP v1 framing and the basic request/reply mechanism are common. This FC's USB CDC VCP speaks MSP v1 now; another Betaflight board still needs a correctly assigned MSP-capable VCP/UART.
- **Command 106:** Both releases serialize the same first 16 bytes through ground course. The final two bytes differ: INAV = HDOP, Betaflight 2026.6.2 = PDOP. Both frame payloads are 18 bytes in these pinned releases. A parser may reuse the core decoder, but must capability-tag the last field.
- **Command 107:** Both expose distance-to-home, direction-to-home and update/heartbeat in five bytes; normalize direction units from each source before treating the fields as identical.
- **Command 166:** INAV exposes receiver parser statistics; no BF 2026.6.2 output serializer was found. Treat as INAV-only here.
- **Command 164:** INAV is a compatibility stub, while BF returns satellite-channel data. Not equivalent.
- **Rich MSPv2:** INAV has an inbound sensor GPS packet with accuracy, ENU/NED velocity and time. That is not evidence that a host can poll those same fields from INAV or that Betaflight implements an equivalent endpoint.

### MAVLink

Both pinned releases schedule HEARTBEAT, SYS_STATUS, GPS_RAW_INT, GLOBAL_POSITION_INT, ATTITUDE, VFR_HUD, and SYSTEM_TIME. Neither pinned sender packs LOCAL_POSITION_NED. INAV emits GPS_GLOBAL_ORIGIN but not HOME_POSITION; Betaflight emits HOME_POSITION when a home fix exists. INAV can send MAVLink 1 or 2 by setting; BF sender uses MAVLink2. GPS output remains gated on a GPS sensor/fix and configured telemetry stream/port.

### Reuse estimate

These percentages are engineering estimates from source, not measured cross-flashing or flight evidence:

| Layer | Expected reuse on Betaflight | Why |
|---|---:|---|
| Serial framing / MSP transport | ~90% | Same protocol framing; USB VCP and serial mapping are board/firmware config details. |
| MSP raw GPS parser | ~80% for an adaptive parser; lower for fixed-field code | Shared 16-byte prefix, but trailing DOP semantic differs; stats/satellite messages differ. |
| MAVLink GPS decoder | ~75% when using MAVLink2 and honoring dialect/field validity | Shared GPS_RAW_INT/GLOBAL_POSITION_INT core; BF fills extensions differently and HOME_POSITION differs. |
| GPS data meaning / freshness | ~60% | Fix/sats/coordinates/speed/course broadly map, but accuracy, altitude datum, time and freshness are not identical. |
| Entire current Mowgli backend unchanged | **<50%** | No INAV/BF profile; GPS covariance/ellipsoid requirements; readiness also expects heartbeat, fresh IMU and canonical power; rover mode/command assumptions cannot be reused for aircraft control. |

If Pepeuch writes a passive GPS decoder against this INAV board, he can reuse most core coordinate/fix decoding on Betaflight. If he builds against the existing full Mowgli MAVROS backend, it will not work unchanged merely because GPS_RAW_INT exists.

## 8. MowgliNext / Pepeuch backend mapping

The current `feat/mavros-refresh` branch is at `b18e6c394a2bed9bd0e60880395cd3a171ff10f7`; it is an integration/installer contract, not the backend implementation. It is 18 commits ahead and 34 behind current `dev` (`6f37770878e4558d385320664ac737d91a8b5eea`). The actual sidecar source inspected is `Pepeuch/mowglimavros` main at `a1fe22c11171b0074a6a1771e249a0bba6c6c1b9`, using MAVROS 2.16.0 and Universal GNSS commit `6f0eb09ff48893ad56c70956266f19e5a775552c`. MowgliNext selects a floating `:latest` sidecar tag, so source inspection does not identify a deployed image.

| MowgliNext need | MAVROS topic / MAVLink | INAV provides? | Betaflight provides? | Confidence / notes |
|---|---|---|---|---|
| FC connection, armed/mode state | `/mavros/state` / HEARTBEAT #0 | Source sender present | Source sender present | Source-only; sysid/compid and custom mode must match and be validated. |
| Canonical GNSS fix/status | `/gps/fix`, `/gps/status` from Universal GNSS GPS1_RAW #24 | Source can emit if GPS sensor/fix active; this FC emitted none on MSP | Source can emit if GPS sensor active | No MAVLink runtime capture. MSP observed no fix. |
| GPS2 path | GPS2_RAW #124 | Not established from this source path | Not established from this source path | Backend requires actual GPS2_RAW if configured for GPS2; no synthesized GPS1 fallback. |
| Covariance accepted by fusion | canonical `NavSatFix` covariance from `h_acc` + `v_acc` | V2 extensions source-populated, but values and Mowgli acceptance not runtime-tested; MAVLink1 lacks them | V2 extensions source-populated from GPS accuracy | Universal GNSS only fills covariance if both available; fusion rejects unknown/zero/nonfinite covariance. |
| Canonical ellipsoid altitude | `GPS_RAW_INT.alt_ellipsoid` extension | INAV sends 0; adapter outputs NaN | BF sends MSL numeric value as `alt_ellipsoid` | BF datum correctness remains unproven despite a nonzero value. |
| Generic MAVROS global position | `/mavros/global_position/global` / GLOBAL_POSITION_INT #33 | Source sender present | Source sender present | Separate from canonical raw GPS path. |
| IMU/readiness | `/mavros/imu/data` / ATTITUDE #30 and other sensor inputs | ATTITUDE source sender present | ATTITUDE source sender present | Actual rate and values not runtime-verified. Full bridge also expects connected state and power. |
| Power readiness | `/hardware_bridge/power`, BATTERY_STATUS #147 through observer | Not checked on this FC | Not checked on this FC | SYS_STATUS alone does not fulfill the current power observer. Battery and ESC investigation is out of scope. |
| Wheel odometry | `/wheel_odom` | Not provided by GPS_RAW_INT or GLOBAL_POSITION_INT | Not provided by GPS_RAW_INT or GLOBAL_POSITION_INT | Current backend does not relabel local position as wheel odometry; wheel input is optional by default. |
| Local NED odometry | MAVROS local-position topics / LOCAL_POSITION_NED #32 | No sender found | No sender found | Do not map to wheel odometry. |
| System time | MAVROS time sync / SYSTEM_TIME #2 | 1 Hz sender | Coupled to SYS_STATUS cadence | Connected INAV status reported an invalid-looking 2041 date/time. |
| Flight mode/control assumptions | MAVROS profile and command plugin | No INAV profile in current launch | No Betaflight profile in current launch | Launch accepts `ardupilot`, `apm`, `px4`; ArduRover y-steering/z-throttle and HOLD/disarm behavior must not be reused for an aircraft. |

**Two blocking GNSS details for unchanged canonical Mowgli fusion:**

1. Universal GNSS creates canonical `NavSatFix` covariance only if both horizontal and vertical accuracy are available. MAVLink1 GPS_RAW_INT lacks those extension fields. Mowgli `fusion_graph` rejects unknown or zero/nonfinite covariance, so coordinates/status can appear without becoming accepted localization data.
2. The canonical altitude adapter deliberately refuses to relabel MSL altitude as ellipsoid altitude. INAV sends zero in the extension, so canonical altitude becomes NaN. Betaflight 2026.6.2 sends its MSL numeric value in that field; this passes the nonzero check but does not prove datum correctness.

Universal GNSS also stamps packet receipt locally and advances its observation sequence for every GPS_RAW_INT packet; a cached/retransmitted position can look like a new transport observation. The source keeps `time_usec` as metadata but does not deduplicate by measurement epoch. Measurement freshness needs a provider-side check.

## 9. Interface recommendation

| Option | Simplest | Robustness/maintenance | Latency evidence | Assessment |
|---|---|---|---|---|
| A. MAVLink → MAVROS | Best fit to the current Mowgli interface and standard ROS topics | Lowest custom protocol surface; requires an INAV/BF profile and deliberate handling of accuracy/altitude/freshness | No FC MAVLink stream was enabled or measured. GPS telemetry rate is configurable/source-scheduled. | Recommended starting point for the Mowgli backend once GNSS-field and profile gaps are addressed. |
| B. MSP → custom bridge | Easiest way to poll FC-native basic GPS and INAV diagnostics on this exact board | More custom parser/version management; Betaflight and INAV tails/statistics differ | Observed VCP transaction median ~9.9 ms for 106/107/166, but no GPS fix; this is not navigation-data latency. | Useful for direct Betaflight MSP integration or diagnostics; not a drop-in MAVROS contract. |
| C. Hybrid | More work initially | Best reach: standard MAVLink for shared state/GNSS, MSP for source-specific diagnostics/capabilities | Cannot rank actual GNSS age from this no-fix capture | Best long-term design if both standard integration and Betaflight-specific diagnostics are needed. Keep exactly one authoritative GPS fix in fusion. |

**Recommendation:** Start with MAVLink/MAVROS for the common GPS/state path because Pepeuch's current ROS backend is already structured around it. Add a narrowly scoped MSP companion only if Betaflight-specific GPS status, home/statistics, or capability data is needed. Do not fuse duplicate MAVLink and MSP positions as independent observations. Implement explicit firmware/capability metadata for DOP-vs-accuracy, altitude datum, message version, and freshness. The evidence does not support claiming that MAVLink has lower real GPS latency than MSP here; no live GPS update was available.

## 10. Unknowns and tests needed on a Betaflight board

- Inspect a real Betaflight `MAMBAF722_2022A` (or the intended board) with its exact release, target, USB descriptor, and port map; target existence is not proof of VCP availability/configuration.
- With a GPS fix, compare observed MSP 106/107/164/166 layouts, DOP fields, satellite usage, GPS stats, and timestamps against the pinned code. Current FC could not answer this about actual receiver values.
- Capture Betaflight MAVLink2 GPS_RAW_INT, GLOBAL_POSITION_INT, HOME_POSITION, ATTITUDE, HEARTBEAT, SYS_STATUS and SYSTEM_TIME on a dedicated telemetry UART; measure stream periods and sender IDs. Compare ellipsoid-altitude and accuracy semantics to hardware output.
- Run the actual pinned Mowgli sidecar against recorded packets or a bench FC to prove topic names, raw message extension parsing, covariance, altitude datum, sysid/compid matching, readiness, and whether fusion accepts the fix.
- Verify passive freshness when GPS receiver updates stop but telemetry continues. Packet-receipt freshness alone is insufficient.
- Establish a known RTC/time source and test system-time output; this INAV board reported 2041.
- Keep this aircraft unarmed; no motor start or servo movement is needed for these read-only protocol tests. For a future telemetry-port configuration, preserve the MSP/CLI recovery port and review before any setting is written.

## 11. Restoration verification

**RESTORATION STATUS: NOT VERIFIED.** No configuration mutation, `save`, or intentional reboot command was sent. In the connected repeat scan, `serial`, `resource`, `feature`, `map`, `get gps`, and `get serial` are unchanged between snapshots. `diff all` and `dump all` differ only in these runtime IMU calibration values:

| Field | Before | After |
|---|---:|---:|
| `gyro_zero_x` | -63 | -65 |
| `gyro_zero_y` | 9 | 12 |
| `gyro_zero_z` | -10 | -10 |
| `ins_gravity_cmss` | 988.236 | 975.096 |

Connected-scan status uptime was 93 seconds before and 213 seconds after; that increase is shorter than the wall-clock interval between captures, for an unexplained reason. The earlier no-cable pass had a separate uptime inconsistency, so uptime should not be used to infer whether an unobserved reset occurred. INAV source allows gyro/gravity calibration to update at runtime; this is a plausible explanation for changed calibration fields but has not been proven here. No attempt was made to set or save old sensor calibration values. No arming, motor command, deliberate servo command, ESC telemetry investigation, or MAVLink configuration change occurred. Full configuration snapshots are retained locally but not included in this public bundle; selected raw MSP traces are included under `runs/2026-10-04-gps-connected/raw/`.

## 12. Evidence files

- Selected no-cable and connected-repeat MSP request/response logs are included under `raw/` and `runs/2026-10-04-gps-connected/raw/`.
- Full CLI config snapshots and host-specific USB enumeration are retained locally but omitted because they contain unrelated aircraft settings and machine identifiers. The connected test was read-only and the report records that restoration limitations remain.
- Source permalinks/commit references: `SOURCE_NOTES.md` and `MOWGLI_MAVROS_SOURCE_FINDINGS.md`.
- `checksums.sha256` records file hashes (manifest itself excluded).
