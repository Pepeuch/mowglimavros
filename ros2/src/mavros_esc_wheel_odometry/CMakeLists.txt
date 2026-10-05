cmake_minimum_required(VERSION 3.16...4.2)
project(mavros_esc_wheel_odometry)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(std_msgs REQUIRED)
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/EscObservation.msg"
  DEPENDENCIES builtin_interfaces std_msgs
)
rosidl_get_typesupport_target(esc_typesupport ${PROJECT_NAME} rosidl_typesupport_cpp)
find_package(ament_cmake_gtest REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(diagnostic_updater REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(libmavconn REQUIRED)
find_package(mavros REQUIRED)
find_package(mavros_msgs REQUIRED)
find_package(mavlink REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

add_library(wheel_odometry_core STATIC
  src/wheel_odometry_core.cpp src/motor_tick_integrator.cpp src/legacy_wheel_adapter.cpp src/observation_engine.cpp)
set_target_properties(wheel_odometry_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(wheel_odometry_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_library(mavros_esc_wheel_odometry_plugins SHARED src/esc_wheel_odometry_plugin.cpp)
target_include_directories(mavros_esc_wheel_odometry_plugins PRIVATE
  ${mavlink_INCLUDE_DIRS}
  ${mavros_INCLUDE_DIRS}
  ${libmavconn_INCLUDE_DIRS})
target_link_libraries(mavros_esc_wheel_odometry_plugins PUBLIC
  wheel_odometry_core
  ${esc_typesupport}
  ${diagnostic_msgs_TARGETS}
  diagnostic_updater::diagnostic_updater
  ${geometry_msgs_TARGETS}
  ${libmavconn_LIBRARIES}
  ${mavros_LIBRARIES}
  ${mavros_msgs_TARGETS}
  ${nav_msgs_TARGETS}
  pluginlib::pluginlib
  rclcpp::rclcpp
  ${sensor_msgs_TARGETS}
  ${std_msgs_TARGETS}
  tf2_eigen::tf2_eigen
  tf2_geometry_msgs::tf2_geometry_msgs
  tf2_ros::tf2_ros)

pluginlib_export_plugin_description_file(mavros mavros_plugins.xml)

install(TARGETS wheel_odometry_core EXPORT wheel_odometry_core_export
  ARCHIVE DESTINATION lib)
ament_export_targets(wheel_odometry_core_export HAS_LIBRARY_TARGET)
ament_export_dependencies(rosidl_default_runtime builtin_interfaces std_msgs)
install(TARGETS mavros_esc_wheel_odometry_plugins
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY include/ DESTINATION include)
install(FILES mavros_plugins.xml DESTINATION share/${PROJECT_NAME})
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  ament_add_gtest(test_motor_tick_integrator test/test_motor_tick_integrator.cpp)
  target_link_libraries(test_motor_tick_integrator wheel_odometry_core)
  ament_add_gtest(test_observation_engine test/test_observation_engine.cpp)
  target_link_libraries(test_observation_engine wheel_odometry_core)
  ament_add_gtest(test_wheel_odometry_core test/test_wheel_odometry_core.cpp)
  target_link_libraries(test_wheel_odometry_core wheel_odometry_core)
endif()

ament_package()
