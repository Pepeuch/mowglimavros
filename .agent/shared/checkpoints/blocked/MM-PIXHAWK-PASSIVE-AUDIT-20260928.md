# MM-PIXHAWK-PASSIVE-AUDIT-20260928

Disposition: BLOCKED — MM-801 active hardware acceptance remains pending.
Repository `/workspaces/mowglimavros`, branch `main`, audited HEAD `781946d7ba8a595d7a472781763ffac6800545da`. Date: 2026-09-28 UTC.
Current wheel mapping correction: see **Superseding wheel telemetry observation** below; the older right-wheel → ESC2/index 1 rows are historical and do not describe the later capture.

## Method and safety

Baseline: `.agent/shared/checkpoints/blocked/MM-PIXHAWK-VALIDATION.md`, the 2026-09-08 capability audit, `mowglimavros_hardware_audit_2026-09-22.md`, README audit notes, and MM-801 in TODO. Existing dirty README and untracked audits were untouched. This is a snapshot of one Rock 5B/Pixhawk configuration, neither a final profile nor active acceptance.

SSH with the existing dedicated key to `pepeuch@192.168.10.32`; Docker/USB/log/ROS graph reads; bounded passive subscriptions; one read-only MAVROS `ParamPull(force_pull=true)` / MAVLink `PARAM_REQUEST_LIST`. It returned `success=true`, 1,014 parameters. Dumps before/after differed only in `STAT_RUNTIME`; fresh local temporary YAML SHA-256: `a0e97186822ddec3fe517a92241cd0601694be6d799c8f0b166c65d30d5dc9f3`. No ARM, motor/blade command, mode change, parameter write/upload, firmware flash, reset/reboot, RTCM injection, or induced disconnection. No persistent FCU change.

## Follow-up POWER1 — 2026-09-28 ~13:05 UTC (later than the snapshot below)

After the operator restored the POWER1 controller, a 12-second passive subscription received 18 valid `mavros_battery_observer/BatteryStatus` samples for **id0** (`BATT_` / POWER1 by the previously established mapping). Latest: **24.812 V, 1.51 A, 96%**; range **24.777–24.812 V**, **1.51–1.52 A**. ID1 also remained valid: latest **24.295 V, 0.38 A** (range 0.38–0.45 A). MAVROS diagnostics changed to `mavros: Battery = Normal`, 24.81 V / -1.5 A / 96%, and `mavros: System` reports Battery=Ok and pre-arm=Ok. This confirms that the POWER1 monitoring channel now reports measurements; the current is **line current measured by the monitor**, not an isolated measurement of the controller's own consumption. The operator subsequently confirmed that the dock charger was connected and its charge current was being adjusted during this observation. The 1.51–1.52 A is therefore a time-stamped POWER1 monitor reading with the charger connected; the relationship to net traction-battery charge current at POWER2 and the sign convention remain unvalidated.

The FCU stayed `connected=true`, `armed=false`; its observed mode was `MANUAL` (previous snapshot `HOLD`), with no mode command from this audit. Bridge `dock_power` and `traction_power` diagnostics still report absent/missing because both bridge battery instance parameters remain `-1`; these diagnostics do not contradict the valid MAVROS id0/id1 observations. No parameter pull or write, RTCM, arm, or actuator command occurred in this follow-up. The earlier id0-unavailable/Battery-1-unhealthy rows below remain dated evidence, **superseded for current POWER1 availability by this follow-up**. Other MM-801 active-test gates remain open.

## Runtime — PROUVÉ

