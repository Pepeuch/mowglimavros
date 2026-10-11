# Minimal FCU adapter (safety-critical actuation change)

Work repository: MowgliMAVROS, branch `main` (direct local commit authorized; former work branch transferred), based on
`main` e7d516c. MowgliNext source and its submodule gitlink remain unchanged.
MowgliNext remains the sole functional authority. The bridge translates its
existing state and blade requests into MAVROS operations.

## Public contract and ownership

No message, service, parameter or BT/GUI integration is added. Inputs remain
`/behavior_tree_node/high_level_status` (`HighLevelStatus`), `/cmd_vel`
(`TwistStamped`), `~/mower_control` (`MowerControl`), existing emergency services,
and existing MAVROS/ESC/safety observations. The node name remains hardware_bridge.
Existing QoS, remaps and telemetry interfaces are preserved.

`FcuSessionController` is only a MODE/ARM RPC sequencer. It stores the requested
mode/arming, observed `/mavros/state`, one pending transport operation and the
last transport failure. It has no functional session state machine, authorization
protocol, intention IDs, session generations or liveness watchdog. Local RPC
serials only associate callbacks with requests; callers never supply them.
`BladeControl` remains unchanged and owns servo183, OFF/FWD/REV, neutral-before-
inversion, acquired ESC stop proof, command ACKs and blade response deadlines.
All bridge MODE/ARM/DISARM writes use the sequencer.

| Existing MowgliNext input | Mode requested | ARM requested | Traction |
| --- | --- | --- | --- |
| Idle/base/no movement session | HOLD | false | neutral |
| RECORDING | MANUAL | true | after FCU confirmation |
| MANUAL_MOWING | MANUAL | true | after FCU confirmation |
| AUTONOMOUS (including return home) | GUIDED | true | after FCU confirmation |
| MowerControl ON without movement session | HOLD | true temporarily | always neutral |
| MowerControl OFF with movement session | current session mode | true | session continues |
| MowerControl OFF without movement session | HOLD | false | neutral |
| Hardware safety switch / explicit emergency service | HOLD | false, priority | neutral |
| Existing tilt or double-lift stop (including tilt latch) | retain observed FCU mode | false, priority | neutral |

GUIDED transport applies to the ArduPilot provider. PX4 retains its existing
Rover-style MANUAL mapping; native PX4 actuation compatibility is not established.
Both profiles retain their capabilities/profile/telemetry implementation.
Unsupported PX4 servo commands still fail rather than aliasing ARM/DISARM.

## Ordering and refusal handling

Normal requests are serialized: matching mode observation plus accepted mode
response, then ARM response plus matching state observation, then forwarding of
current velocity/blade commands. Already matching FCU state skips redundant
operations. A response alone is never proof of ARM. State may arrive before or
after the service response. Both traction transports are neutralized before ARM.
Velocities received while waiting are dropped;
none are stored for replay after arming.

Repeated status publications and BT ticks do not repeat identical MODE/ARM RPCs.
Each pending RPC/confirmation has a3s transport deadline. A refusal or expiry is
reported in logs and the existing `/diagnostics` topic, under
`mowgli_mavros_bridge/fcu_session`, with requested/confirmed mode and ARM,
command pending/acknowledged and target state confirmed. Emergency service true
means DISARM queued or already observed disarmed; HOLD may remain unavailable
or unconfirmed. It never confirms the complete physical stop.
The failed operation is not retried for the unchanged target. Changing the
MowgliNext target clears that failure; no extra authorization or user handshake
is required. A late actual FCU observation satisfying the target also fulfills
that operation without resending it. An obsolete request's refusal cannot block the new target.

For ordinary session end, cutting and traction are neutralized, HOLD is
requested, then DISARM. HOLD refusal, timeout or service unavailability cannot
block DISARM. The earlier safety paths are preserved: `request_blade_disarm()` (tilt/double
lift) requests DISARM while retaining the FCU mode; it does not add HOLD on
subsequent ticks. `request_hold_and_blade_disarm()` (hardware switch/explicit
emergency service) retains HOLD/DISARM. Both use the same sequencer.
Safety DISARM has higher transport priority: it supersedes any
pending MODE/ARM immediately, before waiting for HOLD. Retired RPC callbacks are
ignored; cancellation cannot retract an already emitted MAVLink command. A late
observed ARM during the existing emergency leads to one corrective DISARM;
stable repeated state does not cause a retry storm. A refused DISARM is diagnosed
and latched, not turned into a tick-based retry loop.

Reconnection and emergency release reconcile the demand still supplied by
MowgliNext. The bridge does not invent an obligation to restart an already-active
MowgliNext session. Existing safety conditions still block ARM while engaged.
The existing SafetyState projections/latches and hardware switch, lift and tilt
paths remain; their HOLD/DISARM writes share this command owner. Emergency still
cancels the cutting request through the existing blade-neutral path.

## Backend blade test through the existing service

Existing `mower_control` ON in idle requests temporary ARM in HOLD with both
traction outputs explicitly neutral. ON waits for FCU confirmation, then enters
the unchanged `BladeControl` sequence in the requested direction. Pending
callers are bounded to32 and share existing blade service timing bounds. An OFF
cancels pending ON before any pending ON can be dispatched, including ON waiting
for ARM or a reversal. Refused/expired isolated ON releases its temporary ARM
and cancels the cutting intent; a late ARM cannot resurrect it. An ON failure
during a movement session does not release that session’s ARM. OFF only releases
the temporary ARM when no MowgliNext movement session is active. If a movement
session starts during the blade test, its state owns mode/ARM thereafter.

