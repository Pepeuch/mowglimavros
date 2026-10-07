#include <cstdint>

#include "mowgli_mavros_bridge/button_change_decoder.hpp"
#include "mowgli_mavros_bridge/safety_state.hpp"
#include <gtest/gtest.h>

namespace
{

using mowgli_mavros_bridge::decode_button_change;
using mowgli_mavros_bridge::HardwareSafetyState;
using mowgli_mavros_bridge::SafetyState;

mavros_msgs::msg::Mavlink make_button_change(uint8_t state)
{
  mavlink::mavlink_message_t mavlink_message{};
  mavlink::MsgMap map(mavlink_message);
  mavlink::common::msg::BUTTON_CHANGE button_change;
  button_change.time_boot_ms = 100;
  button_change.last_change_ms = 90;
  button_change.state = state;
  button_change.serialize(map);

  mavros_msgs::msg::Mavlink ros_message;
  EXPECT_TRUE(mavros_msgs::mavlink::convert(mavlink_message, ros_message));
  return ros_message;
}

TEST(ButtonChangeDecoder, MapsBitZeroToLeftWheel)
{
  const auto state = decode_button_change(make_button_change(0x01U));
  ASSERT_TRUE(state.has_value());
  SafetyState safety;
  safety.observe_button_change(*state, 1000000000LL);
  EXPECT_TRUE(safety.left_wheel_lifted());
  EXPECT_FALSE(safety.right_wheel_lifted());
}

TEST(ButtonChangeDecoder, MapsBitOneToRightWheel)
{
  const auto state = decode_button_change(make_button_change(0x02U));
  ASSERT_TRUE(state.has_value());
  SafetyState safety;
  safety.observe_button_change(*state, 1000000000LL);
  EXPECT_FALSE(safety.left_wheel_lifted());
  EXPECT_TRUE(safety.right_wheel_lifted());
}

TEST(ButtonChangeDecoder, MapsBothWheelBits)
{
  const auto state = decode_button_change(make_button_change(0x03U));
  ASSERT_TRUE(state.has_value());
  SafetyState safety;
  safety.observe_button_change(*state, 1000000000LL);
  EXPECT_TRUE(safety.left_wheel_lifted());
  EXPECT_TRUE(safety.right_wheel_lifted());
  EXPECT_EQ(safety.raw_button_state(), 0x03U);
}

TEST(ButtonChangeDecoder, IgnoresInvalidFraming)
{
  auto message = make_button_change(0x01U);
  message.framing_status = mavros_msgs::msg::Mavlink::FRAMING_BAD_CRC;
  EXPECT_FALSE(decode_button_change(message).has_value());
}

TEST(ButtonChangeDecoder, IgnoresOtherMessageId)
{
  auto message = make_button_change(0x01U);
  message.msgid = mavlink::common::msg::BUTTON_CHANGE::MSG_ID + 1U;
  EXPECT_FALSE(decode_button_change(message).has_value());
}

TEST(SafetyState, DisconnectInvalidatesWheelLiftState)
{
  SafetyState safety;
  safety.observe_button_change(0x03U, 1000000000LL);
  safety.disconnect();
  EXPECT_FALSE(safety.wheel_lift_state_valid());
  EXPECT_TRUE(safety.left_wheel_lifted());
  EXPECT_TRUE(safety.right_wheel_lifted());
  EXPECT_EQ(safety.raw_button_state(), 0x03U);
  EXPECT_EQ(safety.last_button_change_ns(), 1000000000LL);
}

TEST(SafetyState, OneWheelWarnsAndTwoWheelsActivateEmergency)
{
  SafetyState safety;
  safety.observe_button_change(0x01U, 1000000000LL);
  auto emergency = safety.project(2500000000LL);
  EXPECT_TRUE(emergency.lift_warning);
  EXPECT_FALSE(emergency.active_emergency);
  EXPECT_FLOAT_EQ(emergency.lift_duration_sec, 1.5F);

  safety.observe_button_change(0x03U, 3000000000LL);
  emergency = safety.project(4000000000LL);
  EXPECT_FALSE(emergency.lift_warning);
  EXPECT_TRUE(emergency.active_emergency);
  EXPECT_FLOAT_EQ(emergency.lift_duration_sec, 3.0F);
}

TEST(SafetyState, HardwareSafetyAlwaysActivatesEmergency)
{
  SafetyState safety;
  safety.observe_motor_outputs(true, false);
  const auto emergency = safety.project(1000000000LL);
  EXPECT_EQ(safety.hardware_safety_state(), HardwareSafetyState::Engaged);
  EXPECT_TRUE(emergency.active_emergency);
  EXPECT_TRUE(emergency.latched_emergency);
  EXPECT_EQ(emergency.reason, "HARDWARE_SAFETY_SWITCH");
}

TEST(SafetyState, HardwareSafetyHasPriorityWithWheelTelemetryPresent)
{
  SafetyState safety;
  safety.observe_motor_outputs(true, false);
  safety.observe_button_change(0x00U, 1000000000LL);
  const auto emergency = safety.project(2000000000LL);
  EXPECT_TRUE(emergency.active_emergency);
  EXPECT_TRUE(emergency.latched_emergency);
  EXPECT_EQ(emergency.reason, "HARDWARE_SAFETY_SWITCH");
}

TEST(SafetyState, SoftwareClearCannotMaskEngagedHardwareSafety)
{
  SafetyState safety;
  safety.set_service_emergency(true);
  safety.observe_motor_outputs(true, false);
  safety.set_service_emergency(false);
  const auto emergency = safety.project(1000000000LL);
  EXPECT_TRUE(emergency.active_emergency);
  EXPECT_TRUE(emergency.latched_emergency);
  EXPECT_EQ(emergency.reason, "HARDWARE_SAFETY_SWITCH");
}

TEST(SafetyState, HardwareHasPriorityOverServiceAndWheelLift)
{
  SafetyState safety;
  safety.set_service_emergency(true);
  safety.observe_button_change(0x03U, 1000000000LL);
  safety.observe_motor_outputs(true, false);
  const auto emergency = safety.project(2000000000LL);
  EXPECT_EQ(emergency.reason, "HARDWARE_SAFETY_SWITCH");
}

TEST(SafetyState, ServiceEmergencyIsPreservedAcrossWheelCompositionAndClear)
{
  SafetyState safety;
  safety.observe_button_change(0x03U, 1000000000LL);
  safety.set_service_emergency(true);
  auto emergency = safety.project(2000000000LL);
  EXPECT_TRUE(emergency.active_emergency);
  EXPECT_TRUE(emergency.latched_emergency);
  EXPECT_EQ(emergency.reason, "SERVICE_EMERGENCY_STOP");

  safety.observe_button_change(0x00U, 3000000000LL);
  safety.set_service_emergency(false);
  emergency = safety.project(4000000000LL);
  EXPECT_FALSE(emergency.active_emergency);
  EXPECT_FALSE(emergency.latched_emergency);
  EXPECT_EQ(emergency.reason, "NONE");
}

TEST(SafetyState, AbsentMotorOutputsRemainUnknown)
{
  SafetyState safety;
  safety.observe_motor_outputs(false, false);
  const auto emergency = safety.project(1000000000LL);
  EXPECT_EQ(safety.hardware_safety_state(), HardwareSafetyState::Unknown);
  EXPECT_FALSE(emergency.active_emergency);
  EXPECT_FALSE(emergency.latched_emergency);
}

TEST(SafetyState, ReleasedSafetyClearsOnlyHardwareLatch)
{
  SafetyState safety;
  safety.observe_motor_outputs(true, false);
  safety.set_service_emergency(true);
  safety.observe_motor_outputs(true, true);
  const auto emergency = safety.project(1000000000LL);
  EXPECT_EQ(safety.hardware_safety_state(), HardwareSafetyState::Released);
  EXPECT_TRUE(emergency.active_emergency);
  EXPECT_TRUE(emergency.latched_emergency);
  EXPECT_EQ(emergency.reason, "SERVICE_EMERGENCY_STOP");
}

TEST(SafetyState, TiltStaysLatchedUntilPhysicalCauseAndServiceReleaseAreClear)
{
  SafetyState safety;
  safety.observe_tilt(true);
  auto emergency = safety.project(1000000000LL);
  EXPECT_TRUE(emergency.active_emergency);
  EXPECT_TRUE(emergency.latched_emergency);
  EXPECT_EQ(emergency.reason, "TILT");

  safety.observe_tilt(false);
  emergency = safety.project(2000000000LL);
  EXPECT_FALSE(emergency.active_emergency);
  EXPECT_TRUE(emergency.latched_emergency);

  safety.clear_released_latches();
  emergency = safety.project(3000000000LL);
  EXPECT_FALSE(emergency.active_emergency);
  EXPECT_FALSE(emergency.latched_emergency);
}

}  // namespace