| Surface | Evidence |
| --- | --- |
| Host/transport | `rock-5b`, aarch64, Armbian 26.8.3/Debian 13, kernel `7.1.8-edge-rockchip64`; Pixhawk5X ArduRover 4.6.3 (`3fc7011a`). USB by-id `usb-Holybro_Pixhawk5X_2F002F001050425937353320-if00 -> ttyACM0`, `if02 -> ttyACM1`; MAVROS uses if00 at 921600. |
| Docker | `mowgli-mavros` running, restart count 0, host network, image `ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar:build-kilted-arm64`, ID `sha256:eda684a85e266dc896403daf0b41bc2b6828233eb2092b103499abdd1a4ba006`; also `mowgli-gps`, GUI, lidar. No running legacy `mowgli-ros2`; one `/hardware_bridge`. Image labels do not encode source revision. |
| FCU/heartbeat | `/mavros/state`: 15 messages/15 s, `connected=true`, `armed=false`, `mode=HOLD`, `system_status=3`. FCU HEARTBEAT sysid:compid 1:1: 8/8 s; separate 255:190 heartbeat also observed. MAVROS heartbeat diagnostic normal at about 1 Hz. Serial router diagnostic: zero drops/overruns/parse errors in its observed counters. UDP GCS broadcast endpoint 14550 active; PC reception is operator-reported, not independently checked from PC. |
| Reconnect | Host kernel journal shows Pixhawk bootloader/USB transitions on 2026-09-24 ending with normal Pixhawk and both ACM interfaces. No reconnect was induced or observed during this session; recovery after future USB loss/FCU reboot remains unproven. |
| ROS graph | 164 topics, 543 services (mostly standard parameter services). `/mavros/param/pull` used once; advertised command/mode/parameter-set services were **not called**. Presence of a ROS surface is not evidence of delivered data. |

15-second bounded sample; counts are observations, not guaranteed rates:

| Surface | Observation |
| --- | --- |
| IMU | `/mavros/imu/data_raw`: 30; `/mavros/imu/data`: 151; near-zero gyro and about 9.81 m/s² acceleration at rest. |
| ESC | `/mavros/esc_telemetry/telemetry`: 45; slots 0/1/2 populated around 23.5 V, 35–36 °C, RPM 0, advancing count; slot 3 zero. MAVLink `ESC_TELEMETRY_1_TO_4` 44. `/mavros/esc_status/status` and `/info` advertised but silent; no `ESC_STATUS` or `ESC_INFO` observed. |
| RPM | MAVLink `RPM` #226: 44; every sampled `rpm1/rpm2` was `-1/-1`. Populated ESC telemetry does not make these fields usable. |
| Battery | MAVLink `BATTERY_STATUS` #147: 44. Observer IDs 0/1 publish: ID0 voltage NaN/unavailable, current 0 A, percentage unavailable; ID1 about 23.76 V, 0.63 A, 93%. `/mavros/battery`, `/mavros/sys_status`, `/battery_state`, `/hardware_bridge/power` advertised but silent in this window. FCU logs repeatedly say `PreArm: Battery 1 unhealthy`. |
| GPS | GPS1 and GPS2 raw: 30 each, `fix_type=0`, 0 satellites, invalid position. No samples from `/mavros/universal_gnss/gps{1,2}/status`, `/gps/fix`, `/gps/status`. `/gps/fix` has only the separate Universal GNSS receiver as graph publisher; `/gps/status` has none. Universal GNSS reports serial transport failure. NTRIP receives RTCM but receiver correction writes are unavailable. `/mavros/gps_rtk/send_rtcm` has no publisher; this audit injected none. |
| Backend | `/hardware_bridge/status`: 150, but stamp zero/default fields are not physical feedback. `/wheel_odom`: no publisher or sample, bridge subscriber present. Readiness diagnostic: FCU connected; GNSS invalid, wheel odometry missing, traction power invalid, dock power absent/stale (nonblocking), `backend_readiness=not_ready`. MAVROS system diagnostic reports AHRS, battery and pre-arm health failures. |

## Relevant ROS interface inventory

This is the discovered **running** graph. Counts are from the same 15-second sample; `0` means advertised but no delivered sample, not a proven absence of FCU capability.

