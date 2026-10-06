# MM-ESC-ODOMETRY-20261005
Disposition: ACTIVE — raw motor ticks refactor validated; signed RPM bench validation complete; RTK metric calibration/odometry acceptance pending.
Repository /workspaces/mowglimavros; branch feat/esc-odometry.
Current HEAD 8e29b196f3c1c31979cb437f76f8b01866194466; original feature baseline e2448ddeffdc6765f6b452ac97ca5cc0d6ba2851; origin Pepeuch/mowglimavros; no submodules.
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
- Slots, ticks_per_meter and track width are installation parameters, not firmware
  constants. Raw signed motor RPM integrates into fractional ticks (1 motor
  revolution per tick); ticks_per_meter alone converts motor ticks to metres.
  No wheel RPM, radius or theoretical gear ratio is assumed. Calibration updates
  retain compatible raw accumulation; source/mapping epochs reset only affected
  accumulators. Atomic on-set validates, post-set commits. FCU SCALING stays1.0.
- Bridge only consumes normalized ESC. Roles default right0/left1/blade2, accept
  distinct slots0..63/-1 disabled and atomic runtime remapping. Plugin retains
  proven default wheel mapping (right ESC0/RPM1, left ESC1/RPM2), with metric
  RPM odometry disabled by ticks_per_meter=0 and track_width_m=0. External
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

