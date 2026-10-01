cmake_minimum_required(VERSION 3.16...4.2)
project(mowgli_mavros_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(mowgli_interfaces REQUIRED)
find_package(mavros_msgs REQUIRED)
find_package(mavros REQUIRED)

add_executable(mavros_hardware_bridge_node
  src/mavros_hardware_bridge_node.cpp
  src/readiness_state.cpp
)

target_include_directories(mavros_hardware_bridge_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(mavros_hardware_bridge_node PUBLIC
  ${geometry_msgs_TARGETS}
  ${nav_msgs_TARGETS}
  ${diagnostic_msgs_TARGETS}
  ${mavros_msgs_TARGETS}
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
  ament_add_gtest(test_readiness_state test/test_readiness_state.cpp src/readiness_state.cpp)
  ament_add_gtest(test_serial_gps_projection test/test_serial_gps_projection.cpp)
  target_include_directories(test_serial_gps_projection PRIVATE include)
  ament_add_gtest(test_esc_telemetry_tracker test/test_esc_telemetry_tracker.cpp)
  target_include_directories(test_esc_telemetry_tracker PRIVATE include)
  ament_add_gtest(test_rover_manual_control test/test_rover_manual_control.cpp)
  target_include_directories(test_rover_manual_control PRIVATE include)
  target_link_libraries(test_rover_manual_control ${geometry_msgs_TARGETS} ${mavros_msgs_TARGETS})
  target_include_directories(test_readiness_state PRIVATE include)
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
