# MM-FIRMWARE-ROCK5B-20261005
Disposition: RETAINED — passive serial bootstrap evidence; limitations below.
Repository: /workspaces/mowglimavros
Branch: refactor/firmware-providers
Baseline HEAD: 82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492
Observation window: 2026-10-05T11:21:04+02:00 to 2026-10-05T11:25:42+02:00 (Europe/Paris).
Commit/push: none.

## Authorization and procedure
User authorized auto bootstrap and explicit ArduPilot override on Rock 5B
192.168.10.32 / Pixhawk5X, without arming, motion, motor commands or FCU parameter
modification. Temporary SSH access was supplied by user; no credentials retained.
Read-only ROS subscriptions and local ROS parameter reads; no MAVROS service calls.

## Baseline / reproducibility
- Rock 5B: aarch64, Armbian 26.8.3 / Debian 13, kernel 7.1.8-edge-rockchip64.
- Pixhawk5X: ArduRover V4.7.1 (dbe79216), USB primary if00 -> ttyACM0, 921600,
  target system/component 1:1. Mode MANUAL, connected, armed=false initially.
- Runtime: ROS Lyrical, libmavconn/MAVROS 2.16.0. Original image unchanged:
  sha256:6c21159e54d60610ebc2a6c9df13e618af41238157935d0605bbaf01467d1f0e.
- GNSS_SOURCE=mavros, GNSS_MAVROS_SOURCE=gps2. Wheel config unmapped/unscaled;
  manual_control_enabled, neutral_manual_control_enabled, blade_control_enabled
  all false before and during both candidates.
- Exact new probe compiled ARM64 in a disposable build container using MAVLink
  2026.9.9 dev package extracted in /tmp. No package install in operational image.
  ARM64 loopback test_firmware_detection: 5/5 cases pass (including v1/v2).
- Temporary prefix supplied current launch/probe plus original installed bridge
  binary and configuration. Candidate used exact original image, /dev/ttyACM0
  device and an empty DDS domain 77. Original serial owner stopped before each
  candidate; EXIT cleanup restored original owner. No simultaneous serial owners.
  Main MowgliNext stack remained on domain 0, isolated from candidate commands.
- This is a temporary bootstrap validation, not deployment of a rebuilt full image
  or the updated entrypoint. Default auto/explicit override dispatch is covered by
  10 local launch tests; physical trials set MAVROS_FIRMWARE explicitly.

## Acceptance evidence
| Surface | auto (16 s capture) | ardupilot (35 s follow-up capture) |
| --- | --- | --- |
| HEARTBEAT identity | 16 target1:1, autopilot=3; probe prints ardupilot | 34 target1:1, autopilot=3; probe bypassed |
| Profile | launch logs auto -> apm; argv contains apm_pluginlists.yaml/apm_config.yaml | same apm files; explicit launch log, no detection log |
| Probe close / handover | successful process exit before MAVROS launch; no probe in process list; MAVROS FD points to /dev/ttyACM0 | direct MAVROS FD on /dev/ttyACM0 |
| /mavros/state | 16 messages; connected=true, armed=false | 34 messages; connected=true, armed=false |
| battery_observer | plugin created/initialized, node present; Power 11, BatteryState 6 samples | same plugin/node; Power 21, BatteryState 11 samples |
| GNSS GPS2 | canonical status 13, fix 11; last fix_valid=true, 29 satellites, raw fix_type=5 | canonical status 26, fix 21; last fix_valid=true, 29 satellites, raw fix_type=5 |
| ESC | 11 samples, three populated slots; last RPM=0 | 21 samples, three populated slots; last RPM=0 |
| wheel odometry | absent; plugin explicitly disabled for invalid/unmapped calibration | same existing disabled configuration |

BatteryState/Power were fresh and finite: last battery voltage ~28.926 V auto,
~28.345 V explicit. No new physical power-role interpretation is inferred.
The raw /mavros/battery_observer/status topic is absent in this runtime; plugin
initialization, node and canonical Power/BatteryState delivery prove availability.

## GNSS observation requiring separate follow-up
The first 16-second explicit trial ended with canonical fix_valid=false despite
raw GPS2 fix_type=5 and upstream Universal GNSS fix_valid=true. A 35-second repeat
finished valid but observed false canonical statuses at offsets 2.11, 14.65,
18.67, 22.69 and 26.67 seconds, followed by valid observations. Latest upstream/raw
samples were valid. Cause is not established; do not claim uninterrupted GNSS
freshness or change GNSS semantics to mask this. The adapter, parameters and
firmware were unchanged by this patch. Retain this as a separate GNSS follow-up.

## Safety evidence / limits
No armed=true sample in baseline, candidates or restored captures. Candidate
outgoing MAVLink captures contain no MANUAL_CONTROL, RC override, actuator-control
or actuator-target messages (69/70/139/140), no PARAM_SET/PARAM_EXT_SET (23/323),
and no arming, servo or mode-change COMMAND_LONG (400/183/184/176/209).
Observed commands were normal startup reads: REQUEST_MESSAGE 512 and
GET_HOME_POSITION 410; PARAM_REQUEST_LIST 21 and mission-read traffic are normal
MAVROS initialization. No explicit FCU command or parameter-write API was invoked.
These packet observations are bounded captures, not an unlimited wire audit.

## Restoration
Candidates and builder removed; own /tmp test files removed from original
container. Host /tmp/mm-firmware-test retains non-secret test scripts/data/logs.
Original image/container and Compose configuration were not replaced. Final
16-second capture: 16 state, 10 Power, 5 BatteryState, 10 GPS fixes, 12 GNSS
statuses, 10 ESC messages; connected=true, armed=false, GNSS last valid. Original
restart count remains 0; GUI/main ROS/GNSS containers were not restarted. SSH closed.

## Artifact fingerprints
Source probe SHA256: 2bd54a55b99dfdb5a5f0b14745a4e513527d0b3ca8a8398a11823461dd06ed25
Source launch SHA256: e8bd318bd9c7a0c147110e527eb520b193f3de83ffa2af60ac9695360ab95995
ARM64 probe SHA256: eb0c7756d5e193466cdc88e0c70ef6d166658f0712dcb6819c481a5c620838bc
Host /tmp/mm-firmware-test evidence SHA256:
- auto.json: 8c02d87a439f298265ca8cc480652e11af7d9e81b0c67512afee3201d09a79bd
- auto.log: a7e02e32112db45c7ecb2cf905ecb65c74751c7b484305110e75aa44d372f8f5
- ardupilot.json: 456be0277750a2404d995ff9bb6a30db18314382ba9f1b38909665eb67c3f8ac
- ardupilot.log: a87282373641643b4aa0c29a3ef0be28ee59acd0c5c1c20abdbb067ba65bd613
- restored-final.json: f97df0b3d8e8b8bb5c77c0b21e1e3491d85fa6cb6a3d5fbccbf17eb1af1c809e

## Remaining gates / exact next step
Review/commit the firmware patch separately from pre-existing magnetometer work.
Investigate canonical GNSS invalidity/freshness without changing this resolver.
Wheel calibration, PX4 physical proof, TCP bootstrap, USB-loss recovery and full
image deployment remain separate acceptance work. Revalidate if firmware, targets,
transport, MAVROS/libmavconn, profiles or hardware topology changes.
