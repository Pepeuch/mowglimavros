#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <geometry_msgs/msg/twist_stamped.hpp>

#include <mavros_msgs/msg/manual_control.hpp>

namespace mowgli_mavros_bridge
{
inline mavros_msgs::msg::ManualControl rover_manual_control_from_twist(
    const geometry_msgs::msg::TwistStamped& twist, double linear_scale, double yaw_scale)
{
  mavros_msgs::msg::ManualControl command{};
  // ArduRover uses MANUAL_CONTROL.y for steering and .z for throttle.
  command.y = static_cast<int16_t>(std::clamp(twist.twist.angular.z * yaw_scale, -1000.0, 1000.0));
  command.z =
      static_cast<int16_t>(std::clamp(twist.twist.linear.x * linear_scale, -1000.0, 1000.0));
  return command;
}
}  // namespace mowgli_mavros_bridge