ARM uses the existing hardware safety/latch decisions. No new GNSS, battery,
charging, ESC-stop or status-age precondition is added to ARM. BladeControl and
its existing mowing-enable, heartbeat/safety freshness, ESC acquisition and
inversion protections still determine whether the cutting command can execute.
No safety is bypassed to permit automatic arming.

ArduPilot source baseline `dbe792162d06cab66c3475fd5556bf7a120f119e`
(`APMrover2-stable`, local reviewed source) routes DO_SET_SERVO through
`libraries/GCS_MAVLink/GCS_Common.cpp` and `GCS_ServoRelay.cpp` without a HOLD
mode restriction. Rover delegates unhandled commands to the common handler in
[`GCS_MAVLink_Rover.cpp`](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/Rover/GCS_MAVLink_Rover.cpp#L438),
and [`AP_ServoRelayEvents::do_set_servo`](https://github.com/ArduPilot/ardupilot/blob/dbe792162d06cab66c3475fd5556bf7a120f119e/libraries/AP_ServoRelayEvents/AP_ServoRelayEvents.cpp#L27)
checks the channel function and applies PWM without checking the Rover mode.
The official [servo command documentation](https://ardupilot.org/dev/docs/mavlink-move-servo.html)
requires a compatible SERVOx_FUNCTION. This supports the choice of HOLD in
software; it does not prove output on this installation. There is no automatic
MANUAL fallback on a refusal: that refusal may instead indicate a channel or
safety incompatibility. Reconsider MANUAL/zero traction only if bench evidence
establishes HOLD incompatibility on the exact target.

## Velocity transport

MANUAL retains the provider's steering-Y/throttle-Z MANUAL_CONTROL conversion.
ArduRover GUIDED uses `/mavros/setpoint_raw/local`, `PositionTarget`, BODY_NED,
mask0x05E7 (velocity plus yaw rate), forward velocity in m/s and yaw rate in rad/s.
MAVROS2.16 converts ROS FLU/body velocity and positive-CCW yaw rate to MAVLink
conventions in `setpoint_raw.cpp::local_cb`. No nonzero MANUAL_CONTROL substitutes
for GUIDED. Both transports have explicit neutral commands. No command/session
watchdog or stamp rejection policy is added to the existing traction contract.
See [ArduRover Guided commands](https://ardupilot.org/dev/docs/mavlink-rover-commands.html).

## Software evidence and hardware boundary

Validation results and source/binary baselines are recorded in the retained
FCU main-commit checkpoint and the [current repository-owned validation archive](../validation/fcu-main-commit-20261011/README.md).
The [review-fixes archive](../validation/fcu-session-review-fixes-20261011/README.md) records the preceding software baseline.
The [earlier archive](../validation/fcu-session-controller-20261011/README.md) is retained as historical baseline. All tests run under UID1000 in a local pinned SDK,
network disabled, no devices, using isolated ROS domains and FCU mocks.
No SSH, hardware command, flash, FCU parameter change, deployment, production
restart, image publication, push or merge is performed. The local commit on `main` is
explicitly authorized by the user; no push is authorized.
The real GUI path currently forwards OFF at idle in MowgliNext. The required
minimal internal correction and its acceptance tests are documented in
[mowglinext-idle-blade-test-follow-up.md](mowglinext-idle-blade-test-follow-up.md).
Backend MowerControl injection does not validate an actual GUI click.

**HARDWARE_REQUIRED**: actual output in HOLD; mode/velocity sign and units;
confirmation timing and priority DISARM under wire races; complete companion-loss
failsafe. Software tests do not establish any of these physical guarantees.

Future separately authorized bench baseline must record: robot unit ID, Pixhawk
model, exact ArduRover image/hash and parameter snapshot, VESC images/channel
mapping, host OS/kernel, MAVROS revision, transport topology, bridge configuration
and binary/image hash, MowgliNext commit and selected MowgliMAVROS gitlink. Prior
Pixhawk5X/ArduRover4.7.1 observations do not validate this patch.

Bench prerequisites: blades removed, propulsion safely constrained/isolated,
qualified operator, independent physical E-stop/power cut and independent output
recorder/tachometer. Proposed procedure and observable acceptance:
1. Idle/base: actual HOLD/disarmed and neutral wheel/cutting outputs.
2. Recording/manual/autonomous: observe the requested mode and confirmed ARM
   before nonneutral output, then verify body velocity/yaw-rate sign and scale.
3. GUI FWD/REV/OFF in HOLD: zero wheel motion throughout; correct channel/direction;
   existing neutral/stop proof before inversion; OFF ends only temporary ARM.
   Refusals must remain observable without automatic MANUAL fallback.
4. Emergency during delayed/refused MODE/ARM: neutral outputs and priority DISARM;
   late responses cannot restore actuation against an active emergency. Release
   and reconnect follow the actual MowgliNext demand and existing protections.
5. Kill ROS/MAVROS/companion power and interrupt USB, in MANUAL, GUIDED and during
   cutting. An independently designed FCU-side failsafe must neutralize/disable
   both drives within the approved stop bound. Proposed initial bench bound is
   1000ms for neutral/disabled outputs, measured outside ROS; safe coast-down time
   and field stop distance still require the physical safety assessment.

Step5 remains an unresolved safety design and acceptance gate, not merely a run
waiting to happen. No production readiness is claimed. A host-side RPC timeout
or the documented GUIDED velocity timeout does not prove blade stop or MANUAL
stop after total companion loss. Shared MAVROS endpoints also do not establish
physical exclusive ownership against external command producers.
