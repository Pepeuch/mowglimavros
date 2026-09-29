# External MowgliMAVROS sidecar integration

This directory is MowgliNext's integration contract for the externally built
MowgliMAVROS sidecar. It intentionally contains no Dockerfile and no copy of
the MowgliMAVROS source: the external image remains the runtime owner of
MAVROS, its hardware bridge, battery observation, and ESC wheel odometry.
The Universal GNSS sidecar is the sole owner of the receiver and NTRIP.

## Image and compose boundary

`image.env` is the authoritative MowgliNext default for the external image:

```text
ghcr.io/pepeuch/mowglimavros/mowgli-mavros-sidecar:kilted@sha256:04e4eb17b0f5ce38f882f68346b1694774fa87e1945b38b57c94f90da34dd560
```

`install/lib/config.sh` sources that file, then writes `MAVROS_IMAGE` to the
generated `docker/.env`. `install/compose/docker-compose.mavros.yml` remains
the installer-owned compose fragment because the installer merges it into the
generated stack. It supplies host networking, `/dev:/dev`, the read-only
derived `docker/config/mavros` mount at `/ros2_ws/config`, the Cyclone DDS
mount, and `MAVROS_PORT`, `MAVROS_BAUD`, autopilot, GCS, and
target-system/component inputs. MowgliNext derives that YAML from the
operator-facing config and overrides only `ntrip_enabled: false`.

Use `MAVROS_BY_ID=/dev/serial/by-id/<stable-device>` when a stable path is
available. The installer makes that path the `MAVROS_PORT`; `/dev/mavros` is
only the compatibility fallback. No machine-specific USB identity belongs in
this repository.

## Backend ownership and fail-closed behavior

`HARDWARE_BACKEND=mavros` selects exactly one external `mowgli-mavros`
sidecar alongside the independent Universal GNSS sidecar when GNSS is enabled.
MAVROS never receives the receiver or NTRIP configuration as an owner. In the
main MowgliNext launch, only the legacy `mowgli_hardware/hardware_bridge_node` is excluded;
`robot_state_publisher` and `twist_mux` remain running. The external sidecar
is expected to own `mavros_hardware_bridge_node`.

The sidecar's expected integration surfaces include `/mavros/state`,
`/mavros/global_position/global`, the backend's public GNSS contract, and its
hardware bridge surfaces. A running container is not readiness evidence. The
external image must keep command and blade paths disabled by default;
MowgliNext passes no command- or blade-enable override. Hardware acceptance is
`HARDWARE_PENDING` and this integration does not alter ArduPilot, motion, or
blade behavior.

The published image above is the current deployment baseline. MowgliMAVROS has
not yet been repinned and republished against Universal GNSS dev
`ef31c4c95a8e831d2c926a5557298806505d52a7`; this directory does not claim
otherwise. Updating that external repository and replacing this digest is a
separate follow-up.

## Validation

Run the source-level contract check without Docker or external image builds:

```bash
python3 sensors/mavros/test_integration.py
```

It validates the image pin, installer/compose mapping, by-id support, launch
ownership boundaries, and that no local command/blade enable override or
machine-specific Pixhawk identity has been introduced.

### Rock 5B / Pixhawk 5X checkpoint — 2026-09-21

This is a read-only, non-actuation checkpoint from the `feat/mavros-refresh`
checkout on the Rock 5B. It is evidence for that host, image set, Pixhawk 5X,
and receiver state only; it is not a general hardware acceptance result.

#### Environment and transport

| Item | Observed value |
| --- | --- |
| Host | Rock 5B, `aarch64` / arm64 |
| OS / kernel | Armbian 26.8.3, Debian 13 (Trixie), `7.1.8-edge-rockchip64` |
| Flight controller | Holybro Pixhawk 5X detected over USB |
| MAVROS serial path | Stable `/dev/serial/by-id/<Pixhawk-5X-device>-if00` (`ttyACM0`) |
| MAVROS baud rate | `921600` |
| Sidecar ROS / RMW | ROS 2 Kilted, `rmw_cyclonedds_cpp` |

