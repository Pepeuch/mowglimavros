# MM-MAVROS-AUDIT-20260907

Status: RETAINED

Repository: Pepeuch/mowglimavros
Branch at audit: main
HEAD at audit: b3ea7066757cd7e513f451de9f6688adf3616d71
Audit date: 2026-09-07

## Purpose

Durable audit baseline for Kilted/Lyrical and MAVROS compatibility.

Do not rediscover these conclusions unless a listed dependency changes.

## Established facts

### MM-AUD-001 — SOFTWARE_NOW / BLOCKING
Local launch integration requests:
- apm.launch.py
- px4.launch.py

MAVROS 2.15.1 installs:
- apm.launch
- px4.launch

The current backend therefore fails before MAVROS launch.

### MM-AUD-002 — SOFTWARE_NOW / BLOCKING
APT repositories observed during audit expose MAVROS 2.15.0 on both
Kilted and Lyrical.

MAVROS 2.15.1 contains the serial baudrate fix required for 921600.

Pinned upstream release:
- MAVROS_VERSION=2.15.1
- MAVROS_COMMIT=22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e

Decision at audit:
use pinned MAVROS 2.15.1 source build for now.

### MM-AUD-003 — SOFTWARE_NOW / BLOCKING LYRICAL
Current workspace:
- Kilted amd64: builds
- Lyrical amd64: fails

Cause:
ament_target_dependencies() still works/deprecates on Kilted but is absent
from Lyrical ament 2.8.8.

This is required compatibility work, not optional cleanup.

### MM-AUD-004 — SOFTWARE_NOW / IMPORTANT
Local NTRIP handling can publish a 4096-byte mavros_msgs/RTCM.

MAVROS gps_rtk accepts at most 720 bytes per RTCM message.

RTCM input must be segmented/bounded before publication.
No byte may be lost, duplicated, or reordered.

### MM-AUD-005 — SOFTWARE_NOW
Current container build is not fully reproducible:
- ROS image tags float
- MAVROS/MAVLink are not pinned
- GeographicLib install script is fetched from moving ros2 branch
- GeographicLib installation is duplicated across stages

### MM-AUD-006 — SOFTWARE_NOW / LYRICAL
Boost package names are Noble/Kilted-specific:
- libboost-system-dev
- libboost-system1.83.0

They must not lock the image to Noble ABI assumptions.

### MM-AUD-007 — HARDWARE_PENDING
Still requires Pixhawk/ArduPilot hardware validation:
- manual_control mapping
- blade behaviour
- charging behaviour
- HOLD/disarm behaviour
- real feedback paths
- failsafe/reconnect semantics

Do not hide software defects behind this hardware gate.

### MM-AUD-008 — LOW PRIORITY
Tracked Python 3.12 .pyc files exist.
Do not include cleanup in the compatibility patch.

## MAVROS ROS contract verified

Used APIs remain compatible with MAVROS 2.15.1:

- /mavros/state
- /mavros/imu/data
- /mavros/battery
- /mavros/local_position/odom
- /mavros/manual_control/send
- /mavros/cmd/arming
- /mavros/set_mode
- /mavros/gps_rtk/send_rtcm

No Kilted/Lyrical conditional logic was found in the C++ implementation.

## Target architecture

One source tree.

Primary target:
- ROS Kilted

Compatibility target:
- ROS Lyrical

Architectures:
- amd64
- arm64

Preferred container strategy:
- one multi-stage Dockerfile
- ROS_DISTRO configurable
- MAVROS_VERSION configurable
- MAVROS 2.15.1 pinned by commit
- separate caches by distro/version/platform

## Invalidates this checkpoint

Re-audit affected conclusions only if one of these changes:

- repository implementation affecting the relevant contract;
- MAVROS version/tag/commit;
- Kilted or Lyrical base image/toolchain;
- APT MAVROS/MAVLink availability when choosing whether source pinning is still needed;
- relevant ArduPilot/Pixhawk runtime evidence.

Do not re-run the whole audit merely because the model/session changed.