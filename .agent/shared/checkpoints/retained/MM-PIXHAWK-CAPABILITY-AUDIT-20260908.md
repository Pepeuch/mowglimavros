# MM-PIXHAWK-CAPABILITY-AUDIT-20260908

Disposition: RETAINED capability evidence; hardware/deployment follow-up remains
Classification: SOFTWARE_VALIDATED_AMD64 / HARDWARE_PENDING_ARM64

Repository: `/workspaces/mowglimavros`
Branch: `main`
Audited HEAD: `54c8b520dee78b9a803c7fda8318bef789fc03da`
Date: 2026-09-08

## Scope and safety

Passive Pixhawk 5X, ArduRover, MAVLink, MAVROS 2.15.1 and GPS/RTK capability
audit. No arming, actuator operation, mode change, parameter write, firmware
change, receiver reconfiguration, or RTCM injection occurred.

## Executive result

- Kilted amd64 workstation software capability: VALIDATED.
- Pixhawk/MAVLink connection over composite USB `if00`: VALIDATED.
- GPS1 originally reported as M9N: NOT OBSERVED. Live configuration was stale
  DroneCAN: GPS1 was configured as DroneCAN for a former HERE4.
- HERE4 reconnected to CAN1 and power-cycled: powered, but still not detected;
  retain as HARDWARE_PENDING. VESC 6 Pro devices historically shared this CAN
  line, so bus/node configuration must be reviewed in a later receiver test.
- GPS2, reported wiring F9P: detected as receiver instance 2 but had no
  outdoor fix during the audit.
- ARM64/RPi4 deployment validation: PENDING because that USB path was unstable.
  This does not invalidate the workstation software result.

## Workstation image and launch

Original image:
`mowgli-mavros-sidecar:kilted-amd64-local`

- Architecture: amd64.
- Immutable image ID:
  `sha256:ba694bba69e90a7dc4e7ebc804205bf15e537a79dc33ac779b3ea7419f152624`.
- Created 2026-09-08; local tag has no RepoDigest and no OCI source revision
  label, so exact source provenance was not encoded in the image.
- MAVROS 2.15.1, pinned commit
  `22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e`; MAVLink 2026.8.8.

Runtime proved the original integration failed by requesting nonexistent
`apm.launch.py`. MAVROS 2.15.1 installs XML `apm.launch` and `px4.launch`.
The minimal patch uses `AnyLaunchDescriptionSource` and those exact names.
Audit image after the patch:
`mowgli-mavros-sidecar:kilted-amd64-audit`, image ID
`sha256:c8245205a241161dde42b2d45dd833b5adf5c0996b06ee53101fa783e46e0c9e`.

Isolated APM launch and live Pixhawk connection passed. An isolated PX4 launch
also resolved and started MAVROS/bridge; its only error was the deliberately
absent `/dev/mavros`. No duplicate backend/node namespace was observed.

The historical MowgliNext `mavrosdev/sensors/mavros/ros2_entrypoint.sh`
corroborates `apm.launch`, `px4.launch`, the serial URL, and target IDs.
`HARDWARE_BACKEND=mavros` remains an orchestration concern to ADAPT later, not
code to copy blindly into the sidecar.

## Pixhawk and USB interfaces

Normal USB identity remained Pixhawk5X, Holybro, serial
`2F002F001050425937353320`, VID:PID `3162:0051`.

- `...-if00` -> `ttyACM0`: live normal ArduPilot MAVLink, remote 1.1. Use this
  stable by-id path for MAVROS.
- `...-if02` -> `ttyACM1`: not equivalent to if00; passive bytes showed binary
  MAVLink2. It was not reconfigured.

Exact Rover-4.6.3 Pixhawk5X `SERIAL_ORDER` maps OTG1 to SERIAL0 and OTG2 to
SERIAL8. There is no SERIAL9 on this board/firmware definition. Live values:

