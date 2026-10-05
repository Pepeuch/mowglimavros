# MM-MAVROS-MAG-UNIT-BUG-20261004

## Status

ACTIVE — verified on real Rock 5B + Pixhawk 5X runtime.

Do not work around this in MowgliNext. Fix the unit semantics in MowgliMAVROS / the pinned MAVROS source, then expose a standards-compliant `sensor_msgs/msg/MagneticField`.

## Problem

The MAVROS 2.16.0 IMU plugin publishes `/mavros/imu/mag` as
`sensor_msgs/msg/MagneticField`, whose magnetic field values are required to be
expressed in Tesla.

On the real Pixhawk/ArduPilot runtime, the topic instead produced:

```text
/mavros/imu/mag

magnetic_field:
  x: -272000.0
  y: -51000.00000000007
  z: -622000.0

```

Observed publication rate was approximately 2 Hz.
These values are physically impossible as Tesla and strongly indicate an
incorrect MAVLink magnetometer unit conversion inside MAVROS.
Verified runtime source
MAVROS logs show:
[mavros.imu]: IMU: Raw IMU message used.

Therefore the currently observed bad magnetic field comes from the MAVROS
RAW_IMU handling path, not merely SCALED_IMU.
Runtime topology at the time of the observation:
Pixhawk / ArduPilot
  -> MAVLink RAW_IMU
  -> MAVROS imu plugin
  -> /mavros/imu/mag       publisher=1
  -> /imu/mag_raw          publisher=0
  -> /imu/mag_yaw          publisher=0

/mavros/imu/mag used frame_id: base_link.
Unit analysis
ArduPilot GCS_MAVLink::send_raw_imu() places the primary compass field into
the MAVLink RAW_IMU xmag/ymag/zmag fields.
The ArduPilot compass field is represented in milligauss.
MAVROS 2.16.0 converts the RAW_IMU magnetometer values in its IMU plugin using
the constant currently named:
MILLIT_TO_TESLA = 1000.0

and multiplies RAW_IMU/SCALED_IMU magnetometer fields by that value before
publishing sensor_msgs/msg/MagneticField.
For milligauss -> Tesla the conversion factor should be:
1 mGauss = 1e-7 Tesla

The observed raw values are therefore plausibly approximately:
-272 mGauss -> -27.2 µT
 -51 mGauss ->  -5.1 µT
-622 mGauss -> -62.2 µT

Magnitude is then about 68 µT, which is physically plausible for the Earth's
field plus local vehicle distortion.
The currently published values differ by roughly 1e10 from the expected
milligauss-to-Tesla conversion.
Upstream context
MAVROS upstream issue #1855 reports a magnetometer unit-conversion problem in
the IMU plugin.
Our runtime evidence additionally confirms that the problem affects the
RAW_IMU path used by the current ArduPilot/Pixhawk configuration.
Re-check the exact upstream state before implementing because this may have
been fixed after the pinned MAVROS revision.
MowgliNext impact
MowgliNext expects the canonical topic:
/imu/mag_raw [sensor_msgs/msg/MagneticField]

Current MAVROS backend state:
/mavros/imu/mag
  publisher count: 1
  subscription count: 0

/imu/mag_raw
  publisher count: 0
  consumers include cog_to_imu/calibration

MowgliNext configuration on the tested robot had:
enable_mag_cal: true
use_magnetometer: true

but mag_yaw_publisher did not start because
/ros2_ws/maps/mag_calibration.yaml did not yet exist. This is expected launch
behaviour and is separate from the MAVROS unit bug.
Do NOT perform magnetometer calibration using the currently malformed MAVROS
values.
Intended fix direction
Preferred ownership:
1. Correct/patch the MAVROS magnetometer conversion in MowgliMAVROS.
2. Add regression tests for RAW_IMU and SCALED_IMU unit conversion.
3. Verify HIGHRES_IMU semantics separately; do not blindly apply one factor to
   all MAVLink IMU message families.
4. Validate /mavros/imu/mag on real hardware after rebuilding the sidecar.
5. Only once the MAVROS topic is standards-compliant, bridge/remap it to the
   Mowgli canonical /imu/mag_raw.
6. Preserve MAVROS base_link/FLU frame semantics and sensor-data QoS.
7. Run magnetometer calibration.
8. Confirm /imu/mag_yaw publication.
9. Only then enable/use the magnetometer factor in FusionGraph.
Acceptance criteria
- /mavros/imu/mag contains physically plausible Tesla values (typically
  order 1e-5 T, not 1e5 T).
- Unit conversion is covered by deterministic tests.
- Real Pixhawk RAW_IMU runtime verified.
- /imu/mag_raw receives standards-compliant Tesla values with no second
  conversion.
- Calibration can generate mag_calibration.yaml.
- /imu/mag_yaw publishes after calibration.
- FusionGraph can consume the magnetic yaw without backend-specific knowledge.
Out of scope for this checkpoint
- Performing the actual magnetometer calibration.
- Tuning magnetic declination / covariance.
- Deciding whether the physical compass location is sufficiently isolated from
  mower/drive motor fields.
- Enabling magnetometer yaw in production before calibration and interference
  validation.
Evidence date
2026-10-04.