## Live Rock5B bench — 2026-10-05 (Europe/Paris)
User initiated physical live validation, operator commands motors manually. Agent
strictly passive: no ARM/mode/motor/blade/FCU parameter command, deployment or
permanent configuration. Only optional upstream send_raw ROS runtime authorized.
Current repository HEAD8e29b196f3c1c31979cb437f76f8b01866194466, feat/esc-odometry
(user's feature commit now present; agent did not commit/push).
Hardware reported: Pixhawk5X/ArduRover, robot on support, wheels raised/blade clear.
Host Rock5B aarch64 Armbian/trixie, kernel7.1.8-edge-rockchip64. Container
mowgli-mavros running requested feat-esc-odometry-lyrical image, image ID
sha256:2032c3d0f7f759902272dcc3fba47926a836e05dab8618c0ea182398d756efcb.
Loaded all four user-specified ROS/MAVROS/UniversalGNSS/workspace environments.

Chronology / baseline before modifications:
- 17:04:20: received connected=true, armed=false, MANUAL, SYSID1/COMPID1.
  ESC0/1/2 source2 valid, unsigned magnitude0, direction_valid=false.
  ESC0 about28.12V/0.02A/42C; ESC1 about27.93V/0.02A/41C;
  ESC2 about28.10V/0A/37C. Empty ESC3 invalid as expected.
  Bridge blade stopped200, RPM/current0, temperature37C, fresh timestamp.
- Plugin param dump: sourceauto; slot/instance/wheel-index/offset -1;
  radii/track0; timeout3s, skew0.25s, component restrictions -1, factors1.
  Bridge roles right0/left1/blade2 and drive/neutral/blade controls false.
  /wheel_odom baseline publishers0/subscribers7 (before observer adds one).
  Diagnostics received and saved; no wheel odometry expected until configuration.
- Upstream /mavros/wheel_odometry absent from node list and exact param list
  reports Node not found. No dedicated RPM raw topic in topic list. send_raw not
  available: no ROS mutation performed, no plugin load/restart/odom configuration.
  Raw226 decoded passively on /uas1/mavlink_source (mavros_msgs/Mavlink), framing
  OK, SYSID1/COMPID1; rpm1=-1,rpm2=-1 at rest. Mapping/sign not inferred yet.
- Temporary observation reader needed Python representation fixes (diagnostic
  uint8 exposed as bytes; rclpy nanoseconds property). Failed attempts closed
  cleanly; only observer affected, no runtime configuration writes.
- 17:06:21.855: continuous passive observer successfully running. Summaries every
 2s, per-message JSONL at container /tmp/mm-esc-live-20261005.jsonl. Raw226 and
 legacy11030 each about10Hz; counters advance about97..99/2s even while stopped.
 17:06:23..29: ESC0/1/2 valid/fresh (age~0.08..0.10s), zero RPM, no stale/reset
 or source mixture; source=none, wheel_messages0. COMMON290/291/9000 absent in
 this window. ESC0~28.32V/41..42C, ESC1~28.14V/41C, ESC2~28.32V/36..37C.
 Blade public projection agrees, raw226 remains -1/-1 (do not interpret direction).
- Ready for operator RIGHT WHEEL FORWARD phase. All phase/mapping conclusions
 remain pending. Stop and explain if an unexpected condition is observed.

MM-902 deferred stream/plugin policy added only to TODO; generated dashboard synced.
No work on that policy. Preserve no-commit/push during bench validation.

### 17:26:30–17:26:42 — operator rotates right wheel, direction unconfirmed
User announced right wheel turning; do not label forward/reverse without confirmation.
Observed ESC0 magnitude60,168,133,145,135,134,155 RPM in successive2s samples.
ESC1 and ESC2 stayed0RPM; right-role mapping ESC0 confirmed for this phase.
ESC0 source2, valid/fresh, rpm_direction_valid=false; voltage~28.17..28.21V,
current0A in sampled rotation windows, temperature41C. No claim powered drive
or load from this observation; armed remainedfalse, MANUAL, connectedtrue.
ESC0 count32409->33005 over~12s, progression97..104 per2s; ESC1/2 counters also
progressed while their RPM stayed0 (freshness is not a motion signal).
Blade projection stayed stopped200/RPM0/current0/temp36C, independently fresh.
No source switch, wheel source none, wheel_messages0, no stale/receipt rollback
or mixed-source diagnostic in these samples. COMMON290/291/9000 not observed.
Unexpected limit: raw226 received~10Hz SYSID1/COMPID1 but rpm1/rpm2 stayed-1/-1
throughout right-wheel motion. Cannot determine RPM226 channel or signed direction.
Do not interpret -1 as wheel reverse; no FCU setting/readback diagnosis made yet.
Agent explained the mismatch and requested operator stop before another phase;
analysis paused at this point. Passive capture remains running. No commands,
configuration changes, commit or push. Next: confirm wheel stopped and decide
read-only investigation of unavailable signedRPM path before advancing phases.

### 17:27:46–17:27:56 — right wheel, operator announces reversal
Operator reports change of rotation direction; absolute forward/reverse labels
still unconfirmed. Preceding received summaries17:27:38/40 showed ESC0=0RPM,
providing observed stop before this second motion window (exact reversal onset
not independently timestamped). ESC0 magnitudes186,152,174,176,192,161RPM;
ESC1/ESC2 remained0. ESC0 source2/fresh/direction_valid=false, ~28.10..28.14V,
0A,41C. Count36152->36643, ~98..99 per2s. Blade remained stopped with
RPM/current0, temp36C. No stale/reset/source mixture, source none, wheel_messages0.
Raw226 continued-1/-1 at~10Hz throughout reversal: no signed-channel mapping can
be inferred. Magnitude staying positive on reversal is expected for legacy ESC,
not a decoder sign regression. SignedRPM limitation remains open; no configuration
or FCU change, no actuation by agent. Capture stays active; request operator stop
before next phase. Do not mark forward/reverse odometry acceptance passed.

### 17:28:39–17:28:43 — stop confirmed after right-wheel phases
Operator reports completion. Three successive summaries confirm ESC0/1/2=0RPM,
valid/fresh, counters still advancing; connected/unarmed MANUAL. No diagnostics
issue, wheel source none, wheel_messages0. Raw226 remains-1/-1. Right-wheel ESC0
magnitude mapping established in both operator-reported directions; signedRPM
and absolute forward/reverse labels not validated. Capture remains ready to
observe left-wheel magnitude independently; signed wheel odometry acceptance
continues blocked by unresponsive226, without any parameter/configuration changes.

### 17:29:49–17:29:53 — left wheel first direction, operator initiated
Prior summaries17:29:45/47 confirmed all three ESC RPM0. On left-wheel rotation,
only ESC1 reacted: magnitude140,126,109RPM; ESC0/ESC2 stayed0. Left-role mapping
ESC1 confirmed. Source2, direction_valid=false, valid/fresh (age~0.06s);
voltage~27.84..27.87V/current0A/temp41C. Count55450->55647, ~98..99/2s.
Connected/unarmed MANUAL; blade public stopped200/RPM0/current0/temp36C fresh.
Raw226 still-1/-1 despite left-wheel rotation; no channel/sign inference.
Wheel source none, wheel_messages0, no stale/reset/mixed-source issues observed.
Absolute forward/reverse unconfirmed. No commands or parameter changes by agent;
capture remains active. Operator to stop before reversing or next phase.

### 17:30:57–17:31:11 — left wheel stopped, opposite direction pending
Operator reports stopped left wheel and intention to reverse. Received summaries
confirm ESC0/1/2=0RPM throughout this window, all live slots fresh and counters
progressing. ESC1~27.78..27.81V/0..0.01A/41C; blade public RPM/current0/temp36C.
Source none, wheel_messages0, no reported stale/reset/source mixture. Raw226
still-1/-1. No opposite-direction motion yet verified in these samples; capture
ready for operator's next left-wheel phase. No actuation/configuration by agent.

### 17:31:47–17:32:01 — left reverse; 17:33:50/52 — stopped
Captured second left motion: only ESC1 reacts, magnitudes131,138,157,156,125,
146,151,143RPM; ESC0/2 remain0. ESC1~28.17..28.23V/0A/41C, count61300->61996,
progress~98..103/2s, source2/fresh/direction_valid=false. Raw226 stays-1/-1;
source none/wheel_messages0/no issues. Latest17:33:50/52 confirms allESC RPM0,
connected/unarmed MANUAL, valid/fresh. Capture remains active.
Operator now explicitly confirms phase order: RIGHT FORWARD -> RIGHT REVERSE ->
LEFT FORWARD -> LEFT REVERSE. This resolves historical absolute-label uncertainty
for phase annotation, not signed telemetry proof. Operator reports VESC/DroneCAN
configuration already defines these directions; no VESC/FCU configuration readback
or command-routing verification performed by agent. ESC0right and ESC1left
confirmed in both reported directions. Legacy uint16 RPM observation remains
magnitude-only by contract; VESC direction configuration alone does not establish
an available signed RPM226 transport. Raw226 mapping/sign still cannot be inferred
from unchanged-1/-1. Signed wheel odometry acceptance remains pending.
No agent commands/config writes/deployment/commit/push. Next independent magnitude
phases (two wheels/blade) pending; do not configure odometry from legacy magnitude.

### 17:36:46–17:36:50 — operator turns blade, observation in progress
Prior17:36:40..44 summaries allESC RPM0. Blade phase: only ESC2 reacts,
magnitude37,69,48RPM; ESC0/1 remain0. ESC2 source2/valid/fresh, direction_valid=false,
voltage~28.29..28.31V/current0A/temp36C. Count62598->62793 (~97..98/2s).
Public /hardware_bridge/status follows ESC2: mower_esc_status201, mower_motor_rpm
37/69/48 respectively, current0, temperature36C and fresh blade_status_stamp.
This validates observed blade projection for the manual phase, not actuation command
routing or loaded motor current. FCU connected/unarmed MANUAL throughout.
Raw226 remains-1/-1, source wheel none, wheel_messages0, no stale/reset/mixedsource.
Legacy blade magnitude is never used as signed wheel direction. Agent issued no
command/config write. Capture remains active; await operator stop before closing
blade phase. Two-wheel simultaneous phases still pending.

### 17:38:05–17:38:21 — blade opposite direction, operator reported
Stop before inversion observed17:37:33..57: allESC RPM0 and public blade stopped200.
Opposite blade motion then observed exclusively on ESC2, sampled magnitudes
40,20,210,95,141,21,281,174RPM (not a signed direction measurement). ESC0/1 stayed0.
ESC2 source2, direction_valid=false, valid/fresh; ~28.14..28.18V/0A/36C,
count892->1674 (~97..99 per2s), reception age~0.09..0.12s. Counter rollover
65535->0 occurred earlier while stopped; modular progression remained normal,
no freshness reset or source-mix alert. Public bladeRPM/current/temp follow ESC2
with status201 while moving and fresh timestamp; no discrepancy in shown samples.
Raw226-1/-1 persists, wheel source none, wheel_messages0, no issues. No agent
commands/configuration changes. Capture active; await operator stop to close phase.

### 17:39:03–17:39:07 — blade stopped, individual phase results
Operator confirms completion. ESC0/1/2 returned to0RPM in three consecutive
summaries, all valid/fresh, counters progressing. Public blade returned to
stopped200/RPM0/current0/temp36C with fresh timestamp. Connected/unarmed MANUAL;
source none, wheel_messages0, no diagnostics issue. Thus ESC2-only blade
magnitude/projection observed coherently in both operator-reported directions,
and return-to-stop projection confirmed. Right0/left1/blade2 independent-role
mapping pass for this manual bench. No actuation-command/current-under-load,
signed wheelRPM, odometry/calibration or COMMON physical acceptance claimed.
Raw226 remains-1/-1. Two-wheel simultaneous forward/reverse phases not yet done.
Capture remains active, no parameter change/agent actuation/commit/push.

### 17:49 — RPM backend source diagnosis and operator reboot
Live FCU logs report ArduRover V4.7.1 (dbe79216). Official Pixhawk5X release
https://firmware.ardupilot.org/Rover/stable-4.7.1/Pixhawk5X/git-version.txt
resolves dbe792162d06cab66c3475fd5556bf7a120f119e; its features.txt explicitly
includes AP_RPM_ESC_TELEM_ENABLED (no exclusion prefix). This proves inclusion
in the official matching target/release; custom compilation variants cannot be
identified solely from the short FCU git hash. No custom build claimed here.
Pinned libraries/AP_RPM/AP_RPM.h maps ESC_TELEM=5, DRONECAN=7.
RPM_DroneCAN.cpp consumes dronecan_sensors_rpm_RPM sensor_id. In contrast,
AP_DroneCAN::handle_ESC_status feeds signed msg.rpm into AP_ESC_Telem; its
get_average_motor_rpm uses a bitmask, preserves sign, and RPM_ESC_Telem.cpp
multiplies by instance scaling without abs. Mask1 selectsESC0/right; mask2 selects
ESC1/left. AP_RPM::init creates driver once, update does not recreate it for a
TYPE change: reboot required for7->5. GCS_MAVLINK::send_rpm initializes fields-1,
then get_rpm fills only healthy instances: previous-1/-1 consistent with absent
DroneCAN RPM sensor messages, not wheel reverse.
Minimal candidate from initial7/7: RPM1_TYPE=5 and RPM2_TYPE=5 only, keeping
ESC_MASK1/2 and SCALING1.0 unchanged. Read-only health check: both MIN_QUAL0.5,
ESC_INDEX0. ESC backend quality0.5 passes default threshold; outbound feedback
is disabled. AP_RPM_ESC_Telem returns average0 even without fresh ESCs, therefore
rawzero alone is NOT proof of stopped/fresh hardware: retain ESC counter gating.
Operator reported correct configuration and reboot. Agent did not write a
parameter or issue reboot/ARM/mode/actuator command. Earlier ROS cached read5/7
was superseded by post-reboot force_pull read success=true,967 parameters;
confirmed actual readback TYPE5/5, MASK1/2, SCALING1/1. No other setting changed
by agent. At17:49:20..56 passive raw226 changed to0/0 at rest; connected/unarmed
MANUAL, ESC0/1/2 zeroRPM/fresh/counters advancing, source none/wheel_messages0.
Disconnect itself not established from latest filtered window; do not claim its
full capture without reading chronology. Observation remains running.

Passive post-reboot protocol (prepared, movement results pending):
1. Confirm readback5/5, masks1/2, scalings1; connected/unarmed, fresh ESC counters,
   raw226 received and0/0 at rest. Keep odometry geometry/mappings disabled.
2. Operator rotates right-forward, stops, right-reverse, stops: test rpm1 responds
   exclusively and changes sign, rpm2 remains0; compareESC0 magnitude/freshness.
   Positive-forward convention must be measured, not inferred from configuration.
3. Repeat left-forward/stop/left-reverse/stop for rpm2 andESC1 with rpm1zero.
4. Operator both-wheels forward/stop/reverse/stop: independent fields, correct
   signs, no cross-pairing, discontinuity or source reset. Never use unsigned ESC
   magnitude as wheel direction or configure geometry before mapping/sign proven.
5. Operator blade-only briefly: ESC2/public blade respond, rpm1/rpm2 stay0 with
   wheelESC counters fresh; no /wheel_odom, no blade leakage into wheel feedback.
6. After every stop confirm rawfields0, fresh counters, no-1/drop/stale/source mix.
   Any anomaly pauses acceptance at that point; no additional parameter writes.
No agent motor commands. This validates feedback mapping/sign only, not wheel
radius/gearing/distance calibration. /wheel_odom remains unconfigured/no producer.

### 17:52:51–17:53:09 — post-TYPE5 right FORWARD, first signed proof
Operator explicitly rotates right wheel FORWARD. ESC0 alone reacts (111/131RPM
initial samples, later210/176RPM); ESC1/2 stay0. Raw226 rpm1 positive113.051,
129.829,206.057,178.436 in matched snapshots, rpm2=0. Sampled2s raw range reaches
+224.516RPM; ESC magnitude and signedRPM are consistent with asynchronous
sampling/interpolation, not expected bit-identical. Confirm right=RPM1 and
FORWARD positive for this installation; reverse sign still requires its own phase.
ESC0 source2/unsigned direction_valid=false unchanged, fresh~0.02..0.08s,
~28.03..28.07V/0A/41C; count3782->4565 with ~97..98/2s progression. No other
ESC motion, stale/reset/source mix or wheel output (source none/wheel_messages0).
At17:53:09 allESC RPM0 and raw2260/0: return-to-stop observed once; continue
separate stop check before reverse. Connected/unarmed MANUAL. No agent command,
parameter change or odometry enable. Record pass only for right-forward feedback,
not complete sign calibration or wheel-distance accuracy. Capture active.

### 17:53:57–17:54:11 — post-TYPE5 right REVERSE, sign inversion confirmed
Operator reports stop then right REVERSE. Prior17:53:49..55 summaries allESC RPM0,
raw2260/0, counters fresh. Reverse: onlyESC0 magnitude139/142/167/177RPM;
rawrpm1 negative-139.821,-160.498,-166,-183RPM in shown snapshots, rpm2 stays0.
Two-second negative ranges include-185.133..-109.386RPM. A brief zero at17:54:07
was followed by further reverse motion; final stop is not assumed from this sample.
Right=RPM1, forwardpositive/reversenegative feedback convention now measured for
this installation. Legacy ESC0 continues unsigned magnitude/direction_valid=false,
source2/fresh, ~27.95..28.01V/0A/41..42C, count7019->7710 and~98/2s progression.
ESC1/ESC2 remain0; no stale/reset/source mixture, source none/wheel_messages0.
No wheel geometry/odom enable or FCU/ROS parameter change by agent. Left signed
mapping and both-wheel/blade isolation post-TYPE5 still pending. Capture active;
operator to stop before advancing phase. Evidence validates feedback signs, not
powered actuation/calibration/ground-distance performance.

### 17:55:08–17:55:14 — post-TYPE5 left FORWARD, signed channel proof
Right stop verified in17:54:49..59: allESC RPM0, raw2260/0, fresh counters.
Operator explicitly rotates LEFT FORWARD. OnlyESC1 responds (197/193/139RPM
shown), rawrpm2 positive195.197/200.543/114.822RPM; rawrpm1 stays0 and ESC0/2
stay0. Left=RPM2 and forwardpositive confirmed; reverse still pending.
Two-secondrpm2 range reaches+209.961. ESC1 source2/magnitude/direction_valid=false,
valid/fresh (age~0.004..0.10s), voltage~27.75..28.06V/current0A/temp41C,
count10569->10871 with ~99..104/2s progression. No stale/reset/source mixture,
source none/wheel_messages0. Asynchronous samples need not match exactly.
No agent parameter write/actuation/odometry configuration; capture active.
Operator to stop before left reverse; calibration and simultaneous phases pending.

### 17:56:42–17:56:52 — post-TYPE5 left REVERSE, second wheel sign confirmed
Operator reports stop then same left wheel REVERSE. OnlyESC1 reacts: magnitude
208/206/157/156/183/160RPM. Rawrpm2 negative-201.164,-206.718,-160,-154.180,
-182.744,-161.265 in corresponding snapshots; rpm1=0 throughout. Per2s range
includes-220..-108.696RPM; no positive motion readings in these shown windows.
ESC0/ESC2 stay0. ESC1 source2/unsigned direction_valid=false, valid/fresh age
~0.07..0.10s, voltage28.23..28.26V/current0A/temp41C, count15232->15731,
progress~98..104/2s. Connected/unarmed MANUAL; source none/wheel_messages0,
no stale/reset/mixed-source diagnostic. Agent sent no commands/parameter changes.
Now both individual feedback mappings measured for this installation:
RIGHT=ESC0=RPM1, LEFT=ESC1=RPM2; each forwardpositive/reversenegative with scaling1.
This does not validate powered command routing, motor-to-wheel gearing/radius,
track calibration, ground-distance accuracy or configured /wheel_odom output.
Capture active; left final stop, both-wheels forward/reverse and post-TYPE5 blade
isolation remain pending. Prior blade phase was before TYPE5; do not substitute it
for proof that new RPM1/RPM2 exclude blade telemetry.

### 17:57:46–17:57:48 — all stopped; operator waives blade repetition
Operator reports stopped. Two successive summaries confirm raw2260/0 and allESC
RPM0/fresh, counters advancing, connected/unarmed MANUAL, no issues, source none,
wheel_messages0. Individual signed wheel phases completed: rightRPM1+/-,
leftRPM2+/-. User explicitly declines blade retest because blade configuration
unchanged. Retain earlier ESC2-only/projection evidence; post-TYPE5 blade isolation
is waived/not executed, not newly PASS. No additional blade test planned now.
Both-wheel simultaneous phases still not executed, wheel geometry/odom remains
unconfigured and distance calibration unvalidated. Capture stays active awaiting
next operator instruction; agent made no configuration/actuation/commit/push.

## Authorized active synchronization bench — pending execution
Operator explicitly authorizes agent ARM and necessary two-wheel tests, is next
to robot on secured support, blade physically removed. This supersedes earlier
no-ARM/no-wheel-command constraint for this bounded test only. No persistent FCU
settings, modes, blade commands, deployment or commits authorized/needed.
Current preflight: connected/unarmed MANUAL, allESC RPM0/fresh, raw2260/0.
Read-only FCU routing: MAV_GCS_SYSID255, RC_OPTIONS65, RC_OVERRIDE_TIME3s,
RCMAP_THROTTLE3/ROLL2, SERVO1_FUNCTION74(right), SERVO2_FUNCTION73(left),
SERVO3_FUNCTION35, reversals0, CAN_D1_UC_ESC_BM7/OF0/RV7. Do not infer actuation
polarity from CAN reversal mask; measure signed feedback. Bridge drive/neutral/
blade flags allfalse: direct MAVROS manual input is an isolated bench path,
not validation of bridge /cmd_vel. Seven focused legacy pairing tests PASS.
Prepared /tmp/mm-sync-bench.py, syntax PASS. Procedure: fresh stationary ESC/raw/
servo preflight, read calibrated RC3 neutral/deadzone; verify20Hz neutral wire
255:191->1 and unchanged servo/blade; single ARM request, require confirmed armed
MANUAL; neutral; z+100/steering0 max3s (MANUAL scale10%, NOT VESC duty);
neutral3s and zero-confirm; z-100 max3s; neutral3s then disarm confirmation.
No auto escalation/retry: abort on ARM refusal, nonresponsive/two-wheel mismatch,
oppose signs, stale/disconnect/mode change, >500signedRPM, >5A/65C/lowvoltage,
bladeRPM/current/servo activity or missing wire command. Cleanup neutral/disarm
also runs after exceptions. Ordinary checks remain enabled; no forced arming.
Accept only actual simultaneous signed-wheel response and return-to-zero, not
same RPM magnitude (independent motors/gearing), odometry or route calibration.
Trace planned container /tmp/mm-wheel-sync-active-20261005.jsonl. Results pending.

### 18:16:29–18:16:53 — first authorized simultaneous powered trial
Preflight passed: RC3 calibration1013..2016, trim1515/DZ30; neutral wire
255:191->1 verified, RCout1500/1500/0/0 unchanged. ARM accepted(result0),
confirmed armed MANUAL. Positive z100 began18:16:32.510; nine nonzero wire
commands sent. At18:16:33.079 (>500RPM) guard aborted after~0.569s, switched
neutral, skipped reverse. Disarm accepted18:16:36.148(result0), final18:16:36.262
connected/unarmed/raw2260/0. Independent observer18:16:49..53 confirms stopped.
Raw synchronized pairs during powered ramp:0/0;0/0.929;89.262/92.632;
240.560/238.983;399.583/397.495;559.753/556.242. Both positive, consistent ramp;
above~240RPM relative pair difference<1%, at~90RPM about3.7%. Sampling~10Hz
cannot establish sub-sample mechanical synchrony; equality not required.
RCout1510/1510/0/0 observed during pulse. ESC0max582RPM/currentmax0.12A/
voltagemin28.07V/tempmax42C; ESC1max586RPM/currentmax0.18A/27.92V/41C.
ESC2 remains0RPM/0A/36C, SERVO3zero, no commanded blade activity. No wheelodom.
Abort is explicit speed-limit protection, not diagnosed source/actuator fault.
No limit raised or FCU setting changed. One lower-amplitude complementary trial
prepared under the same explicit operator authorization (next to secured robot,
blade removed): z+/-75 (7.5%MANUAL), max2s each, same500RPMguard, neutral/disarm,
require >=3 simultaneous moving samples; no further auto escalation or trials.
Source/telemetry software tests remain distinct from this direct-MAVROS actuation.

### 18:20:26–18:21:20 — lower complementary trial, active tests stopped
New fresh preflight and calibrated neutral passed. Single ARM for this lower
trial accepted(result0). z+75 started18:20:29.456, ten nonzero commands; guard
aborted18:20:30.098 after~0.642s on >500RPM, no z-75 sent. Neutral3s then disarm
accepted18:20:33.179(result0), final18:20:33.242 connected/unarmed/RPM0/0.
Independent observer18:21:18/20 confirms allESC zero/fresh/raw0/0 and no issues.
Measured paired ramp:0/0;0/0;65.180/66.133;209.377/207.187;372.017/370.846;
529.913/525.841. First nonzero>2RPM occurs in the same~10Hz sample for both wheels,
not proof of tighter physical simultaneity. Moving-pair relative differences
~1.45%,1.05%,0.32%,0.77%. Both positive, consistent slopes, no channel mixing.
ESC0max562RPM/currentmax0.15A/tempmax42C; ESC1max559RPM/currentmax0.16A/41C;
ESC2 remains0RPM/0A/36C. Only one RCOut snapshot during this short phase shows
1500/1500/0/0 (initial output); do not claim steady active PWM from this sample.
Normal count progression observed, no stale/source reset. Source none,
/wheel_odom remains unconfigured and has no samples. Bridge flags unchanged.
No third trial or safety-limit increase. Positive powered simultaneous feedback
and neutral/disarm response evidenced in two short ramps. Full steady-state
synchronization, simultaneous powered reverse, body-ground accuracy and wheel
odometry acceptance NOT passed. Operator-informed limit stop; no FCU settings,
mode, blade command, deployment, commit or push. Passive observer remains active.
Active trace /tmp/mm-wheel-sync-active-20261005.jsonl in container; temporary
script /tmp/mm-sync-bench.py on host/container (current lower version). Shared
checkpoint stores summaries only, no raw logs or credentials. Next define bounded
speed/duration for any further separately reviewed active protocol; do not keep
retrying beyond this stated stop. Preserve existing hardware/calibration gates.

## Explicit follow-up authorization — powered simultaneous reverse
Operator specifically requests same trial in REVERSE to complete feedback-direction
bench evidence. One reverse-only attempt prepared: fresh stationary preflight,
neutral wire/RC validation, ARM once, neutral, z-75 max2s with same500RPM/current/
freshness/blade guards, finalneutral3s/disarm. No positive pulse or configuration
change. Preserve source/geometry/steady-state limits even if brief feedback passes.
Execution/result pending; no commit/push.

### 18:29:24–18:30:08 — authorized simultaneous powered REVERSE
Fresh stationary preflight/neutral validation passed; ARM accepted18:29:26.212
result0. z-75 pulse begins18:29:27.512; nine nonzero wire commands; >500RPM
protective abort18:29:28.111 (~0.599s), neutral3s then disarm accepted(result0)
18:29:31.253. Final18:29:31.310 connected/unarmed/raw0/0. Independent observer
18:30:06/08 confirms allESC RPM0/fresh/countsprogress, noissues/source none/odom0.
Signed pair ramp:0/0;0/0;-94/-93;-236.025/-235.450;-394.427/-395;
-562.032/-557. Both negative and first moving in the same~10Hz sample. Moving
pair relative differences~1.07%,0.24%,0.15%,0.90%; sub-sample mechanical
synchronization/steady-state equality not established. RCout1490/1490/0/0.
ESC0 magnitude max587RPM/currentmax0.14A/voltagemin28.03V/tempmax42C;
ESC1 magnitude max580RPM/currentmax0.10A/27.85V/41C. Counter deltas25/25 in
shown pulse, ESC2 progresses too but remains0RPM/0A/36C/SERVO3zero. No blade
actuation or wheel/ESC role contamination observed. No parameters/mode changed.
Now established on this installation: manual isolated F+/R- mapping for both
wheels, powered common-command positive and negative coherent ramps, explicit
neutral return and disarm, continued independent legacy ESC freshness. Signed
RPM source is TYPE5/MASK1right/MASK2left/scaling1. This is feedback/short bench
acceptance only: sustained sync, steering/mixed directions, /cmd_vel bridge gate,
geometry/gearing/ground-distance and configured /wheel_odom remain unvalidated.
All active trials ended on declared500RPM guard; no limit raised, no retries now.
No deployment/commit/push. Passivecapture80076 remains active; active guard process
exited. Preserve /tmp/mm-wheel-sync-active-20261005.jsonl for trace analysis.

## Raw motor ticks refactor — validated software implementation
User declares physicalRPMvalidation complete. FCUscaling1 preserved; rawsignedmotor
RPM226 must not be treated as wheelRPM. No new active commands/deployment/FCU changes.
Unit convention1fractionaltick=1motorrevolution, scalarcanonical ticks_per_meter
unset0, trackwidthunset0. Shared MotorTickIntegrator integrates eachmotor with
its own measureinterval (COMMONwiretime, legacyreceipt) using trapezoid signed
RPM/60. Currenttickrate RPM/60 converts speed; accumulatedticks convert distance.
No quantization, radius, gearing, invented physical calibration or PID changes.
Per-source COMMON/legacy counters avoid double-counting; separateL/R diagnostics
available withoutcalibration. Counterepoch and segments identify resets/gaps;
unknownstale duration is not integrated. Parametercalibration changes retaincounts
and timing when mappingcompatible; mapping/producerepoch resets owncounts.
Legacyadapter only qualifies rawRPM with genuinecounterprogress; sourcecorekinematics
unchanged; WHEEL_DISTANCE alreadymeters keepsdirectdt path. Defaults now reflect
proven rightESC0/RPM1,leftESC1/RPM2,offset0; tpm0/track0keepmetricdisabled.
C++/native tests adapted to tickunit calibration (fixturevalues only), add7 pure
integrator+5engine regressioncases. Final validation below supersedes intermediate
runs. Artifacts /tmp/mm-esc-ticks with exactrc4deps. Diagnostics
rawtick/calibration precision uses max_digits10. Command bridge/provider untouched:
/ cmd_vel -> existing firmware MANUAL_CONTROL mapping, no Mowgli wheelPID/FF.
TODO and READMEs synchronized; final software validation complete below.
Do not touch magnetometer checkpoint or commit/push.

### Final raw motor ticks validation — 2026-10-05
- Lyrical build/install PASS: mavros_esc_wheel_odometry, mowgli_mavros_bridge,
  mavros_battery_observer; pinned MAVROS and exact rc4 GNSS dependencies.
- Wheel/source 76 cases PASS: integrator7, observation engine49, pairing/core20.
- All10 bridge suites PASS: launch/provider/detection/provider graph, native ESC
  graph, GNSS graph, readiness13, ESC tracker14, command provider5, manual control3.
- Power state8 PASS. External backend contract10 and public interface contract3
  PASS; README progress check, Python/YAML syntax, C++ uncrustify and diff check PASS.
- Expanded native graph verifies uncalibrated RPM produces no metric publisher
  while signed separate L/R raw counters continue, then dynamic calibration
  restores metric publication without erasing counters. Fixture calibration only.
- Public Mowgli schemas, bridge command/provider code and MowgliNext unchanged.
  No Mowgli wheel PID/feed-forward added to MAVROS command chain.
- No physical calibration invented, no new deployment/FCU writes/actuator commands,
  staging, commit or push during this refactor. Prior magnetometer work untouched.
Next: owner supplies measured track width and calibrates ticks_per_meter against
a known straight RTK distance, comparing separate left/right motor accumulation;
then validate configured metric odometry on hardware. Current deployed runtime
has not received this refactor. Historical sections above retain bench chronology.
