# Upstream source notes

Research date: 2026-10-04. Source-level statements below are pinned to identifiable revisions where possible. These do not replace the live FC observations in `REPORT.md`.

## Revisions inspected

| Project | Version/ref | Commit | Use in report |
|---|---|---|---|
| INAV | 9.1.0 release | `e519b69b02e27c8bdc03b4a0889f1baaae211a54` | Exact installed device version; GPS/MSP/MAVLink path. |
| Betaflight | 2026.6.2 release | `e0b7bb0` | Current stable source comparison and the matching DIAT target metadata. |
| MowgliNext | `feat/mavros-refresh` | `b18e6c394a2bed9bd0e60880395cd3a171ff10f7` | Integration branch and expected sidecar contract. 18 ahead / 34 behind `dev` at `6f37770878e4558d385320664ac737d91a8b5eea`. |
| Pepeuch MAVROS backend | `Pepeuch/mowglimavros` main | `82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492` | Current backend source at final review; see the explicit fix-type refresh. |
| MAVROS | 2.16.0 | `5c68b905ab30de6ce630822dc46c33467e8f23ea` | Pinned by sidecar Dockerfile. |
| Universal GNSS | pinned sidecar source | `383caba3de94e16167764393d5a4ef046078b015` | Private GNSS MAVROS plugin and canonical ROS mapping. |