- SERIAL0 = 921600, protocol 2 (MAVLink2): primary USB / if00.
- SERIAL8 = 921600, protocol 2 (MAVLink2): secondary USB / if02.
- SERIAL5 = 115200, protocol 22 (SLCAN): Telem3.
- SERIAL3 = 460800, protocol 5 (GPS): physical GPS1 UART.
- SERIAL4 = 460800, protocol 5 (GPS): physical GPS2 UART.
- SERIAL9 parameters were absent.

Serial passthrough is disabled: `SERIAL_PASS1=0`, `SERIAL_PASS2=-1`,
`SERIAL_PASSTIMO=0`. It was not enabled or tested.

## ArduPilot/MAVLink baseline

- ArduRover 4.6.3, commit `3fc7011a`; ChibiOS `88b84600`.
- Hardware: Pixhawk5X; sysid 1, compid 1; autopilot ArduPilot (3), ground rover
  vehicle type 10; mode MANUAL, armed false.
- Heartbeat approximately 0.5-0.6 Hz and connection survived a user-performed
  power cycle. No arm or mode command occurred.
- Battery telemetry was not usable (0 V/current -1), with expected prearm
  battery/AHRS warnings on the bench.
- Parameter synchronization reached the expected 1014 values but MAVROS logged
  repeated out-of-order/unsolicited indices. Values remained stable; do not
  force another broad pull without investigating this transport behaviour.
- Some frames were classified BAD_CRC during sampling. Only framing-status OK
  frames were used for the message inventory.

Observed OK-frame messages included HEARTBEAT, SYS_STATUS, SYSTEM_TIME,
PARAM_VALUE, GPS_RAW_INT, GPS2_RAW, RAW_IMU, SCALED_PRESSURE, ATTITUDE,
GLOBAL_POSITION_INT, RC_CHANNELS, VFR_HUD, TIMESYNC, EKF_STATUS_REPORT and
STATUSTEXT. No GPS_RTK, GPS2_RTK, GPS_STATUS, LOCAL_POSITION_NED or
HOME_POSITION was observed in the sampling windows.

GPS_RAW_INT and GPS2_RAW were about 0.5 Hz under the parameter-sync load;
GLOBAL_POSITION_INT was about 0.25-0.5 Hz. All valid sampled FCU frames were
from sysid:compid 1:1. These are observed rates, not configured guarantees.

## Live GPS selection/configuration

Relevant live values:

- GPS1_TYPE=9 DroneCAN; GPS1_CAN_OVRIDE=124; GPS1_CAN_NODEID=0;
  GPS1_RATE_MS=125; GPS1_DELAY_MS=20.
- CAN_P1_DRIVER=1 and CAN_D1_PROTOCOL=1: CAN1/DroneCAN enabled.
- GPS2_TYPE=1 AUTO; GPS2_CAN_OVRIDE=0; GPS2_RATE_MS=125;
  GPS2_DELAY_MS=0.
- GPS_AUTO_CONFIG=3; GPS_AUTO_SWITCH=4; GPS_PRIMARY=1;
  GPS_BLEND_MASK=5; GPS_INJECT_TO=127.
- AHRS_EKF_TYPE=3, AHRS_GPS_USE=1. EK3 source set 1 uses GPS for horizontal
  position and horizontal/vertical velocity; barometer for vertical position,
  compass for yaw. Blend mask is not active while AUTO_SWITCH is mode 4.

Mode 4 uses the configured primary when it has at least a 3D fix, otherwise it
falls back to best receiver. `GPS_PRIMARY=1` prefers receiver instance 2.
ArduPilot emits GPS_RAW_INT for fixed instance 0 and GPS2_RAW for fixed
instance 1; this does not follow whichever receiver is currently primary.
GLOBAL_POSITION_INT is the fused EKF vehicle output.

`BRD_RTC_TYPES=1`: only GPS is accepted as a UTC/RTC source. MAVLink
SYSTEM_TIME and hardware RTC are not accepted. Observed SYSTEM_TIME had
`time_unix_usec=0`, so no UTC was available during the no-fix bench test.

## Receiver observations

GPS1/M9N:

- The M9N was never an observed GPS1 data source. The stale DroneCAN setting
  prevented that assumption.
