#!/bin/bash
set -eo pipefail
test "$(id -u)" = 1000
source /opt/ros/lyrical/setup.bash
source /opt/mowgli/mavros/setup.bash
source /tmp/blade-product-gnss-msgs-install/local_setup.bash
source /tmp/blade-product-ug-install/local_setup.bash
source /tmp/fcu-state/install/local_setup.bash
export LD_LIBRARY_PATH=/tmp/blade-off-cyclone/lib:$LD_LIBRARY_PATH
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
cmake --build /tmp/fcu-state/build/mowgli_mavros_bridge --target test_bridge_regressions --parallel 2 2>&1 | tee /tmp/fcu-state/review-fixes/final-peer-build.log
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST
export ROS_DOMAIN_ID=203
ctest --test-dir /tmp/fcu-state/build/mowgli_mavros_bridge -R '^test_bridge_regressions$' --output-on-failure 2>&1 | tee /tmp/fcu-state/review-fixes/final-peer-test.log
sha256sum /tmp/fcu-state/build/mowgli_mavros_bridge/mavros_hardware_bridge_node /tmp/fcu-state/install/mowgli_mavros_bridge/lib/mowgli_mavros_bridge/mavros_hardware_bridge_node > /tmp/fcu-state/review-fixes/binaries.sha256
