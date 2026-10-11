#!/bin/bash
set -eo pipefail
test "$(id -u)" = 1000
source /opt/ros/lyrical/setup.bash
source /opt/mowgli/mavros/setup.bash
source /tmp/blade-product-gnss-msgs-install/local_setup.bash
source /tmp/blade-product-ug-install/local_setup.bash
source /tmp/fcu-state/install/local_setup.bash
export LD_LIBRARY_PATH=/tmp/blade-off-cyclone/lib:$LD_LIBRARY_PATH
export ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST
export ROS_DOMAIN_ID=203
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
mkdir -p /tmp/fcu-state/review-fixes
colcon --log-base /tmp/fcu-state/log build --base-paths /tmp/fcu-src/ros2/src --packages-select mowgli_mavros_bridge --build-base /tmp/fcu-state/build --install-base /tmp/fcu-state/install --executor sequential --cmake-args -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRMW_IMPLEMENTATION=rmw_fastrtps_cpp 2>&1 | tee /tmp/fcu-state/review-fixes/build.log
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ctest --test-dir /tmp/fcu-state/build/mowgli_mavros_bridge --output-on-failure 2>&1 | tee /tmp/fcu-state/review-fixes/bridge-tests.log