- After the M9N was removed, the HERE4 was connected to CAN1 and the Pixhawk
  was power-cycled. HERE4 LEDs were active, but GPS1 remained
  `fix_type=0`, zero satellites, all position fields zero, and detected node ID
  remained zero before the restart cache reset.
- Do not infer M9N or HERE4 capabilities from this slot. Revisit CAN wiring,
  termination, VESC coexistence and node 124 override in a dedicated test.

GPS2/F9P (identity based on operator wiring, not telemetry identity):

- GPS2_RAW was present: receiver connected but no fix (`fix_type=1`), zero
  satellites, zero position, invalid/unknown accuracy values, yaw zero.
- No RTK FLOAT/FIX, baseline, corrections or usable receiver time was observed.
- MAVLink/MAVROS did not expose the receiver model/firmware identity.

## MAVROS GNSS/RTK surfaces

Key live ROS 2 surfaces:

- `/mavros/gpsstatus/gps1/raw` — `mavros_msgs/msg/GPSRAW`, GPS_RAW_INT instance 0.
- `/mavros/gpsstatus/gps2/raw` — `mavros_msgs/msg/GPSRAW`, GPS2_RAW instance 1.
- `/mavros/gpsstatus/gps1/rtk` and `gps2/rtk` — `GPSRTK`; publishers exist,
  but no messages were observed.
- `/mavros/global_position/raw/fix` — `sensor_msgs/msg/NavSatFix`, derived from
  GPS_RAW_INT instance 0, not the selected/fused primary.
- `/mavros/global_position/raw/gps_vel` and `/raw/satellites` — also instance 0.
- `/mavros/global_position/global` — fused GLOBAL_POSITION_INT position.
- `/mavros/global_position/compass_hdg` — AHRS/compass heading, not GNSS yaw.
- `/mavros/gps_rtk/rtk_baseline` — `RTKBaseline`; no publication observed.
- `/mavros/gps_rtk/send_rtcm` — `mavros_msgs/msg/RTCM` injection input.
- `/mavros/time_reference` and `/mavros/timesync_status` — present but no
  messages observed during sampling.
- `/uas1/mavlink_source` and `/uas1/mavlink_sink` expose inbound/outbound raw
  MAVLink frames.

MAVROS GPSRAW directly retains fix type, position, eph/epv, velocity, course,
satellite count, MAVLink2 accuracy extensions and GPS yaw. For GPS_RAW_INT,
MAVROS hardcodes DGPS channel/age sentinels because that message lacks them;
GPS2_RAW has those fields. GPS_STATUS/per-satellite data was not emitted.

## GPS capability matrix

`DIRECT (empty)` means the ROS contract exists but the receiver supplied no
usable value during this audit.

| Capability | GPS1 slot (M9N/HERE4) | GPS2 slot (reported F9P) | Primary/fused |
|---|---|---|---|
| position | DIRECT (empty) | DIRECT (no fix) | DIRECT MAVROS (no fix) |
| fix type | DIRECT MAVROS | DIRECT MAVROS | DERIVABLE from NavSat status |
| satellite count | DIRECT MAVROS | DIRECT MAVROS | NOT EXPOSED |
| HDOP / VDOP | DIRECT MAVROS eph/epv | DIRECT MAVROS eph/epv | NOT EXPOSED |
| horizontal / vertical accuracy | DIRECT MAVROS extensions | DIRECT MAVROS extensions | NOT EXPOSED |
| velocity / course over ground | DIRECT MAVROS | DIRECT MAVROS | RAW MAVLINK ONLY (GLOBAL_POSITION_INT velocity) |
| GNSS heading/yaw | DIRECT contract, empty | DIRECT contract, yaw=0 | NOT EXPOSED; compass topic is not GNSS |
| receiver UTC/time | NOT EXPOSED faithfully | NOT EXPOSED faithfully | RAW MAVLINK SYSTEM_TIME; currently invalid |
| RTK NONE/FLOAT/FIXED | DIRECT via fix_type | DIRECT via fix_type | DERIVABLE only from selected receiver context |
| RTK baseline | DIRECT contract, no data | DIRECT contract, no data | NOT EXPOSED |
| correction age | NOT EXPOSED (sentinel) | DIRECT contract, no valid data | NOT EXPOSED |
| receiver health | PARAMETER/STATUS ONLY, aggregate | PARAMETER/STATUS ONLY, aggregate | PARAMETER/STATUS ONLY |
| C/N0 / constellation detail | NOT EXPOSED | NOT EXPOSED | NOT EXPOSED |
| jamming/interference | NOT EXPOSED | NOT EXPOSED | NOT EXPOSED |
| raw GNSS observations | NOT EXPOSED | NOT EXPOSED | NOT EXPOSED |
| receiver identity | NOT EXPOSED | NOT EXPOSED | NOT EXPOSED |