The Pixhawk serial endpoint opened successfully, MAVROS received an ArduPilot
heartbeat, and both `mavros_node` and `mavros_hardware_bridge_node` remained
running. `/mavros/state` reported `connected=true`, `armed=false`, and
`mode=MANUAL`.

#### CycloneDDS deployment fix

The sidecar initially restarted because CycloneDDS 0.10.5 rejects the
`Peer@PruneDelay="inf"` attribute:

```text
config: //CycloneDDS/Domain/Discovery/Peers/Peer: PruneDelay: unknown attribute
RCLError: failed to initialize rcl node
```

Removing `PruneDelay` from both deployment copies,
`install/config/cyclonedds.xml` and `docker/config/cyclonedds.xml`, fixed the
startup failure. The `mowgli-mavros` sidecar subsequently remained stable.

#### Passive telemetry result

“Received” below means that one message was observed during this checkpoint.
“Present without data” means that ROS graph discovery found the topic and its
publisher, but no message arrived in the bounded observation window. It does
not identify a receiver or FCU fault by itself.

| Flow | Topic / type | Publisher / probable source | Received | Status |
| --- | --- | --- | --- | --- |
| FCU state | `/mavros/state` — `mavros_msgs/msg/State` | MAVROS state plugin / FCU heartbeat | Yes | **OK**; connected, disarmed, MANUAL |
| IMU | `/mavros/imu/data_raw` — `sensor_msgs/msg/Imu` | MAVROS IMU plugin / Pixhawk IMU | Yes | **OK**; passive IMU values received |
| GPS | `/mavros/global_position/global` — `sensor_msgs/msg/NavSatFix` | MAVROS global-position plugin / FCU GNSS estimate | Yes | **HARDWARE_PENDING**; message carried no valid fix (`status=-1`) |
| Battery | `/mavros/battery` — `sensor_msgs/msg/BatteryState` | MAVROS battery plugin / `BATTERY_STATUS` | Yes | **HARDWARE_PENDING**; voltage/current were not usable |
| Battery observer | `/mavros/battery_observer/status` — `mavros_battery_observer/msg/BatteryStatus` | MAVROS battery observer | Yes | **HARDWARE_PENDING**; `voltage_available=false` |
| ESC status | `/mavros/esc_status/status` — `mavros_msgs/msg/ESCStatus` | MAVROS ESC status plugin / FCU | No | **Present without data**; one publisher, no message in 6 s |
| ESC telemetry | `/mavros/esc_telemetry/telemetry` — `mavros_msgs/msg/ESCTelemetry` | MAVROS ESC telemetry plugin / FCU | No | **Present without data**; one publisher, no message in 6 s |
| Wheel odometry | `/wheel_odom` — `nav_msgs/msg/Odometry` | Expected MAVROS ESC-wheel-odometry adapter | No MAVROS result | **HARDWARE_PENDING**; the adapter was disabled because the left/right ESC, RPM, and wheel-geometry mapping is not yet valid. During this session the legacy bridge also published this compatibility topic, so it cannot be attributed to MAVROS. |

#### Hardware bridge and deployment state

The MAVROS bridge was active with the fail-closed settings
`manual_control_enabled=false`, `blade_control_enabled=false`, and unassigned
dock/traction battery instance IDs. No audit command called MAVROS control
services, published `cmd_vel`, armed the FCU, changed mode, commanded motors,
or commanded the blade. ArduPilot parameters were not modified.

A deployment defect remains separate from the sidecar result: two nodes named
`/hardware_bridge` were discovered. The MAVROS bridge is
`mavros_hardware_bridge_node` in `mowgli-mavros`; the legacy STM32
`mowgli_hardware/hardware_bridge_node` is in `mowgli-ros2`. Consequently the
legacy deployment had two hardware consumers of `/cmd_vel` and two publishers
of `/imu/data`, `/hardware_bridge/status`, `/hardware_bridge/power`, and
`/battery_state`.

`HARDWARE_BACKEND=mavros` was present in the Compose service, but the deployed
`mowgli-ros2` image predated the conditional launch. The source on
`feat/mavros-refresh` contains the required `hardware_backend` condition. A
test that copied only the two newer launch files into the old image failed
before node startup because its installed `robot_config_util.py` lacked the
new `dig_proposal_radius` helper. Both launch files were restored from
in-container backups; no production image was replaced. The permanent fix is
therefore either a `mowgli-ros2` rebuild from this branch or a separately
reviewed, compatibility-preserving backport of only the backend condition.