| Topic(s) | Type | Publisher / 15-second delivery |
| --- | --- | --- |
| `/mavros/state` | `mavros_msgs/msg/State` | `/mavros/sys`; 15 |
| `/mavros/imu/data_raw`, `/mavros/imu/data` | `sensor_msgs/msg/Imu` | `/mavros/imu`; 30, 151 |
| `/mavros/battery_observer/status` | `mavros_battery_observer/msg/BatteryStatus` | `/mavros/battery_observer`; both IDs, 45 total |
| `/mavros/battery`, `/battery_state` | `sensor_msgs/msg/BatteryState` | `/mavros/sys`, `/hardware_bridge`; 0, 0 |
| `/mavros/esc_telemetry/telemetry` | `mavros_msgs/msg/ESCTelemetry` | `/mavros/esc_telemetry`; 45 |
| `/mavros/esc_status/status`, `/info` | `mavros_msgs/msg/ESCStatus`, `ESCInfo` | `/mavros/esc_status`; 0, 0 |
| `/mavros/gpsstatus/gps1/raw`, `/gps2/raw` | `mavros_msgs/msg/GPSRAW` | `/mavros/gpsstatus`; 30, 30 |
| `/mavros/universal_gnss/gps{1,2}/status` | `universal_gnss_ros2/msg/GnssStatus` | plugin graph surface; 0, 0 |
| `/gps/fix`, `/gps/status` | `sensor_msgs/msg/NavSatFix`, `mowgli_interfaces/msg/GnssStatus` | Universal GNSS receiver publisher on fix, no publisher on status; 0, 0 |
| `/hardware_bridge/status`, `/power` | `mowgli_interfaces/msg/Status`, `Power` | sole `/hardware_bridge`; 150, 0 |
| `/wheel_odom` | `nav_msgs/msg/Odometry` | no publisher; 0 |
| `/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | multiple producers; 426 arrays; backend readiness present |

Relevant service inventory: `/mavros/param/pull` (`ParamPull`) is read-only and was the only MAVROS service called. `/mavros/vehicle_info_get` is advertised for identity reads. `/mavros/param/set`, `/mavros/cmd/arming`, `/mavros/cmd/command`, `/mavros/set_mode`, `/hardware_bridge/emergency_stop`, and `/hardware_bridge/mower_control` are advertised but were not called. Their existence does not validate their effects.

## Changes and contradictions against retained audits

- `RPM1_ESC_MASK`/`RPM2_ESC_MASK` were `0/0` in the 2026-09-22 audit and are now `1/2`. MAVLink RPM remains `-1/-1`; `RPMx_TYPE=7` makes the old “zero masks explain missing RPM” hypothesis insufficient.
- `SERIAL4_BAUD` was 460800 in the earlier MAVROS audit, 921600 in the later 2026-09-22 QGC note, and is 460800 after the fresh 2026-09-28 MAVLink pull. `GPS2_TYPE` moved from AUTO (1) to uBlox (2). Neither change is attributed to this audit; modification time and reason remain unknown.
- Historical `BATTERY_STATUS id0` had valid voltage; it is now unavailable/NaN while ID1 remains valid. The historical `MANUAL` mode is now `HOLD`, without an audit-induced mode change. Previous double `/hardware_bridge` ownership is absent from the current running stack.
- Earlier receiver audit saw GPS2 connected without fix (`fix_type=1`); both GPS slots now report `fix_type=0`. Current data do not identify the receiver or establish whether the fault is power, serial settings, wiring, receiver configuration, or another cause.

## FCU parameters after fresh pull — CONFIGURÉ MAIS NON VALIDÉ

Serial baud values are ArduPilot units: `460` means 460800, `921` means 921600.

| Group | Observed values / interpretation |
| --- | --- |
| Servo/skid steer | `FRAME_CLASS=1`, `FRAME_TYPE=0`, `PILOT_STEER_TYPE=0`; `SERVO1_FUNCTION=73` (Throttle Left), `SERVO2_FUNCTION=35` (Motor3), `SERVO3_FUNCTION=74` (Throttle Right). Outputs 1–3 each `MIN=1000 MAX=2000 TRIM=1500 REVERSED=0`; outputs 4–16 each `FUNCTION=0 MIN=1100 MAX=1900 TRIM=1500 REVERSED=0`. `MOT_STR_THR_MIX=0.5`, `MOT_THR_MIN=0`, `MOT_THR_MAX=100`, `MOT_SLEWRATE=100`, `MOT_PWM_TYPE=0`, `MOT_SAFE_DISARM=0`, `CRUISE_SPEED=0.5`, `CRUISE_THROTTLE=80`, `MODE_CH=8`. ArduPilot documents 73/74 as skid steering outputs. Motor3 physical purpose is unproven. |
| CAN/ESC | CAN1 `CAN_P1_DRIVER=1`, `CAN_P1_BITRATE=500000`, `CAN_D1_PROTOCOL=1` DroneCAN, `CAN_D1_PROTOCOL2=1`, `CAN_D1_UC_NODE=10`, `CAN_D1_UC_ESC_BM=7`, `CAN_D1_UC_ESC_OF=0`, `CAN_D1_UC_ESC_RV=7`, `CAN_D1_UC_OPTION=1924`, `CAN_D1_UC_SRV_BM=0`; CAN2 `CAN_P2_DRIVER=2`, `BITRATE=1000000`, `CAN_D2_PROTOCOL=1`, ESC bitmap/reverse bitmap 0. `ESC_TLM_MAV_OFS=0`. Historical bus inspection proved three operational VESC nodes 1/2/3 with `esc_index` 0/1/2; this session reconfirmed three MAVLink ESC slots, not node identity. |
| RPM | `RPM1_TYPE=RPM2_TYPE=7`, `RPM1_ESC_MASK=1`, `RPM2_ESC_MASK=2`, `RPM1_DC_ID=RPM2_DC_ID=-1`, both `SCALING=1`, `ESC_INDEX=0`, `PIN=-1`, `MIN=10`, `MAX=100000`, `MIN_QUAL=0.5`. ArduPilot's [4.6 RPM parameter reference](https://ardupilot.org/rover/docs/parameters-Rover-beta-V4.6.0.html#rpm1-parameters) labels type 5 “ESC Telemetry Motors Bitmask” and type 7 “DroneCAN”. Thus masks 1/2 do not establish a selected ESC telemetry RPM source with type 7. This is a probable explanation of `-1/-1`; confirm against exact 4.6.3 implementation before changing anything. |
| Batteries/charge | `BATT_MONITOR=BATT2_MONITOR=21` INA2XX, I2C buses 1/2, address 69, shunt 0.0005, capacity 3000 mAh, max 60 A, options 34, serial -1. Historical physical mapping: ID0=`BATT_`=`POWER1` dock; ID1=`BATT2_`=`POWER2` traction 7S. `BATT_ARM_VOLT=0`, `BATT_LOW_VOLT=BATT_CRT_VOLT=0`, low/critical capacity thresholds 0, low/critical actions 0. Both monitors have `FS_VOLTSRC=0` and `ARM_MAH=0`. `BATT2_ARM_VOLT=21.8`, `BATT2_CRT_VOLT=22.5`, `BATT2_LOW_VOLT=0`, `BATT2_FS_LOW_ACT=1` (RTL if triggered), `BATT2_FS_CRT_ACT=3` (SmartRTL), low timer 0. Current sign, capacity, thresholds and charge behavior lack physical validation. Explain Battery 1 unhealthy before active tests. |
| GPS/serial | `SERIAL0_PROTOCOL=2 BAUD=921` USB primary; `SERIAL8_PROTOCOL=2 BAUD=921` USB secondary; `SERIAL3_PROTOCOL=5 BAUD=460` GPS1; `SERIAL4_PROTOCOL=5 BAUD=460` GPS2; `SERIAL1_PROTOCOL=11 BAUD=230`, `SERIAL2_PROTOCOL=-1 BAUD=57`, `SERIAL5_PROTOCOL=22 BAUD=115`, `SERIAL6_PROTOCOL=9 BAUD=115`, `SERIAL7_PROTOCOL=9 BAUD=115`, `SERIAL_PASS1=0`, `SERIAL_PASS2=-1`, `SERIAL_PASSTIMO=0`; no SERIAL9. `GPS1_TYPE=9` DroneCAN, `GPS1_CAN_OVRIDE=124`, `GPS1_CAN_NODEID=0`; `GPS2_TYPE=2` uBlox, both `GPSx_RATE_MS=125`, `GPS1_DELAY_MS=20`, `GPS2_DELAY_MS=0`; `GPS_PRIMARY=1`, `GPS_AUTO_SWITCH=4`, `GPS_AUTO_CONFIG=3`, `GPS_INJECT_TO=127`. F9P on GPS2 and former HERE4 on CAN1 are wiring/history statements, not current telemetry identity. The 2026-09-22 QGC note had `SERIAL4_BAUD=921600`; FCU is now back to 460800. `GPS2_TYPE` also changed from older AUTO (1) to uBlox (2). Provenance unknown. |
| AHRS/EK3 | `AHRS_EKF_TYPE=3`, `AHRS_GPS_USE=1`, `EK3_ENABLE=1`, `EK3_PRIMARY=0`, `EK3_IMU_MASK=3`, `EK3_SRC1_POSXY=3 POSZ=1 VELXY=3 VELZ=3 YAW=1`, `EK3_GPS_CHECK=31`. IMU transport works, but AHRS health fails and no usable GPS is present. |
| Arming/failsafe | `ARMING_CHECK=2432` (bits 7/8/11: board voltage, battery level, hardware safety switch; not all checks), `ARMING_REQUIRE=1`, `ARMING_RUDDER=2` (RC arm/disarm allowed), `BRD_SAFETY_DEFLT=0`, `BRD_SAFETYOPTION=0`, `BRD_SAFETY_MASK=0`; `FS_ACTION=0` (nothing), `FS_GCS_ENABLE=0`, `FS_THR_ENABLE=0`, `FS_CRASH_CHECK=0`, `FS_EKF_ACTION=2` (report only), `FS_TIMEOUT=1.5`. Review stop authority before actuation. [Rover 4.6 parameter reference](https://ardupilot.org/rover/docs/parameters-Rover-beta-V4.6.0.html). |

## Superseding wheel telemetry observation, 2026-09-28

After this passive audit, a new operator-confirmed isolated **right-wheel-only** manual rotation was captured for 20 s through `/mavros/esc_telemetry/telemetry`: slot 0 had 26 of 60 samples with nonzero RPM (maximum 1,038 RPM); slots 1 and 2 stayed 0 RPM; all currents were 0 A. CAN Status captured in the same runtime maps ROS slot 0 / `esc_index=0` to node 1. Therefore the **current right wheel → ESC1 / CAN node 1 / `esc_index=0`**. This supersedes the 2026-09-22 historical observation right wheel → ESC2/index 1 for the current physical setup. The reason for the discrepancy is unknown, and the historical result remains recorded for provenance. Do not use the historical right-wheel row below as today's command or telemetry map.

A separate operator-confirmed left-wheel-only hand-rotation of comparable speed, with a 5 s countdown and 20 s recording, produced 60 samples per ROS ESC slot, all 0 RPM/0 A. The left-wheel telemetry index remains unresolved; absence of detected RPM is not proof of physical or VESC identity. Neither wheel exercise sent a motor or CAN command or enabled forwarding. Two separately authorized 10 s MANUAL_CONTROL trials at 5% and 20% produced no visible commanded motor motion; the clean 20% trial had zero RPM/0 A on all three Status sources. See `MM-MANUAL-CONTROL-ROUTING-20260928` for bounded command and stop evidence.

## Three distinct VESC relations

Historical manual wheel observation proved **right wheel → MAVLink ESC2 → `esc_index=1` → ROS `esc_telemetry[1]`**. Telemetry identity is independent of physical command routing. ArduPilot documents `CAN_D1_UC_ESC_BM` as selecting servo/motor output channels and `CAN_D1_UC_ESC_OF` as RawCommand slot offset ([DroneCAN setup](https://software.ardupilot.org/copter/docs/common-uavcan-setup-advanced.html), [parameter reference](https://autotest.ardupilot.org/Parameters/Plane/Parameters.html)). With bitmap 7 and offset 0, the **configured, not bus-measured** relationship is:

| Physical function | ArduRover function → output | Configured RawCommand slot | MAVLink telemetry identity |
| --- | --- | --- | --- |
| Left wheel | Throttle Left 73 → SERVO1 | index 0 | ESC1/index 0 **or** ESC3/index 2; unresolved |
| Right wheel | Throttle Right 74 → SERVO3 | index 2 | **ESC2/index 1 proven** |
| Mower motor | Physical association with Motor3 unproved; Motor3 35 → SERVO2 | index 1 | ESC1/index 0 **or** ESC3/index 2; unresolved |

If each VESC uses the same `esc_index` for RawCommand subscription and status publication, the right-wheel command/telemetry indices disagree (2 vs 1). That premise was not measured here. Treat as a possible actuator-routing hazard, not proof that the right wheel receives Motor3 commands. Read VESC configuration and passively inspect CAN RawCommand before any motor test. Never infer `SERVO1 == ESC1`.

## Status and gates

| Status | Result |
| --- | --- |
| **PROUVÉ** | Rock SSH/if00, FCU heartbeat/disarmed HOLD, sole MAVROS bridge, live IMU, three ESC slots, two BATTERY_STATUS IDs, no valid GPS, freshly read 1,014 parameters, RPM -1/-1, backend not ready. Historical right-wheel telemetry mapping retained unless contradicted by new physical evidence. |
| **CONFIGURÉ MAIS NON VALIDÉ** | Skid-steer servo output and CAN RawCommand settings, battery thresholds and charge semantics, GPS2 serial/uBlox, EK3 source, emergency HOLD/disarm, VESC command routing, transport recovery, actual safety behavior. |
| **À CONFIGURER** | After diagnosis and separate write authorization: suitable wheel RPM source/type and masks; bridge dock/traction IDs; odometry wheel slots/RPM fields/measured geometry; valid public GNSS source; reviewed arming, battery and failsafe policy. No final `.param` profile is defined here. |
| **À TESTER PHYSIQUEMENT** | VESC command/status identity; left/mower mapping, signs and gear ratio; POWER1 dock absence/presence and POWER2 measurement; F9P UART and HERE4/CAN coexistence; stop/feedback; USB loss/reconnect and FCU reboot; outdoor GNSS/RTK. |
| **BLOQUANT AVANT TEST ACTIF** | Possible right-wheel command-slot mismatch; left/mower identity unresolved; RPM unusable; Battery 1 unhealthy and AHRS/pre-arm failures; invalid GNSS and readiness false; bridge command/blade disabled; current failsafe/arming settings lack demonstrated stop authority. |

## Ordered next tests — do not execute in this phase

1. With actuator power physically isolated, read each VESC command `uavcan_esc_index` and status index; passively inspect CAN1 RawCommand/status to reconcile the right-wheel mismatch.
2. While disarmed, observe manual left-wheel rotation for ESC1 vs ESC3, then separately identify the mower without energizing its blade. Record telemetry count and signed RPM; do not assign Motor3 by elimination alone.
3. Diagnose type-7 vs type-5 RPM path against exact Rover 4.6.3 code and source data. After identities are proven, define wheel-only masks and measure wheel radii, track width, and motor/wheel ratio before enabling `/wheel_odom`.
4. Trace POWER1 with dock absent/present and POWER2 against instruments. Explain Battery 1 unhealthy, current sign, capacity and failsafe thresholds; then approve bridge ID0 dock/ID1 traction mapping.
5. Passively check F9P GPS2 power/TX/RX/baud against `SERIAL4_BAUD=460`, and HERE4 node 124 on CAN1. Recover valid GPS and `/gps/status` before any RTCM test.
6. Review arming/failsafe and independent stop authority for RC/GCS/USB/DDS loss, EKF, HOLD, disarm and blade isolation. Do not relax checks merely to clear pre-arm errors.
7. Only in a separately authorized and physically secured active session: bounded one-at-a-time actuator tests, stop/HOLD/disarm/failsafe, power/charging, reconnect/reboot, outdoor GNSS/RTK and finally blade. Keep MM-801/HW-MAV-001..005 open until evidence exists.

Invalidation: firmware, CAN/VESC index, wiring, battery monitor, receiver, image, launch/config, ROS graph, or FCU parameter changes invalidate affected rows. No commit or push.

## Follow-up CAN1 forwarding and passive decode — 2026-09-28 ~13:43 UTC

Hardware/firmware/host/backend are the same as the snapshot above. After checking `ros2 service type /mavros/cmd/command` (`mavros_msgs/srv/CommandLong`) and `ros2 interface show mavros_msgs/srv/CommandLong`, the operator-authorized `MAV_CMD_CAN_FORWARD=32000` was sent to the FCU with `param1=1` on the existing MAVROS link. The FCU was confirmed disarmed in MANUAL first. It was refreshed during a 22.000 s window (nine accepted `COMMAND_ACK result=0` activations/refreshes) and **explicitly disabled** with `param1=0` (`success=true`, `result=0`). A later independent 7 s passive check saw eight FCU heartbeats, zero `CAN_FRAME`/`CANFD_FRAME`, and `/mavros/state=(connected=true, armed=false, MANUAL)`. No FCU parameter, mode, motor/blade command, CAN frame injection, or persistent setting was changed.

The window delivered 33,404 FCU MAVLink `CAN_FRAME` (#386) messages, all bus 0 (CAN1 in this forwarding session), and zero `CANFD_FRAME` (#387). DroneCAN reassembly found 3,254 complete `uavcan.equipment.esc.Status` (type 1034) transfers with valid signature-seeded CRC-16 and no sequence/length errors. The DSDL bit order is **most-significant bit first within bytes**; applying it to the `uint5 esc_index` yields 0/1/2, consistent with the 2026-09-22 CAN audit and the current MAVROS ESC telemetry slot voltages. Each row is the range across the 22 s capture; temperatures are DroneCAN Kelvin converted to Celsius.

| CAN node/source | `Status.esc_index` → MAVLink telemetry | Physical function known | RawCommand slot observed | `Status` RPM / current / voltage / temperature |
| --- | --- | --- | --- | --- |
| 1 | 0 → ESC1 / ROS slot 0 (1,081 status transfers) | Left wheel **or** mower; not identified | None observed | 0 RPM; 0 A; 25.281–25.344 V; 34.85–35.10 °C |
| 2 | 1 → ESC2 / ROS slot 1 (1,092) | **Right wheel**, from historical manual-rotation proof; not moved in this session | None observed | 0 RPM; 0 A; 25.141–25.203 V; 34.60–34.85 °C |
| 3 | 2 → ESC3 / ROS slot 2 (1,081) | Left wheel **or** mower; not identified | None observed | 0 RPM; 0 A; 25.297–25.391 V; 35.35–35.85 °C |

All three Status sources reported `error_count=0`. A separate current `/mavros/esc_telemetry/telemetry` sample had slot voltages 25.35/25.21/25.40 V and slot temperatures 34/34/35 °C, matching node order within telemetry resolution. Zero `uavcan.equipment.esc.RawCommand` (type 1030) and zero `RPMCommand` (1031) frames were observed in the disarmed window; **no command slot or value was bus-observed**. The configured `SERVO1/2/3` → RawCommand slots 0/1/2 remain only parameter/source interpretation. In particular, right-wheel `Status.esc_index=1` versus configured right-throttle `SERVO3`/slot 2 remains a possible routing mismatch; absence of disarmed RawCommand cannot prove VESC command subscription or physical actuation. The next safe step is reading VESC `uavcan_esc_index`/command mapping with actuator power isolated, then a separately authorized bounded physical test. No MM-801 active acceptance item closes from this capture.

Decoder basis: [DroneCAN standard ESC DSDL](https://dronecan.github.io/Specification/7._List_of_standard_data_types/), [DSDL byte/bit order](https://dronecan.github.io/Specification/3._Data_structure_description_language/), [CAN transport/tail/CRC framing](https://dronecan.github.io/Specification/4.1_CAN_bus_transport_layer/). Local raw JSONL and capture script are transient investigation artifacts, not a model parameter profile.
