#include <array>
#include <stdexcept>

#include "mowgli_mavros_bridge/firmware_provider.hpp"
#include "mowgli_mavros_bridge/rover_manual_control.hpp"
#include <gtest/gtest.h>

using mowgli_mavros_bridge::make_firmware_provider;

TEST(CommandProvider, SelectsConcreteFirmwareAndExplicitCapabilities)
{
  for (const auto* firmware : {"ardupilot", "px4"})
  {
    const auto provider = make_firmware_provider(firmware);
    EXPECT_STREQ(provider->name(), firmware);
    const auto caps = provider->capabilities();
    EXPECT_EQ(caps.manual_control_mapping,
              mowgli_mavros_bridge::ManualControlMapping::RoverSteeringYThrottleZ);
    EXPECT_TRUE(caps.manual_control_mapping_implemented);
    EXPECT_TRUE(caps.arm_implemented);
    EXPECT_TRUE(caps.disarm_implemented);
    EXPECT_TRUE(caps.mode_control_implemented);
    EXPECT_TRUE(caps.blade_control_implemented);
    EXPECT_FALSE(caps.actuation_validated);
  }
}

TEST(CommandProvider, RejectsUnresolvedLegacyUnknownAndUnimplementedFirmware)
{
  for (const auto* firmware : {"auto", "apm", "", "unknown", "betaflight", "inav", "mowgli"})
  {
    EXPECT_THROW(make_firmware_provider(firmware), std::invalid_argument);
  }
}

TEST(CommandProvider, BothProvidersPreserveExactExistingConversion)
{
  const std::array<double, 9> values{-100.0, -1.5, -0.35, -0.0019, 0.0, 0.0019, 0.2, 1.5, 100.0};
  for (const auto* firmware : {"ardupilot", "px4"})
  {
    const auto provider = make_firmware_provider(firmware);
    for (double linear : values)
    {
      for (double yaw : values)
      {
        for (double scale : {0.0, 75.5, 1000.0, -1000.0})
        {
          geometry_msgs::msg::TwistStamped twist{};
          twist.header.frame_id = "must_not_be_copied";
          twist.twist.linear.x = linear;
          twist.twist.angular.z = yaw;
          const auto expected =
              mowgli_mavros_bridge::rover_manual_control_from_twist(twist, scale, 1000.0);
          EXPECT_EQ(provider->manual_control_from_twist(twist, scale, 1000.0), expected);
        }
      }
    }
  }
}

TEST(CommandProvider, SteeringThrottleAndUnusedFieldsMatchCurrentRuntime)
{
  for (const auto* firmware : {"ardupilot", "px4"})
  {
    const auto provider = make_firmware_provider(firmware);
    geometry_msgs::msg::TwistStamped twist{};
    twist.twist.linear.x = 0.2;
    twist.twist.angular.z = -0.35;
    auto command = provider->manual_control_from_twist(twist, 1000.0, 1000.0);
    EXPECT_EQ(command.y, -350);
    EXPECT_EQ(command.z, 200);
    EXPECT_EQ(command.x, 0);
    EXPECT_EQ(command.r, 0);
    EXPECT_EQ(command.buttons, 0);
    twist.twist.linear.x = 100;
    twist.twist.angular.z = -100;
    command = provider->manual_control_from_twist(twist, 1000.0, 1000.0);
    EXPECT_EQ(command.y, -1000);
    EXPECT_EQ(command.z, 1000);
  }
}

TEST(CommandProvider, EmergencyPolicyPreservesDefaultsAndEveryOverride)
{
  for (const auto* firmware : {"ardupilot", "px4"})
  {
    const auto provider = make_firmware_provider(firmware);
    const auto defaults = provider->default_emergency_policy();
    EXPECT_EQ(defaults.mode, "HOLD");
    EXPECT_TRUE(defaults.disarm);
    for (const auto* mode : {"HOLD", "CUSTOM", "", "MANUAL"})
    {
      for (bool disarm : {false, true})
      {
        const auto policy = provider->emergency_policy(mode, disarm);
        EXPECT_EQ(policy.mode, mode);
        EXPECT_EQ(policy.disarm, disarm);
      }
    }
  }
}

TEST(FirmwareProvider, GuidedTransportIsSeparateAndBodyRelative)
{
  auto ap = mowgli_mavros_bridge::make_firmware_provider("ardupilot");
  auto px4 = mowgli_mavros_bridge::make_firmware_provider("px4");
  geometry_msgs::msg::TwistStamped twist;
  twist.twist.linear.x = 0.35;
  twist.twist.angular.z = -0.4;
  ASSERT_TRUE(ap->guided_velocity_implemented());
  const auto target = ap->guided_velocity_from_twist(twist);
  EXPECT_EQ(target.coordinate_frame, mavros_msgs::msg::PositionTarget::FRAME_BODY_NED);
  EXPECT_EQ(target.type_mask, 0x05E7);
  EXPECT_DOUBLE_EQ(target.velocity.x, 0.35);
  EXPECT_DOUBLE_EQ(target.velocity.y, 0.0);
  EXPECT_DOUBLE_EQ(target.velocity.z, 0.0);
  EXPECT_FLOAT_EQ(target.yaw_rate, -0.4F);
  EXPECT_FALSE(px4->guided_velocity_implemented());
  EXPECT_THROW(px4->guided_velocity_from_twist(twist), std::logic_error);
}