#### Remaining hardware acceptance (`HARDWARE_PENDING`)

- Verify POWER1 / POWER2 wiring and map their `BATTERY_STATUS` instance IDs.
- Verify ESC telemetry availability, left/right RPM identity, and the
  ESC-wheel-odometry mapping before enabling that adapter.
- Obtain a valid GPS fix and validate its quality / update cadence.
- Validate IMU orientation and cadence against the vehicle frame.
- Evaluate any later VESC or DroneCAN integration as a separate baseline.

### Rock 5B / Pixhawk / ArduPilot passive follow-up — 2026-09-22

This follow-up was performed over SSH as `pepeuch@192.168.10.32` on the
running Rock 5B stack (`mowgli-ros2`, `mowgli-mavros`, `mowgli-lidar`, and
`mowgli-gui` up).  It used only ROS graph inspection, bounded topic
subscriptions, and read-only container-log inspection.  It did not invoke a
service, change a parameter or mode, arm, publish a command, or command a
motor or blade.  The final `/mavros/state` sample remained
`connected=true`, `armed=false`, `mode=MANUAL`, `system_status=4`.

`OK` below means that the stated ROS telemetry was observed, not that the
physical subsystem has completed vehicle acceptance. `PRESENT_NO_DATA` means
the graph exposed the topic and its publisher but the bounded sample received
no message. Values are valid only for this host, the running images, the
connected Pixhawk, and this stationary observation.

| Flow | Topic / type | Publisher | Received / cadence | Useful observed values | Status |
| --- | --- | --- | --- | --- | --- |
| FCU | `/mavros/state` — `mavros_msgs/msg/State` | `/mavros/sys` | Yes; 1 Hz | connected, disarmed, MANUAL, manual input enabled, system status 4 | **OK** |
| Battery `BATTERY_STATUS` | `/mavros/battery` — `sensor_msgs/msg/BatteryState` | `/mavros/sys` | Yes; 2 Hz aggregate | Alternating `location=id1`: 28.35–28.37 V, -0.36 to -0.41 A, 97%; `location=id0`: 28.85–28.86 V, -0.74 to -0.75 A, 95% | **CONFIG_PENDING** |
| Battery observer | `/mavros/battery_observer/status` — `mavros_battery_observer/msg/BatteryStatus` | `/mavros/battery_observer` | Yes; 2 Hz aggregate | Matching `id=1`: 28.35–28.37 V, +0.36–0.41 A, 97%; `id=0`: 28.85–28.86 V, +0.74–0.75 A, 95%; availability flags true | **CONFIG_PENDING** |
| ESC status | `/mavros/esc_status/status` — `mavros_msgs/msg/ESCStatus` | `/mavros/esc_status` | No; no message in 5 s | No ESC count, index, RPM, temperature, voltage, or current exposed | **PRESENT_NO_DATA** |
| ESC info | `/mavros/esc_status/info` — `mavros_msgs/msg/ESCInfo` | `/mavros/esc_status` | No; no message in 5 s | No identity or count exposed | **PRESENT_NO_DATA** |
| ESC telemetry | `/mavros/esc_telemetry/telemetry` — `mavros_msgs/msg/ESCTelemetry` | `/mavros/esc_telemetry` | No; no message in 5 s | No RPM, temperature, voltage, or current exposed | **PRESENT_NO_DATA** |
| Wheel odometry | `/wheel_odom` — `nav_msgs/msg/Odometry` | `/hardware_bridge` | No message in 5 s | The sole visible publisher is not a MAVROS topic/plugin. With two same-named `/hardware_bridge` nodes present, this is legacy/ambiguous rather than evidence of ESC-wheel odometry. | **HARDWARE_PENDING** |
| MAVROS global GPS | `/mavros/global_position/global` — `sensor_msgs/msg/NavSatFix` | `/mavros/global_position` | Yes; 2 Hz | `status=-1`, latitude/longitude 0, covariance unknown; no valid fix | **HARDWARE_PENDING** |
| Public GPS fix | `/gps/fix` — `sensor_msgs/msg/NavSatFix` | none | No; graph publisher count 0 | No public fix is being produced | **CONFIG_PENDING** |
| Public GPS status | `/gps/status` — `mowgli_interfaces/msg/GnssStatus` | `/universal_gnss_topic_bridge` | Publisher present | The public bridge exists, but cannot supply a valid fix while the MAVLink GPS inputs remain invalid | **CONFIG_PENDING** |
| FCU GPS 1 / 2 | `/mavros/gpsstatus/gps{1,2}/raw` — `mavros_msgs/msg/GPSRAW`; `/mavros/universal_gnss/gps{1,2}/status` — `universal_gnss_ros2/msg/GnssStatus` | `/mavros/gpsstatus`, `/mavros/universal_gnss` | Yes; status topics 2 Hz each | Both report `fix_type=0`, zero satellites and invalid/NaN position; sources are `mavlink:fcu:gps1` and `mavlink:fcu:gps2` | **HARDWARE_PENDING** |
| IMU raw | `/mavros/imu/data_raw` — `sensor_msgs/msg/Imu` | `/mavros/imu` | Yes; ~2 Hz | `frame_id=base_link`; gyro about (-0.002, -0.008, -0.003) rad/s; acceleration about (-0.10, -0.34, 9.83) m/s² at rest; raw orientation unavailable | **OK** |
| IMU fused | `/mavros/imu/data` — `sensor_msgs/msg/Imu` | `/mavros/imu` | Yes; ~4 Hz | `frame_id=base_link`; gyro near zero; acceleration about (-0.10, -0.35, 9.82) m/s²; non-identity attitude published | **OK** |

