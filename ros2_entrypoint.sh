#!/bin/bash
set -e

# Source ROS2 and the pinned MAVROS overlay.
: "${ROS_DISTRO:=kilted}"
source "/opt/ros/${ROS_DISTRO}/setup.bash"
source /opt/mowgli/mavros/setup.bash
if [ -f /ros2_ws/install/setup.bash ]; then
  source /ros2_ws/install/setup.bash
fi

: "${ROS_DOMAIN_ID:=0}"
: "${RMW_IMPLEMENTATION:=rmw_cyclonedds_cpp}"

export ROS_DOMAIN_ID
export RMW_IMPLEMENTATION

: "${MAVROS_PORT:=/dev/mavros}"
: "${MAVROS_BAUD:=921600}"
: "${MAVROS_GCS_URL:=}"
: "${MAVROS_TGT_SYSTEM:=1}"
: "${MAVROS_TGT_COMPONENT:=1}"
: "${MAVROS_AUTOPILOT:=ardupilot}"

MAVROS_FCU_URL="serial://${MAVROS_PORT}:${MAVROS_BAUD}"
export MAVROS_FCU_URL
exec ros2 launch mowgli_mavros_bridge mavros_backend.launch.py
