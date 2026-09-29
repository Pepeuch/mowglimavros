#pragma once

#include <geometry_msgs/msg/twist_stamped.hpp>
#include <mavros_msgs/msg/manual_control.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace mowgli_mavros_bridge
{
inline bool neutral_twist(const geometry_msgs::msg::TwistStamped& twist)
{
  return twist.twist.linear.x == 0.0 && twist.twist.angular.z == 0.0;
}

inline bool manual_control_allowed(const geometry_msgs::msg::TwistStamped& twist,
                                   bool drive_enabled, bool neutral_test_enabled)
{
  return std::isfinite(twist.twist.linear.x) && std::isfinite(twist.twist.angular.z) &&
         (drive_enabled || (neutral_test_enabled && neutral_twist(twist)));
}

inline mavros_msgs::msg::ManualControl rover_manual_control_from_twist(
    const geometry_msgs::msg::TwistStamped& twist,
    double linear_scale,
    double yaw_scale)
{
  mavros_msgs::msg::ManualControl command{};
  // ArduRover uses MANUAL_CONTROL.y for steering and .z for throttle.
  command.y = static_cast<int16_t>(std::clamp(
      twist.twist.angular.z * yaw_scale, -1000.0, 1000.0));
  command.z = static_cast<int16_t>(std::clamp(
      twist.twist.linear.x * linear_scale, -1000.0, 1000.0));
  return command;
}
}  // namespace mowgli_mavros_bridge
