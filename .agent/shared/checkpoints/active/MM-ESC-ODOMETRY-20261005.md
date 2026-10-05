# MM-ESC-ODOMETRY-20261005
Disposition: ACTIVE — STATUS-authoritative ownership corrected; software validation PASS; review/physical acceptance pending.
Repository /workspaces/mowglimavros; branch feat/esc-odometry.
HEAD/baseline e2448ddeffdc6765f6b452ac97ca5cc0d6ba2851; origin Pepeuch/mowglimavros; no submodules.
No staging, commit, push or new deployment. Pre-existing untracked magnetometer
checkpoint excluded and untouched. No MowgliNext/public schema/profile change.

## Approved architecture / current behavior
- User approved internal EscObservation in mavros_esc_wheel_odometry on
  /mavros/esc_wheel_odometry/esc_observation, best effort depth64. Plugin alone
  decodes COMMON ESC_STATUS/ESC_INFO and legacy ESC_TELEMETRY_1_TO_4/5_TO_8/9_TO_12.
  Per-field validity; finite unavailable payloads are not availability sentinels.
  Existing diagnostic totalcurrent/count retained only for actual legacy consumers.
- Canonical wheel motion stays C++; generic differential core owns kinematics.
  Single plugin /wheel_odom producer, preserved twist-only frames/covariance and
  unknown pose covariance1e6. No pose integration or public Mowgli change.
- source=auto chooses configured valid/fresh WHEEL_DISTANCE, then signed COMMON
  ESC_STATUS, then signed RPM226 plus genuine legacy counter progression, else none.
  Explicit sources never fall back. Timer selects/expires but never publishes motion.
- MAVLink time_usec supplies measure stamps and WHEEL_DISTANCE dt; ROS reception
  supplies freshness/timeouts/odom header. Legacy226 lacks measure time and uses
  receipt. Equal receipt with advancing measure accepted; repeated/conflicting or
  invalid measure rejected. Connections, source switches, clock epochs, component
  takeover and accepted parameter changes reset required motion references.
- WHEEL_DISTANCE uses SystemAndOk: framing + vehicle SYSID, not AP COMPID. One
  live encoder owns the baseline, alternate may take over after stale. Optional
  component restriction parameter; Hall encoder never creates ESCObservation.
- Slots, radii, track and COMMON signed motor-to-wheel factors are installation
  parameters, not firmware constants. Atomic on-set validates, post-set commits.
  Rejected candidates preserve prior state; accepted wheel updates retain ESC
  reports and clear wheel references. Legacy scaling remains FCU-owned unchanged.
- Bridge only consumes normalized ESC. Roles default right0/left1/blade2, accept
  distinct slots0..63/-1 disabled and atomic runtime remapping. Plugin retains
  previous safe-disabled wheel defaults until calibrated inputs supplied. External
  owner must configure both parameter services consistently; no automatic sync.
- COMMON raw int32 RPM sign preserved; legacy unsigned RPM magnitude blade-only.
  Missing/stale temperature keeps fresh RPM/current and blade timestamp available.
  Public Status temperature quiet_NaN explicitly user-approved since schema has no
  validity flag; internal temperature_valid=false, fresh INFO restores real value.

## Pinned implementation findings — do not rediscover
MAVROS pin 5c68b905ab30de6ce630822dc46c33467e8f23ea.
Stock mavros_extras esc_status.cpp count starts0 until INFO, so STATUS-only ROS
arrays empty. INFO temperature multiplies cdegC by1E2. Direct plugin decoding
avoids both; no upstream patch. Local plugin_filter.hpp SystemAndOk only filters
SYSID/framing. Stock legacy esc_telemetry.cpp uses voltage/current /100 and
charge /1000; new normalization preserves these exact conversions.

## Files / ownership
Wheel package: CMakeLists.txt, package.xml, mavros_plugins.xml, README.md,
config/esc_wheel_odometry.yaml; msg/EscObservation.msg;
include/mavros_esc_wheel_odometry/{wheel_odometry_core,legacy_wheel_adapter,observation_engine}.hpp;
src/{wheel_odometry_core,legacy_wheel_adapter,observation_engine,esc_wheel_odometry_plugin}.cpp;
test/{test_wheel_odometry_core.cpp,test_observation_engine.cpp,test_esc_graph.py}.
Bridge package: CMakeLists.txt, package.xml, README.md,
config/hardware_bridge_mavros.yaml; include/mavros_hardware_bridge_node.hpp;
include/mowgli_mavros_bridge/{esc_telemetry_tracker,vesc_telemetry_projection}.hpp;
src/mavros_hardware_bridge_node.cpp; test/test_esc_telemetry_tracker.cpp.
Root README/TODO and checkpoint INDEX synchronized. One historical INAV source
finding reworded to eliminate the removed selector name while retaining audit date.

