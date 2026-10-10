#!/bin/bash
set -eo pipefail
test "$(id -u)" = 1000
source /opt/ros/lyrical/setup.bash
source /ros2_ws/install/setup.bash
set -u
cmake -S /work/mowgli_mavros_bridge -B /work/build \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_SKIP_RPATH=ON \
  -Dmavlink_DIR=/sdk/share/mavlink/cmake
cmake --build /work/build --target mavros_hardware_bridge_node --parallel 2
readelf -h /work/build/mavros_hardware_bridge_node
ldd /work/build/mavros_hardware_bridge_node
sha256sum /work/build/mavros_hardware_bridge_node
