# MM-GNSS-CANONICAL-ADAPTER-20261003

Disposition: ACTIVE — Lyrical software acceptance complete; only ARM64/real target runtime acceptance remains in TODO MM-603.
Repository: Pepeuch/mowglimavros, `/workspaces/mowglimavros`.
Branch: `main`; initial implementation baseline `6fd9d3b31c9df124e6d92428552484b57fe6b854`.
Initial canonical-adapter implementation HEAD:
`a1fe22c11171b0074a6a1771e249a0bba6c6c1b9`
(`feat(gnss): add canonical MAVROS GNSS adapter`).
The 2026-10-04 follow-up refreshes Universal GNSS to `v0.7.2-rc4`
(`383caba3de94e16167764393d5a4ef046078b015`), synchronizes the Mowgli GNSS
interface from MowgliNext `e789ccc2ecc249377c385e79b34fb48ff5a90927`,
and preserves explicit 2D/3D/DGPS solution types.
Origin is `https://github.com/Pepeuch/mowglimavros.git`; no submodules.
No deployment or robot-local configuration change was performed.

## Scope and ownership

User authorized only MowgliMAVROS's MAVROS layer/plugins. No MowgliNext, GUI,
installer or robot-local configuration changes. No robot connection, hardware
command, restart or deployment was performed. No credentials are recorded here.

New package `ros2/src/mavros_gnss_adapter` exports MAVROS plugin `mowgli_gnss`.
It subscribes to selected private Universal GNSS status/fix and solely owns
canonical `/gps/status` and `/gps/fix` in `GNSS_SOURCE=mavros`.
`GNSS_MAVROS_SOURCE=gps1|gps2` selects the receiver; `direct` creates no canonical
publishers or GNSS subscriptions/handlers. The launch explicitly passes these
environment values to MAVROS. Without GNSS_SOURCE, standalone plugin defaults
to direct. `MAVROS_GPS1_CANONICAL` is preserved as a deprecated consistency
check, not an activation switch, to avoid breaking existing deployment contracts.

Bridge source/header no longer contain GPS raw/projection state, publishers,
`gps1_canonical_enabled`, `on_serial_gps_raw`, or `publish_gps_stale`.
`serial_gps_projection.hpp` and its tests are removed. `on_gnss_status` and the
`/gps/status` readiness subscription are unchanged. The 2026-10-04 rc4 follow-up
extends the public Mowgli `GnssStatus` enum append-only with `FIX_TYPE_2D_FIX=5`,
`FIX_TYPE_3D_FIX=6`, and `FIX_TYPE_DGPS=7`.

## Established contracts — do not rediscover

- Repository UG pin is now `383caba3de94e16167764393d5a4ef046078b015`
  (`v0.7.2-rc4`); MAVROS remains pinned to 2.16.0.
  At this UG pin the C++ `universal_gnss_ros2::msg::GnssStatus` compatibility alias
  resolves to generated `universal_gnss_msgs/msg/GnssStatus`; Python imports the
  latter. The installed old runtime image's `universal_gnss_ros2` Python package
  is not evidence for this pin.
- Upstream `gnss_mavros/src/universal_gnss_plugin.cpp::ConvertRawCommon` supplies
  MSL altitude to private fixes; status stamp is local receipt and RTK can
  advance it without advancing position sequence. Upstream owns all GNSS/RTK
  mapping. New `status_mapping.hpp` maps fix/RTK/baseline enums and capability
  bits symbolically, preserving available values and observation sequence.
- Public Mowgli GnssStatus has no source_id/incarnation fields. These are used
  internally for replacement invalidation; backend is mavros_gps1/gps2. No
  assertion that absent public fields are preserved on the wire.
- Adapter retains position stamp on same-sequence enrichment and expires after
  three steady-clock seconds without a new position. Stale/no-fix never creates
  a new sequence or synthetic position. Missing NTRIP/MSM provenance/capabilities
  remain unknown/unset for downstream diagnostics enrichment; UI quality NaN.
- Only selected raw GPS1/GPS2 handler in this plugin extracts ellipsoid height
  and pairing tuple/time. The fix still comes from UG. Unique matching
  lat/lon/MSL tuple within 20 ms, bounded queue (32), 100 ms delivery wait;
  missing/ambiguous height => NaN, never MSL or previous height. MAVLink extension
  offsets 30 (GPS_RAW_INT) / 37 (GPS2_RAW) are verified against installed common
  headers. All-zero/truncated extensions cannot prove a height and fail unknown.
- See package README for QoS, frame, limitations and exact behavior; TODO MM-603
  is the remaining work queue. Historical 2026-09-29 checkpoint describes the
  deployed legacy image, not this new source implementation.

## Current acceptance policy

