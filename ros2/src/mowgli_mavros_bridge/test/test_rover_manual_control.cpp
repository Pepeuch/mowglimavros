#include <gtest/gtest.h>
#include <mowgli_mavros_bridge/rover_manual_control.hpp>

TEST(RoverManualControl, MapsSteeringAndThrottleToRoverAxes)
{
  geometry_msgs::msg::TwistStamped twist{};
  twist.twist.linear.x = 0.2;
  twist.twist.angular.z = -0.35;

  const auto command = mowgli_mavros_bridge::rover_manual_control_from_twist(twist, 1000.0, 1000.0);
  EXPECT_FLOAT_EQ(command.y, -350.0F);
  EXPECT_FLOAT_EQ(command.z, 200.0F);
  EXPECT_FLOAT_EQ(command.x, 0.0F);
  EXPECT_FLOAT_EQ(command.r, 0.0F);

  twist.twist.linear.x = 0.0;
  twist.twist.angular.z = 0.0;
  const auto neutral = mowgli_mavros_bridge::rover_manual_control_from_twist(twist, 1000.0, 1000.0);
  EXPECT_FLOAT_EQ(neutral.y, 0.0F);
  EXPECT_FLOAT_EQ(neutral.z, 0.0F);
}
TEST(RoverManualControl, RoverAxesAreBounded)
{
  geometry_msgs::msg::TwistStamped twist{};
  twist.twist.linear.x = 100.0;
  twist.twist.angular.z = -100.0;
  const auto command = mowgli_mavros_bridge::rover_manual_control_from_twist(twist, 1000.0, 1000.0);
  EXPECT_EQ(command.y, -1000);
  EXPECT_EQ(command.z, 1000);
}
