# MowgliNext / Pepeuch MAVROS source findings

Research date: 2026-10-04. Everything below is **VERIFIED IN SOURCE** unless explicitly called inferred. No connected FC access, ROS execution, builds, tests, configuration writes, or remote repository writes were performed by this source-research agent.

## Exact current baselines

| Repository | Ref | Exact SHA | Role |
|---|---|---|---|
| `mowglinext/mowglinext` | `feat/mavros-refresh` | `b18e6c394a2bed9bd0e60880395cd3a171ff10f7` | MowgliNext integration/installer contract; latest commit authored by Pepeuch on 2026-10-03 21:28:19 UTC |
| `mowglinext/mowglinext` | `dev` | `6f37770878e4558d385320664ac737d91a8b5eea` | Current upstream development head; refresh branch is 18 commits ahead and 34 behind |
| `Pepeuch/mowglimavros` | `main` | `82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492` | Current external backend source at final review |
| `mavlink/mavros` | `2.16.0` | `5c68b905ab30de6ce630822dc46c33467e8f23ea` | Pinned by sidecar Dockerfile |
| `mowglinext/mowglinext` GNSS interface | source revision | `e789ccc2ecc249377c385e79b34fb48ff5a90927` | Mowgli fix-type interface source synchronized by current main |
| `Pepeuch/universal-gnss` | `v0.7.2-rc4` | `383caba3de94e16167764393d5a4ef046078b015` | Current Universal GNSS plugin pinned by MowgliMAVROS main |

