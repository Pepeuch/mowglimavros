#!/bin/bash
set -eo pipefail
test "$(id -u)" = 1000
source /opt/ros/lyrical/setup.bash
source /ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=197 ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST
python3 /work/mowgli_mavros_bridge/test/test_blade_graph.py --bridge-executable /work/build/mavros_hardware_bridge_node
python3 /work/mowgli_mavros_bridge/test/test_provider_graph.py --bridge-executable /work/build/mavros_hardware_bridge_node
