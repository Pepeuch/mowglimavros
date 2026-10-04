# Test matrix

| Test | Result | Evidence | Boundary |
|---|---|---|---|
| Betaflight identity/config | HDZERO_HALO, Betaflight 2026.6.1; DSHOT600, bidirectional DShot, EDT initially OFF. USB serial identifier withheld. | `REPORT.md`, `captures/idle-baseline-msp.bin` | Runtime FC observation; full CLI dumps are retained locally but not published. |
| Per-motor RPM | Motors tested individually and together at low disarmed outputs. MSP #139 returned independent RPM; no direction sign. | `captures/motor-sweep-frames.jsonl` | No optical tachometer; no mower traction/load/reverse validation. |
| Final EDT trial | Temporary EDT ON; all four motors briefly at output 1150 individually. RPM returned, while temperature, voltage, current and consumption remained zero in #139. | `captures/edt-motor-test-frames.jsonl`, `captures/edt-reboot-idle.csv` | ESC firmware build/config was not identified; this does not distinguish ESC capability from FC decode support. |
| Final restoration | EDT saved OFF and verified after reboot; all four motor outputs verified at 1000. Final status was 92 °C FC core and 22.69 V (6S OK). | `captures/edt-restored-idle.csv`, `restoration.json` | FC core temperature is not ESC temperature. |
| INAV GPS link | During the connected indoor 180 s poll, 2520/2520 selected MSP requests were checksum-valid; UBX packet counter rose 181→3091; no fix was acquired. | `../inav/runs/2026-10-04-gps-connected/raw/msp-readonly-poll.csv` | No outdoor fix/acquisition test; no MAVLink capture. |
| Betaflight / INAV sender inventory | Source review found no ESC_INFO/ESC_STATUS, RPM, WHEEL_DISTANCE or ESC_TELEMETRY sender in the inspected BF/INAV MAVLink telemetry modules. | `SOURCE_NOTES.md`, `../inav/SOURCE_NOTES.md` | Source analysis only, not proof from a live MAVLink packet capture. |
| MAVLink runtime capture | Not performed: both tested VCPs were configured for MSP and no MAVLink port was assigned. | `../README.md` | No temporary routing changes were made. |