ROS 2 Lyrical fully replaces Kilted as the current primary/runtime baseline for
MowgliMAVROS and MowgliNext. The established Lyrical software validation below
satisfies current distro acceptance. Kilted is not a supported acceptance target
for this work and requires no revalidation. Humble support is planned only after
the repository/refactor policy is completed and is outside this task.
Remaining acceptance is limited to ARM64 and real target runtime validation in
TODO MM-603. No Kilted revalidation is required; the 2026-10-04 rc4 changes
were revalidated on Lyrical as recorded below.

## Validation

PASS on native Lyrical amd64:

- Native MAVROS package reports 2.16.0. Exact UG source pin downloaded read-only
  to `/tmp/mm-gnss-dependency`; built `universal_gnss_msgs`, `universal_gnss_ros2`,
  `universal_gnss_mavros` and current `mowgli_interfaces`, without modifying the
  external repository. Build/install/log under `/tmp/mm-gnss-build`.
- Initial affected-suite validation: adapter and bridge builds; adapter compiled
  with `-Wall -Wextra -Wpedantic -Werror`. 11 adapter gtests plus 26 existing
  bridge gtests. `colcon test-result` reported 45 tests with zero
  errors/failures/skips (includes CTest/result wrappers).
- Ellipsoid field-availability follow-up: native Lyrical adapter rebuild PASS;
  13 adapter gtests PASS. GPS1/GPS2/direct isolated graph scenarios PASS with
  payload lengths offset through offset+4, complete-zero => NaN, negative
  ellipsoid height preserved, and recovery on the next valid observation.
- Real pluginlib loading + synthetic MAVLink via `/uas1/mavlink_source` in four
  separate localhost-only domains: mavros/gps1, mavros/gps2, direct/gps1,
  direct/gps2. One canonical publisher per topic only in mavros mode; bridge
  remains subscriber. Rich fields/RTK, sequence preservation, unselected source
  isolation, RTK-only stale timeout, no-fix recovery, 230/330 m ellipsoid versus
  180/280 m MSL, and missing extension => NaN. No FCU or serial device.
- Three launch tests cover source/mode propagation, RTCM gating, optional legacy
  flag, coherent/conflicting legacy values and invalid selections.
- Nine external contract tests, three interface-lock tests and MM-602 interface
  fingerprint check. Python syntax, package/plugin XML parsing, diff whitespace.

The graph script is installed by the adapter and registered as `test_gnss_graph`
in bridge CTest. The launch test is also registered in bridge CTest. Commands:

```sh
source /opt/ros/lyrical/setup.bash
source /opt/mowgli/mavros/setup.bash
source /tmp/mm-gnss-build/install/setup.bash
colcon --log-base /tmp/mm-gnss-build/log test \
  --base-paths ros2/src/mavros_gnss_adapter ros2/src/mowgli_mavros_bridge \
  --build-base /tmp/mm-gnss-build/build \
  --install-base /tmp/mm-gnss-build/install \
  --packages-select mavros_gnss_adapter mowgli_mavros_bridge
colcon test-result --test-result-base /tmp/mm-gnss-build/build --verbose
```

## 2026-10-04 explicit solution-type refresh

- Universal GNSS exact source: `383caba3de94e16167764393d5a4ef046078b015`
  (`v0.7.2-rc4`).
- Mowgli interface source revision:
  `e789ccc2ecc249377c385e79b34fb48ff5a90927`.
- Public Mowgli fix types append 2D/3D/DGPS without renumbering existing values.
- Mapping remains symbolic: UG uses 2D/3D/DGPS values 6/7/8 while Mowgli uses
  5/6/7, so numeric casts are intentionally forbidden.
- Static validation PASS: three MM-602 interface-contract tests, nine external
  backend-contract tests, graph-test Python syntax and `git diff --check`.
- Native Lyrical build against the exact rc4 source PASS.
- Adapter + bridge validation: 48 tests, zero errors/failures/skips.
- Targeted registered `test_gnss_graph`: 1/1 PASS in 29.69 s. The graph now
  verifies MAVLink fix type 2 -> canonical 2D, 3 -> 3D and 4 -> DGPS in addition
  to the existing GPS1/GPS2/direct ownership, RTK, freshness and altitude cases.
- GitHub checkout actions were advanced from v4 to v7.
- No FCU, robot, deployment or physical actuator was involved.

## Pending acceptance / exact next step

Build the target Lyrical ARM64 image and, in a separately authorized deployment,
validate selected-source delivery, ellipsoid pairing under real load, RTK and
external NTRIP/MSM enrichment, stale/no-fix recovery, FCU disconnect/reconnect/reboot
and canonical DDS delivery. These are the only remaining acceptance gates for
MM-603. Existing robot runtime remains untouched by this work. Do not claim
hardware or release readiness from the synthetic software tests; do not change
robot-local configuration in this documentation task.
