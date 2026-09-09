cmake_minimum_required(VERSION 3.08)
project(mowgli_mavros_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(mowgli_interfaces REQUIRED)
find_package(mavros_msgs REQUIRED)
find_package(mavros REQUIRED)
find_package(mavlink REQUIRED)
find_package(mavros_battery_observer REQUIRED)

add_executable(mavros_hardware_bridge_node
  src/mavros_hardware_bridge_node.cpp
  src/power_mapping.cpp
)

target_include_directories(mavros_hardware_bridge_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${mavlink_INCLUDE_DIRS}
)

target_link_libraries(mavros_hardware_bridge_node PUBLIC
  ${geometry_msgs_TARGETS}
  ${mavros_msgs_TARGETS}
  ${mavros_battery_observer_TARGETS}
  ${mowgli_interfaces_TARGETS}
  ${sensor_msgs_TARGETS}
  ${std_msgs_TARGETS}
  ${std_srvs_TARGETS}
  rclcpp::rclcpp
  sensor_msgs::sensor_msgs_library
  tf2::tf2
  tf2_geometry_msgs::tf2_geometry_msgs
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_power_mapping test/test_power_mapping.cpp src/power_mapping.cpp)
  target_include_directories(test_power_mapping PRIVATE include ${mavlink_INCLUDE_DIRS})
endif()

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY
    launch
    config
  DESTINATION share/${PROJECT_NAME}
)

install(
  TARGETS mavros_hardware_bridge_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