The refresh branch no longer owns the backend source. Its [integration README](https://github.com/mowglinext/mowglinext/blob/b18e6c394a2bed9bd0e60880395cd3a171ff10f7/sensors/mavros/README.md) and [image.env](https://github.com/mowglinext/mowglinext/blob/b18e6c394a2bed9bd0e60880395cd3a171ff10f7/sensors/mavros/image.env) select `ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar:latest`. That floating image reference cannot prove which source is actually deployed. Older migration/checkpoint documents describe earlier pins and architectures; the live source above supersedes those historical details for implementation analysis. [Actual Dockerfile pins](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/Dockerfile#L5-L13).

## Current GPS path

```
GPS_RAW_INT (#24; GPS1) / GPS2_RAW (#124; GPS2)
  -> Universal GNSS MAVROS plugin
  -> /mavros/universal_gnss/gps{1,2}/status + /fix
  -> MowgliMAVROS mowgli_gnss adapter
  -> /gps/status (mowgli_interfaces/GnssStatus)
     /gps/fix    (sensor_msgs/NavSatFix)
```

`GNSS_SOURCE=mavros` activates the canonical adapter; `GNSS_MAVROS_SOURCE=gps1|gps2` chooses receiver. `GNSS_SOURCE=direct` creates no canonical endpoints in that adapter. The hardware bridge consumes `/gps/status` only for readiness; it does not consume `/mavros/global_position/global` for GPS conversion. [GNSS plugin endpoints, handlers, source selection](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_gnss_adapter/src/gnss_adapter_plugin.cpp#L39-L94).

The final source review was refreshed to MowgliMAVROS `main` at `82a1e390` and its pinned Universal GNSS `v0.7.2-rc4` (`383caba3`). The October 4 update now preserves explicit 2D/3D/DGPS solution types symbolically through Universal GNSS and Mowgli `GnssStatus`; its synthetic graph test checks MAVLink fix types 2→2D, 3→3D, and 4→DGPS. This closes the earlier generic-fix status-loss concern for those inputs, but it does not provide runtime proof for a Betaflight/INAV FC and does not change the accuracy/ellipsoid-altitude requirements described below. [Mowgli symbolic mapping](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_gnss_adapter/include/mavros_gnss_adapter/status_mapping.hpp), [synthetic MAVLink cases](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_gnss_adapter/test/test_gnss_graph.py).

Universal GNSS decodes GPS_RAW_INT/GPS2_RAW fields: `time_usec`, `fix_type`, `lat`, `lon`, MSL `alt`, `eph`, `epv`, `vel`, `cog`, satellites, `h_acc`, `v_acc`; it can enrich status with GPS_RTK (#127), GPS2_RTK (#128), SYSTEM_TIME (#2). RTK and SYSTEM_TIME are optional to basic GPS delivery. RTCM `/rtcm` is fragmented and sent as GPS_RTCM_DATA (#233), but a ROS publisher or outgoing MAVLink packet does not establish FC correction acceptance. [Conversion and handlers](https://github.com/Pepeuch/universal-gnss/blob/383caba3de94e16167764393d5a4ef046078b015/gnss_mavros/src/universal_gnss_plugin.cpp#L41-L211).

At the current Universal GNSS pin, fix types 0/1 become no fix; 2 maps to 2D; 3 to 3D; 4 to DGPS; 5/6 to RTK float/fixed; and 7/8 to generic fix. HDOP and VDOP are divided by 100, speed by 100, course by 100, coordinates by 1e7, altitude by 1000, and horizontal/vertical accuracy by 1000. Unknown accuracies (zero or UINT32_MAX) remain unavailable. Satellites from GPS_RAW_INT are represented as **satellites_visible**, not inferred satellites_used. [Exact mapping](https://github.com/Pepeuch/universal-gnss/blob/383caba3de94e16167764393d5a4ef046078b015/gnss_mavros/src/mavlink_gnss_adapter.cpp#L22-L175).

### Two concrete limitations for INAV/Betaflight MAVLink1 telemetry

1. **Covariance gates actual localization.** Canonical Universal GNSS NavSatFix populates covariance only when **both horizontal and vertical accuracy values are available**. It does not estimate accuracy from HDOP/VDOP. MAVLink1 GPS_RAW_INT contains neither h_acc nor v_acc, so canonical `/gps/fix` has UNKNOWN covariance. MowgliNext `fusion_graph` explicitly rejects UNKNOWN or zero/nonfinite covariance. Consequently valid coordinates can reach ROS/status/UI without becoming accepted localization observations. This is a source-based consequence conditional on a producer lacking those MAVLink2 extension values. [UG covariance](https://github.com/Pepeuch/universal-gnss/blob/383caba3de94e16167764393d5a4ef046078b015/gnss_ros2/src/navsat_fix_adapter.cpp#L35-L63), [fusion acceptance](https://github.com/mowglinext/mowglinext/blob/b18e6c394a2bed9bd0e60880395cd3a171ff10f7/ros2/src/fusion_graph/src/fusion_graph_node_callbacks_a.cpp#L258-L288).

2. **MSL is deliberately not mislabeled as ellipsoid height.** The canonical adapter pairs the private fix with the same GPS_RAW_INT/GPS2_RAW tuple and an on-wire nonzero `alt_ellipsoid` extension. Absent extension produces `NaN` canonical altitude. GPS_RAW_INT needs payload length at least 34 bytes for this field; GPS2_RAW needs at least 41. The adapter checks complete payload length and excludes zero altitude, so a genuine zero ellipsoid altitude is also unavailable under its current rule. Pair tolerance is 20 ms; fix delivery deliberately waits 100 ms and runs a 20 ms timer. It expires GNSS validity after 3 s. [Length check](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_gnss_adapter/include/mavros_gnss_adapter/ellipsoid_altitude.hpp#L8-L18), [pairing/delay/timeout](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_gnss_adapter/include/mavros_gnss_adapter/adapter_state.hpp#L45-L165).

Standard MAVROS `/mavros/global_position/raw/fix` is a different path: GPS_RAW_INT, including configurable DOP times UERE covariance fallback. `/mavros/global_position/global` instead requires GLOBAL_POSITION_INT (#33). Do not promise `/global` from GPS_RAW_INT alone. [Raw GPS handler](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros/src/plugins/global_position.cpp#L234-L304), [global handler](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros/src/plugins/global_position.cpp#L325-L350).

### Freshness semantics

Universal GNSS preserves `time_usec` as metadata, stamps observations with local packet receipt, and increments position_observation_sequence on **every received raw-GPS packet**. It does not deduplicate receiver measurement epochs based on time_usec. A telemetry sender retransmitting cached GPS therefore creates new transport observations even if the receiver has stalled. SYSTEM_TIME boot regression or connection change resets source incarnation. Mowgli adapter tracks sequence changes and resets validity after 3 s; hardware bridge readiness requires fresh sequence/validity within 5 s. Source epoch freshness needs separate proof when FC telemetry fields lack measurement timestamps. [Observation identity](https://github.com/Pepeuch/universal-gnss/blob/383caba3de94e16167764393d5a4ef046078b015/gnss_mavros/src/mavlink_gnss_adapter.cpp#L223-L245), [bridge readiness](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/src/readiness_state.cpp#L40-L106).

## Requirements table for integrating provider evidence

The provider column must be completed from INAV/Betaflight source and runtime captures; this agent did not access the FC.

| MowgliNext needs | ROS input / MAVLink dependency | Exact current expectation / integration consequence |
|---|---|---|
| FC connection/armed/mode | `/mavros/state` / HEARTBEAT #0 | Target system **and component** must match; HEARTBEAT updates connection, armed, guided, manual, mode, system_status. Inbound data decoding does not require a full ArduPilot command implementation. |
| Canonical GPS1 fix and status | `/gps/{fix,status}` via private UG GPS1 topics / GPS_RAW_INT #24 | Basic coordinates/fix can decode; covariance/ellipsoid limitations above prevent claiming full unchanged backend compatibility. |
| Canonical GPS2 | same adapter selected gps2 / GPS2_RAW #124 | No gps1 fallback is synthesized. Provider must emit GPS2_RAW if gps2 selected. |
| Accuracy for localization | canonical NavSatFix covariance / positive h_acc and v_acc | Both extensions needed for current UG covariance; UNKNOWN covariance rejected by fusion_graph. |
| GPS status richness | `/gps/status` capability/value flags | No fabricated quality score: quality_percent and MSM summary age are NaN; missing satellite-used/CN0/corrections/heading capabilities remain unavailable. |
| Ellipsoid altitude | matched `alt_ellipsoid` in raw GPS MAVLink2 payload | MSL altitude deliberately becomes unavailable in canonical message when ellipsoid extension absent. |
| IMU | `/mavros/imu/data` / ATTITUDE #30 or ATTITUDE_QUATERNION #31, accelerometer from IMU messages if available | ATTITUDE creates orientation and rate sample; verify numeric content, because zero angular-rate fields are not gyro measurements. RAW_IMU acceleration interpretation depends on reported autopilot type. |
| Full bridge readiness | `/mavros/state`, IMU, canonical GNSS status, canonical Power | Requires connected + fresh IMU + fresh finite traction voltage; GNSS required by default; wheel odometry optional by default. GPS transport alone cannot make full bridge ready. |
| Wheel odometry | `/wheel_odom` | Current backend no longer relabels autopilot local position as wheel odometry. Signed RPM + ESC observation counters and configured geometry underpin its optional dedicated plugin; GPS/aircraft local position is not a substitute. |
| RTCM corrections | `/rtcm` -> GPS_RTCM_DATA #233 | Outgoing transport supported by UG; FC inbound decode/receiver forwarding requires separate provider proof. |
| Generic MAVROS global position | `/mavros/global_position/global` / GLOBAL_POSITION_INT #33 | Separate fused/global topic, not mandatory for canonical raw-GPS adapter. |

[Bridge subscribed topics](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp#L72-L108), [HEARTBEAT target/state implementation](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros/src/plugins/sys_status.cpp#L918-L972), [IMU handler](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros/src/plugins/imu.cpp#L329-L371), [RAW_IMU scaling caveat](https://github.com/mavlink/mavros/blob/5c68b905ab30de6ce630822dc46c33467e8f23ea/mavros/src/plugins/imu.cpp#L520-L558).

## ArduPilot assumptions requiring changes for another FC family

- Launch accepts only `MAVROS_AUTOPILOT=ardupilot|apm|px4` and loads corresponding upstream profiles; there is no Betaflight or INAV profile. [Launch lines 35–43](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py#L35-L43).
- Defaults target system/component 1/1, VCP/UART speed 921600. These are deployment defaults, not detected provider facts. [Compose](https://github.com/mowglinext/mowglinext/blob/b18e6c394a2bed9bd0e60880395cd3a171ff10f7/install/compose/docker-compose.mavros.yml#L27-L38).
- ArduRover MANUAL_CONTROL mapping assigns steering to y and throttle to z; emergency mode defaults HOLD, and bridge creates arming/mode service clients. Those semantics cannot be inferred for INAV/Betaflight. Both drive and blade commands are disabled by default. [Command mapping](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/include/mowgli_mavros_bridge/rover_manual_control.hpp#L24-L35), [defaults](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/config/hardware_bridge_mavros.yaml).
- Full bridge power readiness currently comes from BATTERY_STATUS (#147) through battery_observer, not SYS_STATUS alone. The YAML overrides constructor battery instance defaults: configured traction id0 and dock id1. Historical checkpoint mappings differ; use current source/config. No aircraft PWM ESC investigation is required to establish this input contract. [Power message handler](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_battery_observer/src/battery_observer_plugin.cpp#L93-L129), [configured instance mapping](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_battery_observer/config/battery_observer.yaml#L4-L7).
- Wheel odometry is a separate optional ArduPilot-dialect plugin, gated and disabled without geometry/source configuration; it does not depend on LOCAL_POSITION_NED as wheel truth. [Plugin](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_esc_wheel_odometry/src/esc_wheel_odometry_plugin.cpp#L20-L63).

## Implementation recommendation from this layer

Preserve the public `/gps/fix` plus typed `/gps/status` contract and source freshness/capability truth. If provider MAVLink GPS_RAW_INT is limited to v1, an MSP adapter may carry richer navigation statistics than a unchanged MAVROS GNSS path, but its measured accuracy and altitude datum still require explicit validation. A future MAVROS profile must decouple passive GPS acceptance from ArduRover actuation/readiness assumptions. Do not estimate precision from DOP without an explicit documented calibration/error model; never label DOP as meters or MSL as WGS84 ellipsoid height. Neither INAV-derived data nor MAVROS decoding proves Betaflight hardware/GPS semantics.

## Checks and remaining lead review

Read-only GitHub branch, commit, tree, exact blob/archive and line inspection performed. Source files cached in `/tmp/mowgli-mavros-research/`; no checks requiring hardware or ROS were run. Only this research artifact was written in the investigation directory. Lead must merge verified INAV/Betaflight producer message/field lists and observed FC capture results into the table, and decide interface/rate recommendation. No repository or live hardware change resulted from this research.
