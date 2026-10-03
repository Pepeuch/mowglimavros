# MM-GNSS-CANONICAL-ADAPTER-20261003

Disposition: ACTIVE — implementation complete; Lyrical software validation PASS; ARM64/target acceptance remains in TODO MM-603.
Repository: Pepeuch/mowglimavros, `/workspaces/mowglimavros`.
Branch: `main`; baseline/unchanged HEAD `6fd9d3b31c9df124e6d92428552484b57fe6b854`.
Initially clean worktree; origin `https://github.com/Pepeuch/mowglimavros.git`; no submodules.
Changes remain unstaged/uncommitted; no push or deployment.

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
`/gps/status` readiness subscription are unchanged. No public IDL was modified.

## Established contracts — do not rediscover

- Repository UG pin `6f0eb09ff48893ad56c70956266f19e5a775552c`; MAVROS pin 2.16.0.
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

## Pending acceptance / exact next step

Review the uncommitted diff. Lyrical is the current supported acceptance target;
Kilted is historical and requires no further validation for MM-603. Humble support
is deferred until the planned repository refactor. No new broad/multiarch image
build was performed. Then build the target Lyrical ARM64 image and, in a separately
authorized deployment,
validate selected-source delivery, ellipsoid pairing under real load, RTK and
external NTRIP/MSM enrichment, FCU disconnect/reconnect/reboot. Existing robot
runtime remains untouched. Do not claim hardware or release readiness from the
synthetic software tests; do not change robot-local configuration in this task.