#### Battery identity and CAN/DroneCAN limits

Two battery instance IDs, `0` and `1`, are now proven. Their voltage, current,
and remaining values agree across the MAVROS `BatteryState.location="idN"`
and the observer `id=N` surfaces. This observation does **not** identify either
one as POWER1/dock/charger or POWER2/traction. The opposite current signs in
the two exposed message families are recorded as-is; they are not evidence of
a charge/discharge mapping. Separately, `/mavros/sys_status` reported 28.875 V,
7.3 A, and 94% remaining; its relationship to either instance is likewise
unproven. Keep both dock and traction instance mappings
unassigned until the physical wiring and the relevant ArduPilot battery
configuration are independently evidenced.

No MAVLink surface observed in this session supplied a DroneCAN device list or
identified a HERE4/GNSS-CAN peripheral. MAVROS exposes two FCU GPS slots, but
both have zero satellites and invalid fixes; that is not evidence of HERE4 or
of CAN enumeration. No ArduPilot CAN, GPS, battery, ESC, or servo parameter
was read or changed. If a physically connected ESC or CAN GNSS remains absent
after wiring verification, its required ArduPilot configuration must be
identified from the exact controller and firmware baseline before changing it;
this session establishes neither a device identity nor a safe parameter set.

### ArduPilot read-only topology audit — 2026-09-22

The physical topology supplied for this audit is: Pixhawk 5X; three VESCs on
CAN1 (left wheel, right wheel, mower); u-blox F9P on the Pixhawk GPS2 port;
POWER1 is the charger/dock input and POWER2 is the 7S traction input. It is a
physical-wiring statement, not a claim that a logical ESC index identifies a
particular motor.

The MAVROS parameter cache was refreshed with one `PARAM_REQUEST_LIST`
(`ParamPull(force_pull=true)`) and received 1,014 parameters. This is a
read-only MAVLink operation. No `PARAM_SET`, mode change, arming, motor/blade
command, or `cmd_vel` publication occurred.

