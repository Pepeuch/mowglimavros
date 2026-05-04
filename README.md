# mowglimavros

Sidecar ROS 2 repository for the optional MAVROS backend used by MowgliNext.

This repository intentionally contains only:

- `ros2/src/mowgli_interfaces` (minimal interface package required to build the sidecar alone)
- `ros2/src/mowgli_mavros_bridge`
- `ros2/src/mowgli_ntrip_client`
- `Dockerfile`
- `ros2_entrypoint.sh`
- `build.sh`
- `README.md`

It does not carry MowgliNext bringup, GUI, Nav2, simulation, FusionCore, or any
other main-stack package.

The sidecar is expected to run next to MowgliNext over ROS 2/DDS with
`network_mode: host`, while MowgliNext itself only switches backend selection
through `HARDWARE_BACKEND=mavros`.
