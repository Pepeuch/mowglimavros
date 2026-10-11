# MowgliNext blade policy — integration reference

Status: historical idle-ON proposal SUPERSEDED by the user decision of 2026-10-11.
MowgliNext retains its historical blade policy: an idle direction click may forward OFF;
the backend must obey the existing commands and sequence MODE/ARM when appropriate.
No BT/GUI correction, new service or authorization is required by MAVROS integration.

The original detailed GUI/BladeDirection investigation was transferred byte for byte
to its owner: [Next historical analysis](../../../../../../docs/validation/gui-idle-blade-20261011/mavros-origin-idle-blade-analysis.md).
The policy decision and regression evidence are [in Next](../../../../../../docs/validation/gui-idle-blade-rollback-20261011/README.md).
Backend physical gates remain in [fcu-session-controller.md](fcu-session-controller.md).
No code, ROS contract, safety policy or acceptance status changed.