| Subsystem | Physical wiring | Current ArduPilot configuration | MAVLink state | Probable issue | Proposed correction |
| --- | --- | --- | --- | --- | --- |
| CAN1 / VESCs | Three VESCs: left wheel, right wheel, mower | `CAN_P1_DRIVER=1`, `CAN_P1_BITRATE=500000`; driver 1 is `CAN_D1_PROTOCOL=1` (DroneCAN); `CAN_D1_UC_ESC_BM=7`, `CAN_D1_UC_ESC_OF=0`, `CAN_D1_UC_ESC_RV=7`; autopilot DroneCAN node ID `CAN_D1_UC_NODE=10` | No `ESCStatus`, `ESCInfo`, or `ESCTelemetry` message in 5 s; no node ID or per-ESC identity exposed | The FCU is configured to send DroneCAN RawCommand slots 0–2, but there is no evidence that any VESC is discovered or publishing DroneCAN ESC status telemetry. This can be bus/power/termination/bitrate, VESC DroneCAN firmware/configuration, or a telemetry capability issue; the parameter set alone cannot distinguish them. | Do not change ArduPilot yet. Passively inspect CAN1 frames or the FCU's node-discovery/status surface with the VESC firmware baseline, then verify that each VESC publishes DroneCAN ESC status. Only after identities are evidenced, configure/validate telemetry mapping. |
| CAN2 comparison | Not the current VESC bus | `CAN_P2_DRIVER=2`, `CAN_P2_BITRATE=1000000`; `CAN_D2_PROTOCOL=1`, but `CAN_D2_UC_ESC_BM=0`, `CAN_D2_UC_ESC_RV=0` | No ESC data | CAN2 has a separate enabled DroneCAN driver but no ESC command bitmap. It is not the active ESC-command configuration. | Leave unchanged; retain only as comparison until its attached devices are separately identified. |
| ESC logical slots | VESC order intentionally unknown | Bitmap `7` enables logical ESC slots 0, 1, and 2; all three are marked reversible. `SERVO1_FUNCTION=73` (Throttle Left), `SERVO3_FUNCTION=74` (Throttle Right), `SERVO2_FUNCTION=35`; `ESC_TLM_MAV_OFS=0`; `RPM1_ESC_MASK=0`, `RPM2_ESC_MASK=0` | No index, node ID, RPM, voltage, current, or temperature is received | Slots 0–2 are configured, but no received telemetry establishes which slot is left, right, or mower. The third function value must not be labelled as the mower solely from this parameter audit. `ESC_TLM_MAV_OFS=0` is recorded; it does not itself prove why telemetry is absent. | Keep the motor identity mapping unassigned. Obtain one passive per-node telemetry/ESC-status observation before assigning left/right/mower labels or enabling ESC-wheel odometry. |
| F9P / GPS2 | Serial u-blox F9P on Pixhawk GPS2, not DroneCAN | Pixhawk 5X hardware mapping identifies GPS2 as `SERIAL4` (UART8). The FCU has `SERIAL4_PROTOCOL=5` (GPS) and `SERIAL4_BAUD=460` (460800), with `GPS2_TYPE=1` (AUTO), `GPS_AUTO_CONFIG=3`, `GPS_AUTO_SWITCH=4`. `GPS1_TYPE=9` is DroneCAN. | MAVROS GPS1 and GPS2 both report `fix_type=0`, zero satellites, and invalid position; no `/gps/fix` publisher | The logical serial configuration for the physical GPS2 port is present. The absence of all receiver data points first to the F9P link/receiver state (power, TX/RX, baud/protocol, or receiver configuration), not to a missing `SERIALx_PROTOCOL=GPS` setting. `GPS1_TYPE=9` is a separate DroneCAN logical GPS configuration and does not turn the serial F9P into DroneCAN. | Do not alter FCU parameters from this evidence. Passively verify F9P power and UART traffic on GPS2, then inspect F9P protocol/baud/configuration against this exact 460800 serial setting. Re-evaluate GPS1/GPS2 MAVLink after valid bytes and satellites appear. |
| POWER1 / POWER2 | POWER1 charger/dock; POWER2 traction 7S | `BATT_MONITOR=21`, `BATT_I2C_BUS=1`, `BATT_I2C_ADDR=69`; `BATT2_MONITOR=21`, `BATT2_I2C_BUS=2`, `BATT2_I2C_ADDR=69`; both serial numbers are `-1` | `BATTERY_STATUS id0` matches the first monitor (`BATT_`); `id1` matches the second (`BATT2_`). Both are live at 1 Hz each. | None in the instance mapping: ArduPilot emits the battery-monitor instance as the MAVLink battery ID. Combined with the Pixhawk's two power-monitor buses and the supplied POWER1/POWER2 wiring, this establishes `id0 = BATT_ = POWER1` and `id1 = BATT2_ = POWER2`, independently of voltage. | Keep this mapping as the evidence-backed dock/traction mapping; do not use voltage alone for future remapping. Validate current-sign conventions separately before using either current as charging state. |

