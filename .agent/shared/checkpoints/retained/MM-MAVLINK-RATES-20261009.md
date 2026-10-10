# MM-MAVLINK-RATES-20261009

Disposition RETAINED — READ-ONLY runtime audit complete, no stream changes.
MowgliMAVROS main9f62ba4b + preexisting product patch, robot192.168.10.32
rock-5b/ArduRover4.7.1dbe79216/Pixhawk5X. Sidecar788d5831… unchanged,
StartedAt2026-10-09T07:39:58.379363269Z. No commit/push.

Canonical evidence and next-step table:
docs/investigations/2026-10-07-runtime-bringup/mavlink-runtime-frequency-audit.md
and its aggregate CSV/JSON. Read these before changing rates or re-running blades.

- Retained capture60.02054s + instance/topic verification30.01992s, sys1/comp1,
  depth4096 best-effort, no observed sequence gaps, no actuator/stream commands.
  Initial depth5 capture undercounted bursts and is excluded as rate evidence.
- Most periodic messages1Hz; TIMESYNC10.097Hz. ESC11030 andRPM226 both1Hz.
  Three EscObservation streams deliver3Hz but distinct count acquisitions1Hz.
  Maximum distinct gapESC2=1.024259s versus BladeControl1000ms freshness.
- FREQ-001 OPEN: cadence/freshness margin defect, not proof of sole cause of
  previous FWD refusal. Preserve source/stamp/count identity; no timeout masking.
- FREQ-002 RETAINED: short diagnostic history undercounts multitype bursts.
  Do not mistake initial gaps for FCU outages; product raw-QoS loss not proven.
- USBttyACM0, SERIAL0_PROTOCOL2/BAUD921; stream names are MAV1/MAV2, not SRx
  on installed4.7.1. Groups1Hz, PARAMS10, ADSB0, options0.
- Battery1471Hz aggregate is0.5Hz per instance0/1. Power topic1Hz is not
  acquisition1Hz per battery. PluginVIBRATION disabled although241 received.
- Exact MAVROS commit5c68b905… sources inspected; interval511 is explicit
  service path, no auto startup path identified. No511/66 observed in captures;
  historical startup commands not exhaustively captured, no restart to obtain it.
- Proposals only: ESC11030 10Hz, servo36 10Hz, SYS_STATUS5Hz, ATTITUDE/RAW_IMU
  50Hz, RPM22620Hz if retained wheel source. Missing messages not enabled blindly.
  Use per-message511 under one owner, qualify support/count progression/gaps
  passively before motors. No intervals have been sent or parameters written.

Final during captures: connected/disarmed/MANUAL, rc/out1500x3, bridge active.
FWD/REV acceptance remains incomplete; total-companion-loss failsafe unresolved.
Invalidate measured rates after stream/firmware/link/consumer-baseline changes.
Next requires operator decision on the individual-interval plan; no auto change.