## Initial feature validation (before external review)
Lyrical build/install with pinned MAVROS, exact rc4 GNSS dependencies; build artifacts
outside repo /tmp/mm-esc-feature. Wheel suites PASS: 20 retained legacy cases +27
engine cases. Tracker/blade14 PASS, native pinned MAVROS +bridge graph PASS,
including COMMON alone, sign/current, INFO25.34C/NaN recovery, COMPID201 accepted,
wrongSYSID ignored, one wheel publisher, atomic mappings/track/alternate blade,
unsigned legacy exclusion, expiry, real disconnected/reconnected callback resets.
Provider launch/detection/command graph, GNSS graph, readiness and power regressions
PASS on the final installed build: wheel2/2 suites (47 cases), bridge10/10 suites
(including14 tracker cases), battery1/1 suite. Final bridge matrix completed in72.34s.
Static contracts10 + public interface3 PASS; Python/XML/YAML syntax PASS;
ROS uncrustify check PASS for wheel code/tests and ESC tracker/projection/tests;
git diff --check PASS. Removed selector search (including hidden shared state,
excluding .git and ignored local checkpoints) has no results. ArduPilot/PX4
launch/provider files, GNSS/power implementations and public interfaces unchanged.

## Physical boundary / missing acceptance
Passive Rock/Pixhawk baseline only: connected=true, armed=false, MANUAL; all
manual/neutral/blade paths false. Twelve seconds RPM226=12, legacy11030=12;
COMMON290/291/9000=0. No restart, FCU write, arm/motor command or new deployment.
Temporary observer removed/SSH closed. This window does not prove COMMON firmware
incapability. Existing wheels uncalibrated, so absent physical odometry is not a
regression. No physical PX4/Betaflight/INAV validation claimed.
Missing: installation mappings/calibration, signed wheel motion and measured
physical distance, producer loss/reboot and transport resets on target hardware.
Configurable speed guard catches excessive distance jumps; a plausible cumulative
counter reset without a timestamp/connection change cannot always be distinguished
from legitimate reverse motion. Keep MM-604 physical gates open.

## Exact next step
External review A/B/C corrected and tests passed; user reviews uncommitted patch. Do not commit
or push before review. Next physical acceptance requires calibrated installation
and separately authorized bench procedure; do not arm or command motors now.

## External patch review corrections A/B/C
- Resets split into reset_common/reset_distance plus global reset only for disconnect
  or ROS receive-clock rollback. STATUS time rollback resets COMMON; WD rollback
  resets its owner/baseline. INFO rollback invalidates only its four-slot metadata
  group, preserving RPM/current and independent sources; reset duplicates stay invalid.
- common_pair_max_skew_s=0.25 is finite, positive, <= freshness timeout and applies
  only to STATUS wheel pairing. Separate-group tests cover close, excessive, exact
  boundary, and source transitions through none without cached/new-wheel mixing.
- COMMON packets carry component_id from both native handlers. esc_component_id=-1
  selects one fresh STATUS owner; INFO enriches it without acquiring/renewing its lease.
  Other STATUS components are ignored until expiry; foreign INFO never takes over.
  Genuine repeated frames do not renew ownership. Takeover resets only COMMON.
  Restriction changes discard incompatible cached COMMON reports. No message/public
  schema or cross-node configuration infrastructure changes.
- Native graph extended: both foreign COMMON handlers rejected; component restriction
  runtime accepted/rejected; INFO/STATUS rollback leaves live ESP32 WD baseline intact;
  real disconnect/reconnect still globally clears sources. First extended native PASS.
- Original47 wheel/source cases preserved. Added15 regressions; target engine42 +
  legacy20, all PASS. Final installed Lyrical matrix: wheel2/2 suites (62 cases),
  bridge10/10 suites (75.21s, native ESC26.53s plus GNSS/provider/readiness),
  battery1/1 suite. Tracker14 cases remain PASS. Contracts10 and public interface3
  PASS; Python/YAML syntax, README progress, uncrustify and git diff --check PASS.
  Final whitespace-only plugin formatting rebuild/install PASS (three packages,8.55s).
- Existing physical boundary and no deployment/FCU write/motion/commit/push remain.

## Final COMMON acquisition review
Previous accept_common_component allowed isolated INFO A to acquire/renew an owner,
blocking first STATUS B. Two new regressions reproduced both defects before the fix.
STATUS now establishes common_status_owner_ and alone updates its lease timestamp.
Pre-STATUS INFO retained as a single-component candidate; first STATUS B discards
foreign candidate A immediately, accepts B RPM, and does not merge A metadata.
Same-component candidate INFO can still enrich STATUS, with component restriction
preserved. INFO cannot take over an expired STATUS owner or renew its lease.
Dedicated status/info acceptance helpers and candidate flag survive/reset the same
existing reconfiguration/source lifecycle boundaries. Public messages unchanged.
Final Lyrical build/install PASS (three packages,10.5s). Wheel suites2/2 PASS:
44 engine +20 retained legacy cases (64 total). Bridge10/10 suites PASS (75.76s),
including native ESC26.69s, provider/GNSS/readiness/tracker; battery1/1 PASS.
Contracts10 + public interface3, README progress, uncrustify and git diff --check PASS.
No public interface/launch/provider change from this final ownership correction.
No hardware action, deployment, commit/push; pre-existing magnetometer work untouched.