The Pixhawk 5X UART mapping is documented by ArduPilot as GPS1=`SERIAL3` and
GPS2=`SERIAL4`; its serial-port mapping is board-specific rather than a general
Pixhawk assumption. ArduPilot also documents `CAN_P1_DRIVER=1` plus
`CAN_D1_PROTOCOL=1` as the CAN1/DroneCAN configuration, and `CAN_D1_UC_ESC_BM`
bit positions 0–2 as the first three DroneCAN ESC command slots. The current
configuration matches those relationships, but there is still no passive
MAVLink proof that a VESC node exists on CAN1 or that it reports ESC telemetry.

### ESC MAVLink → MAVROS audit — 2026-09-22

This later, read-only audit supersedes the preceding `PRESENT_NO_DATA` finding
for `/mavros/esc_telemetry/telemetry` only. It is evidence for the running
Rock 5B stack at the time of observation: Pixhawk 5X over the MAVROS USB
endpoint, `mowgli-mavros:build-kilted-arm64`, MAVROS with MAVLink package
`2026.8.8`, and the attached three-VESC DroneCAN bus. It does not establish
wheel/mower identity, actuator safety, or a general rate guarantee.

The observation used ROS graph inspection, bounded passive subscriptions, and
a 15-second passive packet capture of the MAVROS UDP GCS mirror. It did not
send a MAVLink request, call a ROS service, change a parameter or mode, arm,
or publish an actuator command.

| MAVLink message | ID | Packets in 15 s | MAVROS consumer | Observed ROS result |
| --- | ---: | ---: | --- | --- |
| `ESC_TELEMETRY_1_TO_4` | 11030 | 29 | `esc_telemetry` | `/mavros/esc_telemetry/telemetry` at ~2 Hz |
| `ESC_TELEMETRY_5_TO_8` | 11031 | 0 | `esc_telemetry` | No data (no such frame observed) |
| `ESC_STATUS` | 291 | 0 | `esc_status` | `/mavros/esc_status/status` remains empty |
| `ESC_INFO` | 290 | 0 | `esc_status` | `/mavros/esc_status/info` remains empty |
| `RPM` | 226 | 30 | standard `wheel_odometry` plugin | Both payload fields were `-1.0`; no usable wheel RPM |

MAVROS loaded and initialized both `esc_status` and `esc_telemetry`; neither is
in its plugin denylist. `wheel_odometry` is in that denylist. The custom
`esc_wheel_odometry` plugin also loaded but deliberately disabled itself: its
left/right ESC slots, RPM field mapping, offset expectation, wheel radii, and
track width are all invalid/unassigned.

The received `ESC_TELEMETRY_1_TO_4` ROS sample contained three populated
slots: `30 C / 26.18 V / count 782`, `29 C / 26.03 V / count 1459`, and
`31 C / 26.28 V / count 778`; its fourth slot was zero. The fact that a DroneCAN
VESC status stream was independently observed near 49 Hz must not be projected
onto MAVLink: ArduPilot emitted the aggregated MAVLink telemetry near 2 Hz in
this baseline.

Mission Planner's `esc1`/`esc2`/`esc3` display is therefore explained by
MAVLink `ESC_TELEMETRY_1_TO_4`, not by `ESC_STATUS` or `ESC_INFO`. MAVROS maps
that APM-specific message family to `/mavros/esc_telemetry/telemetry`; it maps
the distinct standard messages `ESC_STATUS` and `ESC_INFO` to the two
`/mavros/esc_status/*` topics. Empty status/info topics are expected while IDs
291 and 290 are absent. No ArduPilot or VESC configuration was modified.
