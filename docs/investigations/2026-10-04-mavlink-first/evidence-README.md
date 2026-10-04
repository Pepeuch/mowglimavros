# Evidence included in this public bundle

This folder contains the source-grounded reports and selected runtime records for the 2026-10-04 Betaflight/INAV → MAVLink → MAVROS investigation. Hardware observations are limited to the specific boards and firmware versions named in the reports. The MSP records do not constitute an FC-to-MAVROS MAVLink capture.

## Betaflight

- `betaflight/captures/motor-sweep-frames.jsonl`: bounded disarmed low-output RPM sweep; temporary MSP_SET_MOTOR commands are recorded in the stream.
- `betaflight/captures/edt-motor-test-frames.jsonl`: final EDT-ON trial, one motor at a time, output 1150 for about 0.55 s, followed by neutral writes.
- The adjacent CSVs capture read-only idle/reboot/restoration MSP polls. The final poll confirms `dshot_edt=OFF`, four motor outputs at 1000, and zero RPM at rest.
- FC core temperature at the final status read was 92 °C; this is not ESC temperature.

## INAV

- `inav/raw/` is the initial no-GPS-cable read-only MSP poll.
- `inav/runs/2026-10-04-gps-connected/raw/` is the repeat 180-second connected poll: 2,520 read-only requests, all checksum-valid; the UBX packet counter rose 181→3091, while no navigation fix was acquired indoors.
- Captures contain the logged MSP fields (including zero position fields) and no host USB serial identifier.

Full CLI configuration dumps, aircraft profile names, host USB device identifiers, incomplete/misaligned CLI capture attempts, and source checkouts are intentionally excluded. No MAVLink settings, firmware, ESC settings, arming, or mower motion were changed for this investigation.
