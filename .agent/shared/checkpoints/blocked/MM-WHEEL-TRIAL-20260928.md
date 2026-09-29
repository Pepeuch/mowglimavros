# MM-WHEEL-TRIAL-20260928 — unique authorized wheel trial

Disposition: BLOCKED — FCU refused arming before propulsion. Repository `/workspaces/mowglimavros`, branch `main`, HEAD `781946d7ba8a595d7a472781763ffac6800545da`. Preflight recorded **before arming or nonzero command**. This checkpoint is evidence, not authorization for a repeat trial.

## Pretrial baseline

- Hardware: Pixhawk 5X on USB `if00` to Rock 5B `192.168.10.32`; `mowgli-mavros`, ROS 2 Kilted overlay `/ros2_ws/install/setup.bash`. Operator reports stable stand, wheels free, mower blade removed. Trial is limited to one MANUAL wheel command at `y=0,z=200` for at most 10 s; no CAN/FCU parameter change or mower control.
- Firmware **live verified** via `/mavros/vehicle_info_get`: FCU sysid:compid 1:1, `flight_sw_version=67568127` = Rover **4.7.1**. `/mavros/state` before trial: `connected=true`, `armed=false`, `mode=MANUAL`, `manual_input=true`, `system_status=4`. MAVROS `system_id=255`, `target_system_id=1`; Rover 4.7.1 cached `MAV_GCS_SYSID=255`, `MAV_GCS_SYSID_HI=0`, `MAV_SYSID=1`. The older 4.6.3 `SYSID_MYGCS` name is not present in this parameter cache.
- CAN live cached parameters: `CAN_P1_DRIVER=1`, `CAN_P1_BITRATE=500000`, `CAN_D1_PROTOCOL=1`, `CAN_D1_UC_NODE=10`, `CAN_D1_UC_ESC_BM=7`, `CAN_D1_UC_ESC_OF=0`, `CAN_D1_UC_ESC_RV=7`; CAN2 `CAN_P2_DRIVER=2`, `CAN_P2_BITRATE=1000000`, `CAN_D2_PROTOCOL=1`; `ESC_TLM_MAV_OFS=0`.
- Physical VESC telemetry mapping, operator-confirmed: slot/index 0, CAN node 1 = right wheel; 1, node 2 = left wheel; 2, node 3 = mower. Live cached `SERVO1_FUNCTION=74`, `SERVO2_FUNCTION=73`, `SERVO3_FUNCTION=35`. This **supersedes** the older 4.6.3 checkpoint's servo order 73/35/74. The status-index mapping must not be assumed to prove command routing.
- Physical power mapping, previously established in `mowglimavros_hardware_audit_2026-09-22.md` and `MM-PIXHAWK-PASSIVE-AUDIT-20260928`: POWER1 = dock/charger, `BATTERY_STATUS id0`/`BATT_`; POWER2 = traction and whole-robot draw, id1/`BATT2_`. Charge current belongs to POWER1; robot consumption to POWER2. Current sign and calibration still need validation. An earlier separate POWER1 sample was 31.819 V, 0.13 A, 85%; use the simultaneous per-ID values in the result table for this attempt.
- ROS command path: `/mavros/cmd/arming` type `mavros_msgs/srv/CommandBool`; `/mavros/manual_control/send` type `mavros_msgs/msg/ManualControl`, Rover steering `y`, throttle `z`. Bridge `/cmd_vel` remains disabled. Baseline `/mavros/rc/out`: channels 1/2 = 1500/1500, channel 3 = 0 (SERVO3 Motor3); abort if channel 3 becomes non-neutral. `/mavros/esc_telemetry/telemetry` reliably publishes slots 0/1/2 at ~1 Hz; slot 3 is empty; `/mavros/esc_status/status` is silent.
- Known anomalies: older checkpoint describes failed 4.6.3 MANUAL trials with no visible wheel response and an outdated servo order; it does not establish current 4.7.1 response. Earlier left-wheel manual rotation lacked visible ESC RPM. `RPM` #226 had been -1/-1. Bridge wheel odometry, individual VESC command index, physical direction, POWER current sign, stop/failsafe authority remain unproven. Abort on FCU loss, mode change, disarm, serious warning/failsafe, mower slot RPM/current rise, mower SERVO3 output, or other abnormal motor behavior observed by operator.

## Execution/result — 2026-09-28

The one authorized sequence attempted ARM once. Its preflight captured all required sources. /mavros/cmd/arming responded success=false, MAVLink result 4 (FAILED). FCU emitted "Arm: Compass 2 not found" and "Arm: DroneCAN: Node 124 unhealthy!" with severity 2. The guarded script aborted before positive propulsion: positive_sent=0, no z=200 phase, no physical motor test. It sent 43 MANUAL_CONTROL neutral y=0,z=0 messages over 2.210 s. No disarm request was needed because FCU never armed. Final /mavros/state and separate postcheck: connected=true, armed=false, mode=MANUAL.

| Signal | Baseline and neutral-only capture |
| --- | --- |
| RC out | Ch1/ch2 1500/1500; ch3 (Motor3) 0; unchanged. No commanded wheel or mower output observed. |
| ESC0 right | RPM 0; current 0–0.02 A; voltage 28.50–28.51 V; temp 40 °C; count 32362→32655. |
| ESC1 left | RPM 0; current 0–0.01 A; voltage 28.29–28.32 V; temp 40 °C; count 34120→34416. |
| ESC2 mower | RPM 0; current 0 A; voltage 28.50–28.51 V; temp 35 °C; count 32366→32659. |
| POWER1 dock/charger, id0 | 1.48–1.49 A, 29.272–29.283 V, observed separately. |
| POWER2 traction/robot, id1 | 0.39–0.41 A, 28.784–28.788 V, observed separately. |
| /mavros/manual_control/control | No sample in this window. Direct sent-neutral events are in the local JSONL trace. |

The POWER values are neutral/disarmed measurements, not motor draw. No wheel direction or propulsion response can be concluded. Recent MAVROS logs also contained "GP: No GPS fix" and "TM: Wrong FCU time", but the FCU explicit ARM refusal named the missing compass and unhealthy DroneCAN node. No parameter, CAN configuration, container, or firmware change occurred. Local timestamped capture: .agent/checkpoints/wheel_trial_capture_20260928.jsonl (ignored by Git).

Blocked by: FCU ARM refusal "Compass 2 not found" and "DroneCAN: Node 124 unhealthy!".
Unblocks when: those FCU pre-arm conditions are diagnosed and corrected with separately authorized work, then a new bounded wheel-only trial is explicitly authorized. Do not bypass arming checks or automatically retry.
Invalidation: FCU firmware/parameters, CAN device health, VESC/servo/power mapping, or container source identity changes.
