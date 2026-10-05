cmake_minimum_required(VERSION 3.16...4.2)
project(mowgli_mavros_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
ament_python_install_package(${PROJECT_NAME} PACKAGE_DIR python/${PROJECT_NAME})
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
find_package(libmavconn REQUIRED)
find_package(mavlink REQUIRED)

add_executable(detect_mavros_firmware src/detect_mavros_firmware.cpp)
# libmavconn 2.16 exports include/library variables, not an imported target.
target_include_directories(detect_mavros_firmware SYSTEM PRIVATE
  ${libmavconn_INCLUDE_DIRS}
  ${mavlink_INCLUDE_DIRS}
)
target_link_libraries(detect_mavros_firmware PRIVATE ${libmavconn_LIBRARIES})

add_executable(mavros_hardware_bridge_node
  src/mavros_hardware_bridge_node.cpp
  src/readiness_state.cpp
  src/firmware_provider.cpp
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
  find_package(ament_cmake_test REQUIRED)
  find_package(mavros_gnss_adapter REQUIRED)
  ament_add_test(test_gnss_launch
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test/test_gnss_launch.py"
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
  )
  ament_add_test(test_firmware_provider
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test/test_firmware_provider.py"
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
  )
  ament_add_test(test_firmware_detection
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test/test_firmware_detection.py"
      $<TARGET_FILE:detect_mavros_firmware>
    TIMEOUT 30
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
  )
  ament_add_test(test_provider_graph
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test/test_provider_graph.py"
      --bridge-executable $<TARGET_FILE:mavros_hardware_bridge_node>
    TIMEOUT 180
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
  )
  ament_add_test(test_gnss_graph
    COMMAND "${Python3_EXECUTABLE}"
      "${mavros_gnss_adapter_DIR}/../../../lib/mavros_gnss_adapter/test_gnss_graph.py"
      --bridge-executable $<TARGET_FILE:mavros_hardware_bridge_node>
    TIMEOUT 180
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
  )
  ament_add_gtest(test_readiness_state test/test_readiness_state.cpp src/readiness_state.cpp)
  ament_add_gtest(test_esc_telemetry_tracker test/test_esc_telemetry_tracker.cpp)
  target_include_directories(test_esc_telemetry_tracker PRIVATE include)
  ament_add_gtest(test_command_provider test/test_command_provider.cpp src/firmware_provider.cpp)
  target_include_directories(test_command_provider PRIVATE include)
  target_link_libraries(test_command_provider
    geometry_msgs::geometry_msgs__rosidl_generator_cpp
    mavros_msgs::mavros_msgs__rosidl_generator_cpp
  )
  ament_add_gtest(test_rover_manual_control test/test_rover_manual_control.cpp)
  target_include_directories(test_rover_manual_control PRIVATE include)
  target_link_libraries(
    test_rover_manual_control
    geometry_msgs::geometry_msgs__rosidl_generator_cpp
    mavros_msgs::mavros_msgs__rosidl_generator_cpp
  )
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
  TARGETS mavros_hardware_bridge_node detect_mavros_firmware
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
