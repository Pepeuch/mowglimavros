# Imported physical acceptance section — historical

Original Next docs/validation/gui-idle-blade-20261011/README.md; source SHA256 24f29e6c8c93389d032c4a18cf95985bc075e3416a21c80cda204c541bd785fe.
Source execution baseline: Next7748af56acd98449938750714d2286193cf6143a, backend8eecc57.
No physical acceptance was acquired during this reorganization.
WITHDRAWN: GUI idle-ON is not a requirement; this old procedure remains only as rejected-design evidence.
Current backend gates: [FCU architecture](../../architecture/fcu-session-controller.md).

## Preserved original section

<!-- source: MowgliNext:docs/validation/gui-idle-blade-20261011/README.md; lines 94-111; sha256 cbf2467cddee91e0bf0f23ae5d7e4b3779b50ba770fa7364aa93a4967188388d -->

## Hardware acceptance

HARDWARE_REQUIRED, no production readiness claim. Software baseline is the
MowgliNext worktree hash manifest with MowgliMAVROS gitlink8eecc57; no physical
run on this baseline has occurred. Before an authorized bench run, record robot
unit, Pixhawk model/image/hash/parameter snapshot, VESC images and servo mapping,
MAVROS revision, OS/kernel, topology, configuration and installed binary hashes.
Blades removed, propulsion isolated/constrained, qualified operator, independent
physical emergency power cut and independent output/tachometer recording.

Run actual GUI FWD/REV/OFF at normal idle: observe HOLD and confirmed ARM before
cutting output; pass only if wheel output/motion stays zero and cutting direction
matches. Several complete idle cycles must preserve the test; OFF/STOP/guard
must neutralize cutting, and OFF must DISARM only a temporary idle session.
Separately verify MANUAL/GUIDED session behavior and physical reversal/stop
proof on the exact installation. Independent stopping after total companion /
ROS / MAVROS loss remains unresolved in the FCU adapter acceptance plan and
must be established separately; this patch does not supply that guarantee.
