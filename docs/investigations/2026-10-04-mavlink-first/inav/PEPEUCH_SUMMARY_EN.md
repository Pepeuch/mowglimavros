# Summary for Pepeuch — USB and GPS analysis

I analysed the Mamba F722 with the GPS connected. **The flight controller is running INAV 9.1.0, not Betaflight**, so the live results below confirm INAV behaviour on this board, not the messages emitted by a Betaflight installation.

## What I confirmed

- The FC's USB virtual COM port responds to **MSP v1** and also provides CLI access. USB is currently configured for MSP, not MAVLink telemetry.
- After reconnecting the GPS cable that was missing during the first scan, INAV identified **UBLOX10, protocol 34.09, at 115200 baud** on the configured GPS port. During a 180-second capture, the counter increased from **181 to 3091 valid UBX frames**. The GPS-to-FC serial link is working and INAV is parsing the receiver's messages.
- GPS-related MSP requests (`RAW_GPS` 106, `COMP_GPS` 107 and `GPSSTATISTICS` 166) returned valid responses. There was **no fix and no satellites**, and therefore no usable position: the scan was done indoors. An outdoor test with a clear view of the sky is needed to confirm acquisition of a fix.

## Findings that can help with the Betaflight GPS backend / MAVROS work

- **MSP over USB:** I compared the INAV 9.1 and Betaflight 2026.6.2 source. MSP 106 has the same 16-byte prefix in both (fix, satellites, latitude, longitude, altitude, speed and course). The last two bytes have different meanings: **HDOP in INAV, PDOP in Betaflight**. MSP 107 is five bytes in both, but its bearing units need normalization. INAV also provides GPS statistics in MSP 166, which Betaflight does not serialize in the examined release. This gives a concrete starting point for a mostly shared MSP parser with firmware-specific handling.
- **MAVLink / MAVROS:** Both firmware sources emit the standard `GPS_RAW_INT` (#24) and `GLOBAL_POSITION_INT` (#33) messages. Mowgli's current backend uses `GPS_RAW_INT` for its canonical GNSS input. It needs horizontal and vertical accuracy extensions to create covariance that the fusion system can use, and it uses `alt_ellipsoid` for canonical altitude. INAV currently sets that altitude field to zero. Betaflight populates it with its MSL altitude value; that may avoid a missing value, but **does not prove the ellipsoid datum is correct**.
- The USB capture here is **MSP, not MAVLink**. It does not demonstrate MAVLink transport from a Betaflight FC to MAVROS; the MAVLink points above come from source-code analysis.
- These comparisons use the firmware and backend source revisions listed in the full report. They can guide adapter design and identify fields to verify, but they do not prove the values emitted by a running Betaflight board or that Mowgli accepts its fix at runtime.

## What this capture does not cover

- It does not show **Betaflight running on the board**; the board was running INAV. The source comparison above does identify GPS parser differences to handle.
- It does not capture raw UBX bytes on the GPS-to-FC wire. It observes GPS data parsed by INAV and exposed via MSP.
- It contains no MAVLink capture from this FC; USB was not assigned to MAVLink.
- It contains no test of ESC messages forwarded to MAVROS. No motor was run.

I also compared the INAV and Mowgli/MAVROS sources. That points to integration details for INAV, including GPS covariance and ellipsoid altitude. These are code-level findings, **not runtime validation**.

**In short:** this investigation is useful for the GPS part: it confirms the physical GPS-to-FC link under INAV, documents the MSP replies, and compares GPS fields in both firmware sources with the current Mowgli/MAVROS path. Those messages still need runtime validation on Betaflight. ESC-to-MAVROS testing remains a separate investigation.

See the [full report](REPORT.md) and its source notes for detailed evidence and logs.