## RTCM path

Software-supported path (not injected in this audit):

`NTRIP -> MowgliMAVROS /rtcm -> /mavros/gps_rtk/send_rtcm -> MAVROS
GPS_RTCM_DATA -> Pixhawk AP_GPS -> active GPS backends`

MAVROS 2.15.1 accepts at most 4 x 180 = 720 bytes in one ROS RTCM message,
fragments messages over 180 bytes into at most four MAVLink packets, and rejects
larger messages. ArduPilot reassembles the fragments and injects completed RTCM
data into active GPS backends; live `GPS_INJECT_TO=127` selects all receivers.
Current Mowgli NTRIP reads can contain 4096 bytes and publish the whole vector,
so MM-201 segmentation to chunks <=720 remains required. No byte was injected
into hardware during this audit.

## Universal GNSS adapter gap analysis

Faithful from current MAVROS surfaces when values are valid:

- local receipt stamp generated by the adapter;
- stable configured `source_id` and new `source_incarnation` per adapter/transport
  lifetime;
- `position_observation_sequence` incremented only on a new per-slot GPS sample;
- fix validity/type, RTK mode, position, satellites, HDOP/VDOP,
  horizontal/vertical accuracy, speed and course;
- GNSS yaw only when the GPSRAW yaw field is explicitly valid;
- limited correction state/age for GPS2_RAW; baseline only if GPS_RTK messages
  later appear.

Not faithful from current observed MAVROS alone:

- receiver-native UTC epoch, identity/firmware, C/N0 and constellation detail,
  jamming/interference, raw observations, and distinct per-receiver health.
- The canonical Universal GNSS runtime has speed/course/UTC fields that the
  current ROS `GnssStatus.msg` projection does not expose; that is a separate UG
  contract gap.

Raw MAVLink access may recover vehicle SYSTEM_TIME, GLOBAL_POSITION_INT
velocity and any future GPS_STATUS/GPS_RTK messages, but cannot invent messages
not emitted. Extra MAVROS plugin work could project those fields when emitted.
Serial passthrough is transient and disruptive and does not apply to a DroneCAN
HERE4; it remains unvalidated for a serial receiver. A direct second receiver
link to the RPi is the clearest route to receiver-native UBX diagnostics, but
requires wiring, port ownership and power validation.

## Remaining work

1. Implement/test MM-201 RTCM chunking; do not inject until ready.
2. Dedicated HERE4 CAN1 test: wiring/termination, VESC 6 Pro coexistence,
   discover actual node ID, then decide whether override 124 is correct.
3. Outdoor F9P test with sky view and controlled RTCM test to validate fix,
   RTK state, correction age and baseline surfaces.
4. Final ARM64/RPi4 sidecar and USB validation; the earlier RPi USB bootloop is
   a separate environment/hardware issue.
5. MM-801 actuator/status/failsafe validation remains blocked and was outside
   this passive audit.

## Validation and repository state

- `python3 -m py_compile` passed for the changed launch file.
- `git diff --check` passed before checkpoint update.
- Kilted amd64 image build passed.
- Isolated APM and PX4 launch smoke tests passed launch resolution.
- Live APM connection used if00 and remained disarmed.
- No commit and no push.