The INAV 9.1.0 release page shows the tag resolves to `e519b69`; the Betaflight 2026.6.2 release page shows `e0b7bb0`. INAV later published 10.0.0-rc2 before this research date. This comparison uses installed 9.1.0 source rather than assuming the aircraft runs the later release candidate. [INAV 9.1.0 release](https://github.com/iNavFlight/inav/releases/tag/9.1.0), [INAV releases](https://github.com/iNavFlight/inav/releases), [Betaflight 2026.6.2 release](https://github.com/betaflight/betaflight/releases/tag/2026.6.2).

## INAV GPS input and internal solution

- [`gps.c` at e519b69](https://github.com/iNavFlight/inav/blob/e519b69/src/main/io/gps.c#L78-L108): driver solution (`gpsSolDRV`) and common/internal solution (`gpsSol`), provider selection.
- [`gps.c` initialization/port selection](https://github.com/iNavFlight/inav/blob/e519b69/src/main/io/gps.c#L414-L468): UBLOX opens a `FUNCTION_GPS` serial port; MSP provider uses the MSP GPS path rather than a physical GPS UART.
- [`gps.c` fix and common-solution processing](https://github.com/iNavFlight/inav/blob/e519b69/src/main/io/gps.c#L317-L377): sensor-ready, fix and minimum-satellite gates before common navigation consumes a solution.
- [`gps_msp.c`](https://github.com/iNavFlight/inav/blob/e519b69/src/main/io/gps_msp.c#L53-L107): richer MSPv2 sensor-GPS input populates fix/satellites/position/velocity/accuracy/HDOP/time and enters the same common solution path.
- [`fc_msp.c`, MSP_SET_RAW_GPS](https://github.com/iNavFlight/inav/blob/e519b69/src/main/fc/fc_msp.c#L2853-L2881): legacy inbound command 201 has 14-byte position/fix/speed payload and zeroes velocity.
- [`msp_protocol_v2_sensor.h`](https://github.com/iNavFlight/inav/blob/e519b69/src/main/msp/msp_protocol_v2_sensor.h#L16-L24) and [`msp_protocol_v2_sensor_msg.h`](https://github.com/iNavFlight/inav/blob/e519b69/src/main/msp/msp_protocol_v2_sensor_msg.h#L35-L59): `MSP2_SENSOR_GPS` (`0x1F03`) data structures and 51-byte rich sensor payload.

## Betaflight GPS input and internal solution

- [`gps.c` initialization and serial-provider handling](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/io/gps.c#L399-L462): MSP/virtual providers can feed the shared GPS solution without taking ownership of a physical GPS serial port; UBLOX/NMEA path opens a `FUNCTION_GPS` port and runs receiver detection/configuration.
- [`gps.c` solution/update state](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/io/gps.c#L2308-L2316): GPS update bookkeeping distinguishes directly parsed vs MSP-injected solution updates.
- [`msp.c` legacy MSP_SET_RAW_GPS](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/msp/msp.c#L3873-L3881): incoming command 201 fills the shared solution through the legacy MSP path.

## INAV MSP output

- [`msp_protocol.h` command IDs](https://github.com/iNavFlight/inav/blob/e519b69/src/main/msp/msp_protocol.h#L210-L225): `MSP_RAW_GPS` 106, `MSP_COMP_GPS` 107, `MSP_GPSSVINFO` 164, `MSP_GPSSTATISTICS` 166, and related GPS commands.
- [`fc_msp.c` serializers](https://github.com/iNavFlight/inav/blob/e519b69/src/main/fc/fc_msp.c#L909-L954): 106 is 18 bytes, ending in HDOP; 107 is 5 bytes; 164 is a compatibility stub; 166 emits last-message timing, errors, timeouts, packet count, HDOP/EPH/EPV and hardware version.
- [`MSP_SENSOR_STATUS`](https://github.com/iNavFlight/inav/blob/e519b69/src/main/fc/fc_msp.c#L420-L430): aggregate plus per-sensor health/status bytes.
- [`MSP_STATUS` and extended status](https://github.com/iNavFlight/inav/blob/e519b69/src/main/fc/fc_msp.c#L441-L480): legacy status vs INAV extended status response.
- [`MSP_WP` serializer](https://github.com/iNavFlight/inav/blob/e519b69/src/main/fc/fc_msp.c#L1786-L1800): waypoint reply fields; waypoint 0 is home when set. The investigation did not request waypoint 0 from the board.

## INAV serial/VCP and MAVLink

- [`serial.h` function masks](https://github.com/iNavFlight/inav/blob/e519b69/src/main/io/serial.h#L28-L59): GPS, MSP, MSP-OSD and MAVLink telemetry function values.
- [`serial.c` serial-port types](https://github.com/iNavFlight/inav/blob/e519b69/src/main/io/serial.c#L60-L95) and [port dispatch](https://github.com/iNavFlight/inav/blob/e519b69/src/main/io/serial.c#L334-L355): VCP/UART separation and serial-port identifiers.
- [`Serial.md`](https://github.com/iNavFlight/inav/blob/e519b69/docs/Serial.md#serial-port-types): VCP/physical UART distinction and serial-function configuration.
- [`mavlink.c` compile gate](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L25-L28), [port initialization, version and streams](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L302-L366), and [telemetry sharing gate](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/telemetry.c#L139-L150).
- [`mavlink.c` GPS_RAW_INT packer](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L577-L634): GPS_RAW_INT common fields and MAVLink2 extensions, including `alt_ellipsoid=0` and h/v accuracy copied from EPH/EPV.
- [`GLOBAL_POSITION_INT` and GPS_GLOBAL_ORIGIN](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L635-L668): emitted by INAV position sender; this is not HOME_POSITION.
- [`HEARTBEAT`](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L703-L782), [ATTITUDE](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L670-L688), [VFR_HUD](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L784-L824), [SYSTEM_TIME](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L690-L701), [stream schedule](https://github.com/iNavFlight/inav/blob/e519b69/src/main/telemetry/mavlink.c#L916-L949).

## Betaflight GPS, MSP, MAVLink and target

- [`msp.c`, MSP_RAW_GPS 106 and COMP_GPS 107](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/msp/msp.c#L1606-L1622): release 2026.6.2 emits the same 16-byte common prefix plus PDOP at the final u16; COMP_GPS remains a 5-byte response. Its COMP_GPS direction divides the internally stored value by 10, while INAV writes its stored direction directly, so normalize bearing units before sharing a decoder.
- [`msp.c`, MSP_GPSSVINFO](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/msp/msp.c#L1624-L1630): actual satellite-channel records in the BF path. No GPSSTATISTICS serializer was found in the pinned release's `msp.c`.
- [`mavlink.c`, sender init/version framing](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L273-L299) and [position/GPS_RAW_INT packer](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L687-L747): BF emits MAVLink2 GPS_RAW_INT extensions; `alt_ellipsoid` is numerically populated with the MSL altitude value; accuracy extensions are copied from GPS solution accuracy.
- [`GLOBAL_POSITION_INT`](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L754-L784), [`HOME_POSITION`](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L804-L820), [ATTITUDE](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L822-L846), and [VFR_HUD](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L927-L957).
- [Betaflight telemetry message tables](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L1030-L1095): HEARTBEAT, SYS_STATUS, GPS position, ATTITUDE, VFR_HUD and stream scheduling. `SYSTEM_TIME` is sent alongside SYS_STATUS ([packer](https://github.com/betaflight/betaflight/blob/e0b7bb0/src/main/telemetry/mavlink.c#L583-L653)). Neither pinned telemetry sender packs LOCAL_POSITION_NED.
- [`DIAT/MAMBAF722_2022A` target config](https://github.com/betaflight/config/blob/master/configs/DIAT/MAMBAF722_2022A/config.h#L467-L536) and [official target explorer](https://support.betaflight.com/targets/MAMBAF722_2022A): target exists and has the UART/pin map stated in the report. These target files are maintained separately from the firmware repository; exact coupling to release tag `e0b7bb0` is unverified.

## MowgliNext and Pepeuch MAVROS source

The complete Mowgli/MAVROS analysis, exact topic expectations, readiness/odometry caveats, and source links are in [`MOWGLI_MAVROS_SOURCE_FINDINGS.md`](MOWGLI_MAVROS_SOURCE_FINDINGS.md). Key permalinks:

- [MowgliNext integration README](https://github.com/mowglinext/mowglinext/blob/b18e6c394a2bed9bd0e60880395cd3a171ff10f7/sensors/mavros/README.md) and [sidecar Dockerfile pins](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/Dockerfile#L5-L13).
- [Canonical GNSS adapter](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mavros_gnss_adapter/src/gnss_adapter_plugin.cpp#L39-L94).
- [Universal GNSS raw GPS conversion](https://github.com/Pepeuch/universal-gnss/blob/383caba3de94e16167764393d5a4ef046078b015/gnss_mavros/src/universal_gnss_plugin.cpp#L41-L211), [accuracy/covariance conversion](https://github.com/Pepeuch/universal-gnss/blob/383caba3de94e16167764393d5a4ef046078b015/gnss_ros2/src/navsat_fix_adapter.cpp#L35-L63), and [measurement packet identity/freshness](https://github.com/Pepeuch/universal-gnss/blob/383caba3de94e16167764393d5a4ef046078b015/gnss_mavros/src/mavlink_gnss_adapter.cpp#L223-L245).
- [Mowgli fusion covariance acceptance](https://github.com/mowglinext/mowglinext/blob/b18e6c394a2bed9bd0e60880395cd3a171ff10f7/ros2/src/fusion_graph/src/fusion_graph_node_callbacks_a.cpp#L258-L288).
- [Bridge topic subscriptions](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp#L72-L108), [INAV/BF profile gate](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py#L35-L43), and [readiness rules](https://github.com/Pepeuch/mowglimavros/blob/82a1e390b59eeb7ab08c10dcd8e4c5675ec4c492/ros2/src/mowgli_mavros_bridge/src/readiness_state.cpp#L40-L106).

## Evidence boundaries

- The connected board evidence is from read-only CLI queries and MSP v1 requests only; no MAVLink runtime stream was enabled/captured.
- No GPS fix or receiver packet was observed. Configured update rate, protocol, and target source capabilities must not be presented as measured receiver behavior.
- The exact run-time cause of the IMU calibration-value and uptime differences is unknown. No source conclusion is substituted for the missing hardware explanation.
- No repository writes, GitHub writes, flight, arming, motor command, or deliberate servo command occurred.
