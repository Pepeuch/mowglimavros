# Canonical Mowgli GNSS adapter

`mowgli_gnss` is an external MAVROS plugin in MowgliMAVROS. It consumes the
selected private `/mavros/universal_gnss/gps{1,2}/status` and `/fix` outputs.
Universal GNSS remains responsible for MAVLink fix/RTK decoding and observation
identity. This adapter publishes one reliable, volatile, depth-10 `/gps/status`
(`mowgli_interfaces/msg/GnssStatus`) and `/gps/fix` (`sensor_msgs/msg/NavSatFix`).
The pinned `universal_gnss_ros2::msg::GnssStatus` C++ type is a compatibility
alias for the generated `universal_gnss_msgs/msg/GnssStatus` ROS interface.
Inputs use SensorDataQoS, matching Universal GNSS's best-effort publishers.
Canonical headers use `gps_link`, preserving the former bridge output frame.

## Selection and ownership

The repository launch sets `GNSS_SOURCE=mavros|direct` (default `mavros`) and
`GNSS_MAVROS_SOURCE=gps1|gps2` (default `gps1`) in the MAVROS process environment.
`direct` creates no canonical publishers, private GNSS subscriptions or raw
altitude handlers in this plugin. A standalone MAVROS process with no
`GNSS_SOURCE` also defaults to disabled (`direct`). Invalid values fail plugin
construction; the launch validates them before starting MAVROS.

`MAVROS_GPS1_CANONICAL` remains a deprecated launch consistency guard only:
if supplied, it must equal `(GNSS_SOURCE == mavros && GNSS_MAVROS_SOURCE == gps1)`.
It neither enables nor disables any publisher. Existing deployments may omit
it; GPS2 canonical ownership works without adding a second compatibility flag.

The hardware bridge only subscribes to `/gps/status` for readiness. Do not
remap Universal GNSS's private status to `/gps/status`: its type and enum/flag
values differ. No external direct-GNSS owner is started or stopped here.

## Status, corrections and freshness

Fix, RTK, baseline enums and every supported capability/value bit are mapped
symbolically. Rich fields are retained, including differential corrections,
corrections-active, dilution, accuracy, satellites, heading, baseline and CN0.
Unknown enum values fail to no-fix/unknown; unknown capability bits are dropped.
`position_observation_sequence` is copied unchanged, including on stale output.
The public Mowgli IDL has no `source_id` or `source_incarnation`: these are used
internally to invalidate queued data on source replacement, without changing
that IDL. `backend` identifies `mavros_gps1` or `mavros_gps2`.

Upstream status stamps may advance on RTK-only updates. The canonical header
retains the last position stamp, and a steady-clock three-second deadline
advances only on a new sequence. Cached or RTK-only updates cannot keep a
position alive. Timeout publishes an invalid/no-fix status with cleared value
flags, unchanged sequence/stamp, and no synthetic fix. Explicit upstream
no-fix/disconnect clears pending fixes. A new source incarnation resets local
pairing/freshness bookkeeping, not the upstream sequence.

No NTRIP/MSM/correction transport/flow/semantic truth or UI quality score is
invented. These capabilities remain unset, correction enums unknown, provenance
strings empty, and the absent quality score NaN. Existing downstream diagnostics
consumers can continue enriching those fields from their NTRIP/MSM diagnostics.
This plugin does not start an NTRIP client or become a second diagnostics owner.

## Ellipsoidal altitude, never a silent MSL remap

At the pinned Universal GNSS commit `6f0eb09ff48893ad56c70956266f19e5a775552c`,
`ConvertRawCommon()` forwards `GPS_RAW_INT.alt` / `GPS2_RAW.alt` (MSL) to the
private NavSatFix. The canonical Mowgli contract instead uses WGS84 ellipsoidal
height, previously supplied by the bridge's GPS1 `alt_ellipsoid` projection.
On the test robot the reported example is roughly 180 m MSL versus 230 m
ellipsoidal. A simple remap would therefore change the public meaning.

Only this adapter retains selected-receiver MAVLink raw handlers, filtered by
MAVROS `SystemAndOk`. They extract `alt_ellipsoid` plus lat/lon/MSL altitude and
local receipt time for association; they do not interpret fix, RTK, dilution,
accuracy or satellites. GPS1 uses the extension at offset 30; GPS2 at offset 37
(after yaw), per MAVLink common. All four bytes of `alt_ellipsoid` must be
present (`payload length >= offset + sizeof(alt_ellipsoid)`). An absent or
partially truncated field is unknown. A zero `alt_ellipsoid` also remains unknown,
even when the payload includes the complete field or later extensions: zero
cannot prove that the extension was populated. These cases yield NaN, never an
assumed zero-metre height.

The private fix must belong to an accepted Universal GNSS position stamp.
Delivery waits 100 ms to tolerate callback ordering. An altitude is accepted
only for a unique matching lat/lon/MSL tuple within 20 ms of the private fix's
local receipt stamp, and is consumed once. Ambiguous, absent or unmatched
altitude becomes NaN; latitude/longitude/covariance remain from Universal GNSS.
No geoid offset, previous height, or MSL fallback is used. Queues are bounded
at 32 entries. The timing window is deliberately conservative: overloaded
runtime dispatch can produce unknown altitude, not a mismatched confident
height. Bench validation of pairing under target load remains necessary.

## Validation

`test_gnss_adapter` exercises selection, symbolic mapping, observation identity,
no-fix/stale and ellipsoid pairing. `test_gnss_graph.py` drives isolated MAVROS
with synthetic MAVLink (no FCU/robot) and verifies real plugin loading, GPS1 and
GPS2 ownership, bridge subscription, direct-mode absence, and rich/altitude data.
The graph test is registered in the bridge's CMake tests (which also builds the
bridge executable); the adapter unit tests belong to its own package. Run
`colcon test --packages-select mavros_gnss_adapter mowgli_mavros_bridge` after
building and sourcing the workspace. Tests use the repository-pinned MAVROS
and Universal GNSS dependencies.
