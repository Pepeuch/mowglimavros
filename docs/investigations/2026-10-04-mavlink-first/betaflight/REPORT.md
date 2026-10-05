# MAVLink-first Betaflight / INAV / MAVROS investigation

**Prepared for Pepeuch — 4 October 2026**

## Executive summary

- **The architecture is plausible for common navigation and vehicle telemetry, but it does not work as a firmware-free ESC contract today.** Betaflight 2026.6.1 and INAV 9.1 emit overlapping standard MAVLink position, attitude, status and battery messages. Neither sender currently emits `ESC_INFO` (#290), `ESC_STATUS` (#291), `ESC_TELEMETRY_*`, `RPM` (#226) or `WHEEL_DISTANCE` (#9000).
- The tested Betaflight FC currently has **no MAVLink serial output enabled**. Its USB VCP is MSP-only; this is a configuration observation. The sender audit is source-verified; we did not alter routing or capture MAVLink packets on the quad.
- On this quad, bidirectional DShot is working. Four individual motors produced per-motor mechanical RPM in Betaflight `MSP_MOTOR_TELEMETRY` (#139), and all four produced RPM in a short simultaneous sample. That makes MSP useful for diagnosis and for validating the FC-to-ESC RPM path, but does not make it the desired cross-firmware MAVROS interface.
- The installed FC reports `DSHOT600`, bidirectional DShot ON and motor pole count 12. A final temporary `dshot_edt=ON` test still produced no per-ESC temperature, voltage, current or consumption in MSP #139; EDT was then restored to OFF and verified after reboot. The quad/ESC was described as AM32 by the user, but its firmware build could not be identified, so the runtime result does not distinguish unsupported ESC firmware/configuration from an FC decode limitation.
- With output values 1100, 1150 and 1200, stable reported RPM was approximately 6.7–6.9k, 9.6–9.8k and 12.5–12.8k respectively. The first RPM sample arrived by the next host poll; it settled within roughly 0.1 s in this short bench run. MSP polling was about 30 Hz, so these are host-observed latencies, not the ESC's native sample rate.
- Betaflight's MSP #139 gives unsigned mechanical RPM and a rolling DShot invalid-packet percentage. It does not carry a sample timestamp, packet sequence, or rotation direction. Its current RPM therefore cannot alone distinguish forward from reverse wheel rotation.
- Betaflight's MAVLink GPS sender puts MSL altitude into **both** `alt` and `alt_ellipsoid`. The latter is not a valid ellipsoid altitude. Betaflight otherwise sends h/v accuracy in millimetres from its GPS accuracy values, HDOP/VDOP in `eph`/`epv`, and zero for GPS yaw.
- INAV 9.1 sets `alt_ellipsoid=0`, correctly avoiding a false MSL-as-ellipsoid claim. Its `h_acc`/`v_acc` are derived from its centimetre accuracy estimates; however, it also puts those accuracy estimates directly into `eph`/`epv`, whose MAVLink meaning is HDOP/VDOP ×100. This is a semantic mismatch. Its GPS yaw is zero; its `cog` is course over ground, not vehicle heading.
- MAVROS `esc_status` consumes common `ESC_INFO` and `ESC_STATUS`. `ESC_INFO` is operationally needed first: without it the plugin has no ESC count and publishes an empty status vector. ESC_STATUS RPM sign, voltage and current are copied directly. **The plugin multiplies ESC_INFO temperature by 100**, though MAVLink input is already centi-degrees C; its ROS message does not document another scaling convention, so this appears to be an exposure bug.
- MAVROS `esc_telemetry` currently subscribes only to ArduPilotMega groups for ESCs 1–12. Support for 13–32 requires five additional message handlers/groups, dialect generation and tests. `RPM` and `WHEEL_DISTANCE` are handled by MAVROS' separate wheel-odometry plugin.
- The Mowgli MAVROS wheel plugin currently uses ArduPilot `RPM` #226, gated by per-ESC packet counters from `ESC_TELEMETRY_*`; it does not currently consume common `ESC_STATUS`/`ESC_INFO`. Changing the FC to standard ESC messages alone would therefore not feed this odometry path.
- ArduPilot already has signed ESC RPM and richer internal ESC telemetry, but its MAVLink sender emits legacy `ESC_TELEMETRY_*` (unsigned absolute RPM) and `RPM` when configured. Source does not document a deliberate rejection of ESC_INFO/STATUS; adding those senders looks technically feasible. The main upstream caveat is that MAVLink currently marks common `ESC_STATUS` **work in progress**, and it does not carry ESC consumption or packet freshness counters.
- Recommendation: keep MSP for diagnostics/configuration; make MAVLink the primary interface for shared GPS/attitude/battery/status. For odometry, prototype a hybrid/common extension: add an ESC_STATUS receiver to Mowgli, preserve legacy RPM + ESC_TELEMETRY support during migration, and only standardize on ESC_INFO/STATUS after deciding how to handle its WIP status, timestamp/freshness and consumption gaps. Betaflight and INAV need firmware sender work before that common ESC contract exists.
- The final configuration matches the original for the setting changed during the last test: `dshot_edt=OFF` was verified after reboot. The temporary motor override was returned to neutral; post-test MSP showed all four outputs at 1000. The earlier full snapshots still match exactly for the original motor sweep. See [`restoration.json`](restoration.json).

## 1. Test target and evidence boundary

The physical target was a `Betaflight _ HDZERO_HALO` USB CDC device, VID:PID `0483:5740` (USB serial identifier withheld), running Betaflight `2026.6.1` build key `c90887965570eeb2e63faefc3b5eddd8`, target `HDZERO_HALO`, STM32H743 at 480 MHz, ICM42688P gyro/accelerometer and 16 MB flash. The CLI exposes four motor outputs. Battery voltage at baseline was 23.13 V (6S). The final attempt began at 76 °C FC core temperature. Betaflight reported 83 °C after enabling EDT, and 92 °C after the short final motor check and restoring EDT. These are **FC MCU core temperatures**, not ESC temperatures. Battery voltage remained 22.69–22.71 V (6S OK).

The FC configuration says DSHOT600, bidirectional DShot ON, DShot bitbang ON, EDT OFF, 12 motor poles, `FEATURE_ESC_SENSOR` OFF, and GPS NOT ENABLED. No MAVLink port is assigned: VCP is MSP and UART5 is MSP+VTX MSP. The user identified the ESCs as AM32, but model and firmware version are not exposed by the saved Betaflight snapshot. The exact AM32 installation and per-ESC sensors remain unverified.

Evidence tags used below:

- **OBSERVED**: measured on this quad during this session.
- **SOURCE VERIFIED**: present/absent in the pinned source implementation.
- **DOCUMENTED**: stated in upstream documentation.
- **INFERRED**: likely explanation or engineering conclusion, not explicitly stated by upstream.
- **UNKNOWN**: not established here.

The MAVLink message matrix is source-based, not a runtime packet capture: this quad had GPS disabled and no MAVLink serial port assigned. We did not modify VCP/UART routing merely to create a capture. In particular, generated MAVLink message definitions are not treated as proof of a sender.

## 2. Betaflight ESC path and runtime RPM

### Runtime result

**OBSERVED:** idle bidirectional-DShot statistics reported all four motors' eRPM channel (`R----`) with 0.00% invalid, and the read-only MSP baseline returned four records from message #139. During the motor test, the captured #139 samples showed each motor's RPM increase independently with its own output; the other three remained at zero. At all-four output 1100, all four returned RPM. No MSP checksum or frame errors occurred in the 575 test exchanges.

| MSP_SET_MOTOR external output | Motor 1 | Motor 2 | Motor 3 | Motor 4 | Interpretation |
|---:|---:|---:|---:|---:|---|
| 1100 | 6,850 RPM | 6,867 RPM | 6,783 RPM | 6,683 RPM | Stable samples; early ramp samples excluded from the representative value |
| 1150 | 9,767 RPM | 9,733 RPM | 9,617 RPM | 9,617 RPM | Stable samples |
| 1200 | 12,717 RPM | 12,850 RPM | 12,633 RPM | 12,600 RPM | Stable samples |

These are Betaflight-reported motor-shaft RPM estimates, not independent optical tachometer measurements. `MSP_MOTOR_TELEMETRY` samples arrived at about 30 Hz from the host polling loop. The first sample was received on the next poll and values were near their stable band within about 40–80 ms after that first RPM sample (roughly 0.1 s from output change, limited by polling cadence). A precise ESC-side update frequency was not measured.

**OBSERVED:** with EDT OFF, MSP #139 records contained nonzero RPM but temperature, voltage, current and consumed mAh were zero. In the final check, EDT was temporarily enabled, each motor was individually run at output 1150 for about 0.55 s, and RPM rose to roughly 9.4–9.7k; temperature, voltage, current and consumption still remained zero. This confirms no such fields reached Betaflight #139 in this test, not that the ESCs measured zero. Exact ESC firmware/configuration remains unknown.

### Source mapping and semantics

- `src/main/drivers/dshot.c`, `dshotTelemetryDecode()` and the bidirectional-DShot processing path decode the GCR response, store the received DShot telemetry type/data and update `dshotRpm[]` via `erpmToRpm()`.
- Betaflight computes `erpmToHz = ERPM_PER_LSB / 60 / (motorPoleCount / 2)`; `erpmToRpm()` therefore returns `raw_eRPM * ERPM_PER_LSB / pole_pairs`. In this build the configured pole count is 12 (six pole pairs), so the conversion is approximately raw eRPM ×100/6 to mechanical shaft RPM. Incorrect motor-pole configuration scales the reported RPM incorrectly.
- `msp.c`, `MSP_MOTOR_TELEMETRY` (#139) emits `motorCount` then 13 bytes per motor: `uint32 rpm` (mechanical shaft RPM), `uint16 invalidPct` (1/100 percent; 10000 = 100.00%), `uint8 temperature` (°C), `uint16 voltage`, `uint16 current`, `uint16 consumption` (mAh). The DShot EDT branch supplies whole-degree C, whole amp, voltage in 0.25 V steps converted to whole volts; the separate ESC-sensor branch uses centi-volts/centi-amps in the same fields. This source-level unit asymmetry means a consumer must know the acquisition path; do not blindly apply one scale to every #139 record.
- MSP #139 contains no timestamp or sample counter. Polling more often than Betaflight refreshes the cached eRPM can repeat the same value. For odometry, add a receiver-side timestamp and freshness/quality gating; the FC protocol itself does not give a per-record age.
- `getDshotRpm()` and the DShot eRPM are unsigned. The quad's yaw-motor reverse option is a configured output-direction choice, not a sign bit in the RPM telemetry. This bench run only produced positive RPM. A mower needs independent forward/reverse knowledge, signed ESC data, or another direction sensor before signed distance can be trusted.
- Betaflight keeps DShot invalid packet statistics and may decode EDT temperature/current/voltage and state/event telemetry types if the ESC and FC settings support them. Those invalid-packet statistics describe the FC's received DShot frames; they are not ESC-reported error counters. No standard per-ESC failure count, duty cycle, stress, or desync field was observed through the current setup.

### AM32 capability versus this quad

**DOCUMENTED/SOURCE VERIFIED:** current upstream AM32 advertises bidirectional DShot and KISS serial ESC telemetry. Its KISS telemetry packet carries temperature (°C), voltage (centivolts), current (centiamps), consumed mAh and eRPM. AM32's DShot EDT source schedules temperature, voltage and current extended frames. Betaflight's `dshot_edt=OFF` originally meant this quad was not requesting/decoding those values. A short runtime trial with EDT ON still returned only RPM via #139; the final setting was restored to OFF. EDT availability depends on the actual ESC firmware and its configuration; the AM32 version/support was not read from hardware, so the cause of absent fields is unresolved.

AM32 firmware has internal control/desync logic, but the inspected KISS telemetry packet and the DShot EDT values do not expose a generic desync counter, stress estimate or duty-cycle field to Betaflight. The exact ESC diagnostics available to this specific AM32 build are **UNKNOWN** until its version and protocol responses are read directly.

## 3. Current MAVLink senders: Betaflight and INAV

### Message matrix

Rates are configured through firmware stream groups. When a group is disabled, its messages do not stream; some messages also require a sensor/feature. No runtime MAVLink rates were measured on this quad.

| Message | Betaflight 2026.6.1 source | INAV 9.1 source/docs | Rate / condition / fields relevant here |
|---|---|---|---|
| `SYS_STATUS` #1 | Yes | Yes | Configured EXTENDED_STATUS group. System sensor/load and FC battery status; not ESC status. |
| `RC_CHANNELS_RAW` #35 or `RC_CHANNELS_SCALED` #34 | BF sends RAW | INAV documents RAW under MAVLink v1 and SCALED under v2 | RC stream group; not actuator/motor telemetry. |
| `GPS_RAW_INT` #24 | Yes, if GPS compiled and detected | Yes when GPS sensor or configured estimated fix is available | Position group; see GPS section below. |
| `GLOBAL_POSITION_INT` #33 | Yes, if GPS compiled and detected | Yes | Position group; MSL altitude, relative altitude estimate, velocity and vehicle yaw/heading. |
| `GPS_GLOBAL_ORIGIN` #49; `HOME_POSITION` #242 | Yes with GPS support | `GPS_GLOBAL_ORIGIN` documented/emitted; no HOME_POSITION in INAV sender | Position group / home available. |
| `ATTITUDE` #30 | Yes | Yes | EXTRA1 group; radians and body angular rates. |
| `HEARTBEAT` #0 | Yes | Yes | EXTRA2 group / separate heartbeat schedule. |
| `VFR_HUD` #74 | Yes | Yes | EXTRA2 group; speed/throttle/altitude/climb. |
| `BATTERY_STATUS` #147 | Yes | Yes | EXTRA3 group. FC battery monitoring, not per-ESC current. |
| `SCALED_PRESSURE` #29 | No sender found in BF MAVLink output module | INAV documents it in EXTRA3 | INAV pressure/barometer temperature when supported; not IMU data. |
| `ESC_INFO` #290 / `ESC_STATUS` #291 | No sender | No sender | Not emitted by the inspected sender tables. |
| `ESC_TELEMETRY_1_TO_4` etc. | No sender; BF bundled sender does not include the ArduPilot ESC dialect path | No sender in INAV MAVLink telemetry implementation | Not emitted. |
| `RPM` #226 | No sender | No sender | No common MAVLink wheel RPM output from these firmware senders. |
| `WHEEL_DISTANCE` #9000 | No sender | No sender | INAV/BF do not provide this via current MAVLink sender. |
| `HIGHRES_IMU` #105 / `SCALED_IMU` #26 etc. | No sender in BF telemetry table | No sender in INAV telemetry table | Use ATTITUDE/other supported paths; no direct MAVLink IMU packet from these sender modules. |

**Source-verified emitted-message inventory:** Betaflight's MAVLink telemetry table contains `SYS_STATUS`, RC channels, GPS raw/global position/origin/home (when GPS compiled), `ATTITUDE`, `HEARTBEAT`, `VFR_HUD`, and `BATTERY_STATUS`; separate scheduler functions also send `SYSTEM_TIME` and responses such as `STATUSTEXT`, `TIMESYNC`/`PING`/command acknowledgments as applicable. INAV 9.1's documented and source-backed groups are `SYS_STATUS`, RC channels, GPS raw/global position/origin, `ATTITUDE`, `VFR_HUD`, `HEARTBEAT`, `BATTERY_STATUS`, `SCALED_PRESSURE`, and pending `STATUSTEXT`, plus `SYSTEM_TIME`. Group defaults documented by INAV 9.1 are EXTENDED_STATUS 2 Hz, RC 1 Hz, POSITION 2 Hz, EXTRA1 3 Hz, EXTRA2 2 Hz, EXTRA3 1 Hz; stream settings can change these.

The important negative is not just “no ESC_STATUS call found”: both current sender paths omit all of `ESC_INFO`, `ESC_STATUS`, `ESC_TELEMETRY_*`, `RPM` and `WHEEL_DISTANCE`. Those common/ArduPilot dialect message definitions being available in generated MAVLink headers does not imply either firmware sends them.

## 4. GPS field differences

All MAVLink GPS altitude values below are millimetres; `eph`/`epv` are unsigned `HDOP/VDOP ×100`; `h_acc`/`v_acc` are millimetres; `yaw` is centidegrees from north. `cog` is course over ground, not vehicle heading.

| Field | Betaflight 2026.6.1 | INAV 9.1 | Practical meaning |
|---|---|---|---|
| `fix_type` | 0 only if no GPS sensor (then sender returns); 1 when GPS present but no `GPS_FIX`; 2 when GPS_FIX but satellite count below Betaflight minimum; otherwise 3 | maps `GPS_NO_FIX`→1, 2D→2, 3D→3; estimated-fix mode can synthesize a 3D fix | INAV may report estimated 3D and synthetic accuracy; consumer must use fix source/quality context. |
| `eph`, `epv` | GPS HDOP/VDOP values in their expected ×100 scale | Directly copies INAV `gpsSol.eph`/`epv`, which its source defines as horizontal/vertical accuracy in **cm**, not DOP | INAV currently puts a centimetre accuracy quantity into fields defined as unitless DOP×100. This is a unit/meaning mismatch. |
| `alt` | `gpsSol.llh.altCm * 10`: MSL altitude in mm | `gpsSol.llh.alt * 10`: MSL altitude in mm | MSL in both. |
| `alt_ellipsoid` | Also `gpsSol.llh.altCm * 10`: it repeats MSL altitude; **not valid ellipsoid altitude** | literal zero | INAV uses zero as unknown/unavailable; BF looks numeric but is mislabeled MSL. Do not consume Betaflight's extension as ellipsoid height. |
| `h_acc`, `v_acc` | GPS solution accuracy values passed through in mm | `eph * 10`, `epv * 10`: INAV centimetre accuracy values converted to mm; estimated-fix mode sets 100 cm | BF supplies receiver-derived accuracy when populated. INAV has accuracy semantics here despite the `eph/epv` mismatch; estimated values are synthetic. |
| GPS `yaw`, `hdg_acc` | yaw=0; hdg_acc=`UINT32_MAX` | yaw=0; hdg_acc=0 | Neither reports GPS heading/yaw through this message. Zero yaw means unavailable by MAVLink convention; INAV's zero heading accuracy is not explicitly marked unknown. |
| `cog` | GPS course over ground from `groundCourse` | GPS course over ground from `groundCourse` | Not compass heading, and unreliable at standstill/low speed. |
| `GLOBAL_POSITION_INT.hdg` | Betaflight attitude yaw | INAV attitude yaw | Vehicle estimator heading, different field/source from GPS course. |

The quad had `GPS: NOT ENABLED`, so none of these GPS fields were runtime-checked here. Betaflight's misleading `alt_ellipsoid` assignment is confirmed directly in its sender source. INAV's zero ellipsoid and eph/epv assignments are confirmed in its `mavlinkSendPosition()` implementation and `gpsSolutionData_t` field definitions.

## 5. MAVROS data path and gaps

### Common `esc_status` plugin

ROS 2 MAVROS subscribes to common `ESC_INFO` and `ESC_STATUS`, publishing `~/info` (`mavros_msgs/ESCInfo`) and `~/status` (`mavros_msgs/ESCStatus`). The MAVLink messages carry groups of four ESCs; their `index` is the zero-based index of the first ESC in that batch, and ESC order is motor order.

- The plugin stores `_max_esc_count` from `ESC_INFO.count`; it does not infer a count from `ESC_STATUS`. If ESC_STATUS arrives before any usable ESC_INFO, `_max_esc_count` remains zero, the status array remains empty, and MAVROS publishes an empty status vector. It does not reject/log the ordering, so send ESC_INFO first and periodically enough to restore the contract after connection reset.
- Each batch writes `esc_status[index + i]` and publishes after reaching the highest batch index seen. This is a zero-based vector mapping. `ESC_INFO.index` maps the same way.
- `ESC_STATUS.rpm` is signed `int32` and the plugin copies it to ROS `int32` without absolute value or scaling; reverse sign is preserved. Voltage (V) and current (A) are floats in the common message and are copied directly to ROS float fields. No unit conversion/drop was found on these three fields.
- `ESC_INFO.temperature` arrives in centi-degrees C, but the plugin sets ROS `temperature = input * 100`. The ROS `ESCInfoItem.temperature` is `int32` and its `.msg` contains no unit or scaling comment. Unless the intended ROS unit is 10,000ths of a degree (undocumented), this is an erroneous extra ×100. This is a real field-level MAVROS concern to resolve before relying on temperature.
- Common `ESC_STATUS` currently has MAVLink `[WIP]` status. The common definition recommends around 10 Hz and splits slower metadata/failures into `ESC_INFO` (about 1 Hz). It carries signed RPM, voltage and current, but no mAh consumption or explicit packet freshness count.

### Legacy `esc_telemetry` plugin

MAVROS' ArduPilot-specific `esc_telemetry` plugin subscribes only to `ESC_TELEMETRY_1_TO_4`, `_5_TO_8`, and `_9_TO_12` (#11030–11032). It publishes `~/telemetry` and maps each four-slot group into vector offsets 0, 4 and 8. It converts voltage cV→V, current cA→A and accumulated mAh→Ah; temperature and RPM are copied directly. It cannot see ESCs 13–32 today.

To support ESC 13–32: add generated ArduPilotMega dialect message types for `ESC_TELEMETRY_13_TO_16` through `_29_TO_32` (#11040–11044; IDs 11033–11037 belong to unrelated messages), register five additional handlers with offsets 12/16/20/24/28, keep size/bounds behavior consistent, and add mapping tests. The existing ROS vector can grow dynamically, so the core shape need not change. This is a meaningful compatibility patch, but does not solve the Betaflight/INAV contract because those firmwares do not currently send the ArduPilot dialect messages.

### RPM and wheel-distance plugins

MAVROS `wheel_odometry` separately handles ArduPilotMega `RPM` #226 and common `WHEEL_DISTANCE` #9000. It publishes raw wheel RPM (`~/rpm`) and distances (`~/distance`), and can compute wheel velocity/odometry using wheel geometry. `WHEEL_DISTANCE` cumulative metres is documented as the more accurate input. These are not automatically derived from ESC_STATUS and are not aliases for it.

Mowgli's current custom ESC-wheel plugin consumes `RPM` #226 for selected left/right channels and uses the `count[]` fields from `ESC_TELEMETRY_*` as a freshness gate; current code subscribes to groups 1–12. It does not subscribe to common ESC_INFO/STATUS or WHEEL_DISTANCE. A migration to the requested common contract needs a Mowgli plugin change: consume per-ESC signed RPM from ESC_STATUS, define a freshness/age policy using timestamps (and optionally ESC_INFO counters), and remove or adapt the legacy ESC_TELEMETRY count gate. ESC_INFO count ordering/resets also need explicit handling.

## 6. ArduPilot and upstream feasibility

ArduPilot's `AP_ESC_Telem` stores per-instance telemetry including RPM as a signed float where available, voltage, current, consumed mAh, temperature and freshness/count metadata. Backends differ: DShot eRPM is unsigned, while other backends can retain signed direction. The current MAVLink path has two separate outputs:

1. `RPM` #226 comes from the `AP_RPM` sensor library and sends two floats when enabled; this is a wheel/RPM sensor path, not a per-ESC array.
2. `AP_ESC_Telem::send_esc_telemetry_mavlink()` sends ArduPilotMega four-ESC batch messages. It casts RPM to an unsigned absolute value, and includes temperature, cV, cA, consumed mAh and an ESC telemetry count. Group messages for IDs 13–32 are compile-time dependent on the configured maximum ESC count.

**Why does ArduPilot not send ESC_INFO/ESC_STATUS?** The inspected source contains no sender or scheduler mapping for common IDs 290/291, and it does not explain the omission. **INFERRED:** the existing path predates or preserves compatibility with BLHeli-oriented ground-station consumers, and carries legacy fields/counters (notably accumulated mAh/count) absent from common ESC_STATUS. There is no source evidence that upstream rejected the common messages on principle.

Adding a standard sender is technically realistic: map the internal per-ESC list into batches of four, send `ESC_INFO` with count, connection type, availability, error count, failures and temperature at low rate, then send `ESC_STATUS` with timestamp, signed RPM, voltage and current at higher rate. Keep legacy messages while downstream consumers migrate. Important design issues for an upstream proposal:

- The common ESC_STATUS definition currently says **WORK IN PROGRESS**; ask maintainers whether its fields/semantics are stable enough for a production sender.
- Select and document when RPM sign is trustworthy. Do not turn unsigned DShot eRPM into fake signed direction. Keep it positive/unknown or pair it with an explicit direction source.
- Set unavailable values to the MAVLink-defined invalid values; do not send zeros that look measured.
- Preserve a freshness or count signal for consumers. ESC_STATUS timestamp is useful but does not encode missed ESC samples or count progression.
- Common ESC_STATUS lacks consumed mAh and packet count; retain legacy telemetry or propose a standard extension if those are required.
- Keep four-ESC batch indexing/count behavior coherent, including partially populated final groups and more than 12 ESCs.

## 7. Does the proposed three-firmware architecture work?

| Stream | Betaflight today | INAV today | ArduPilot today | MAVROS/Mowgli consequence |
|---|---|---|---|---|
| Shared GPS/attitude/battery/status | Partial, source-supported; enable MAVLink port and sensors | Partial, source-supported; INAV MAVLink is explicitly documented as partial | Yes, established MAVLink vehicle backend | MAVROS can bridge the common messages, but fields and semantics still need per-firmware validation; fix GPS altitude/accuracy issues. |
| Standard ESC_INFO + ESC_STATUS | No sender | No sender | No sender found; only legacy ESC telemetry/RPM senders | MAVROS has a receiver for 290/291, but receives nothing today. Betaflight and INAV require firmware work; ArduPilot requires a new sender. |
| Current Mowgli wheel odometry | No compatible messages sent | No compatible messages sent | Uses RPM #226 + ESC_TELEMETRY counts in current plugin | Current implementation is ArduPilot-specific. Extend it before calling the path firmware-neutral. |
| MSP role | Rich Betaflight diagnostics/configuration/RPM access | Rich INAV control/configuration path | Not the common path | Keep as diagnostic/configuration fallback; it need not be the primary Mowgli telemetry architecture. |

**Recommendation:** proceed with MAVLink as the preferred interface boundary, but treat it as a staged firmware feature, not a config-only shortcut. First correct/validate common GPS semantics and confirm MAVROS publishes the required fields. Next add a small Mowgli receiver for common ESC_INFO/STATUS and unit tests for indexing, negative RPM, missing INFO, stale timestamps and missing ESC values. Then prototype ESC_INFO/STATUS output in ArduPilot while keeping its legacy output. Once that contract is accepted, implement the equivalent sender paths in Betaflight and INAV. If an immediate Betaflight proof-of-concept is needed before upstream work, a narrow MSP diagnostic bridge is lower-risk and already validated on this FC, but it should be clearly separated from the desired common runtime contract.

## 8. Tests, restoration and remaining unknowns

- **Completed:** pre-test `version`, `status`, tasks, serial, resources, features, map/timer/DMA, motor/DShot/telemetry/RPM/ESC/debug, `diff all` and `dump all` snapshots were captured locally. Full config dumps are not included in this public evidence bundle.
- **Completed:** 15-second idle MSP sampling (518/518 requests valid) and short low-output per-motor/all-motor sweep.
- **Completed:** post-test idle sample; all motor outputs neutral and RPM returned to zero.
- **Completed:** the original motor-sweep before/after `diff all`, `dump all`, serial, feature, motor, DShot, telemetry, RPM, ESC and debug snapshots match after CRLF normalization. In the final EDT probe, the only changed setting was restored to `dshot_edt=OFF` and verified after reboot. No ESC configuration or firmware was changed. See the included `restoration.json` and checksums.
- Betaflight native USB de-enumerates while leaving CLI on this target. The CLI exit acknowledgment was not captured; all command outputs, including the full after dump, were saved before disconnection, and MSP reconnect/idle polling worked afterward. The snapshot script now preserves its transcript if this occurs again.
- **Not done:** no runtime MAVLink packet capture; no GPS runtime check (GPS disabled); no ESC firmware identification; no standalone tachometer reference; no mower traction/load/reverse test. The odometry relation from no-load prop RPM to wheel ground speed remains unvalidated and should not be inferred from this quad bench test.
- The last FC core temperature read was 92 °C after the short bench test; allow it to cool before another powered session. ESC temperature remained unavailable. Treat the MCU sensor value as an FC reading, not proof of ESC overheating.

## 9. Evidence files

- [`SOURCE_NOTES.md`](SOURCE_NOTES.md) — pinned source files, functions, message IDs and upstream links.
- [`TEST_MATRIX.md`](TEST_MATRIX.md) — completed tests and evidence strength.
- Full CLI config snapshots are retained locally but omitted from this public bundle because they contain unrelated aircraft profile settings.
- Selected runtime evidence is included under `captures/`; the incomplete/intermediate final CLI attempts and host-specific USB identifiers are excluded. No MAVLink packet capture exists because the tested FC exposed MSP, not MAVLink, on the USB port.
- `restoration.json` and `raw/evidence-sha256.txt` record the final setting and motor-output restoration proof.
