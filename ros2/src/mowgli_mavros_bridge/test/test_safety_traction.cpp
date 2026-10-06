#include "mowgli_mavros_bridge/safety_state.hpp"
#include <gtest/gtest.h>

using namespace mowgli_mavros_bridge;

TEST(SafetyTraction,
     PresenceDefinesUnknownEngagedReleasedAndReleaseClearsLatch) {
  SafetyState s;
  s.observe_motor_outputs(false, false);
  EXPECT_EQ(s.hardware_safety_state(), HardwareSafetyState::Unknown);
  EXPECT_FALSE(s.project(1, true).active_emergency);
  s.observe_motor_outputs(true, false);
  EXPECT_TRUE(s.project(1, true).latched_emergency);
  s.set_service_emergency(false);
  EXPECT_TRUE(s.project(1, true).active_emergency);
  s.observe_motor_outputs(true, true);
  EXPECT_FALSE(s.project(1, true).latched_emergency);
  s.set_service_emergency(true);
  EXPECT_TRUE(s.project(1, true).latched_emergency);
  s.set_service_emergency(false);
  EXPECT_FALSE(s.project(1, true).latched_emergency);
  s.disconnect();
  EXPECT_EQ(s.hardware_safety_state(), HardwareSafetyState::Unknown);
}

TEST(SafetyTraction, ExplicitEnableRequiresConnectedArmedAndReleasedSafety) {
  SafetyState s;
  EXPECT_FALSE(s.traction_allowed(true, true, true, 1, true));
  s.observe_motor_outputs(true, true);
  EXPECT_FALSE(s.traction_allowed(false, true, true, 1, true));
  EXPECT_FALSE(s.traction_allowed(true, false, true, 1, true));
  EXPECT_FALSE(s.traction_allowed(true, true, false, 1, true));
  EXPECT_TRUE(s.traction_allowed(true, true, true, 1, true));
  s.observe_motor_outputs(true, false);
  EXPECT_FALSE(s.traction_allowed(true, true, true, 1, false));
}

TEST(SafetyTraction,
     LiftTelemetrySurvivesDisabledProtectionAndHardwareStillBlocks) {
  SafetyState s;
  s.observe_motor_outputs(true, true);
  s.observe_button_change(0x01U, 1);
  EXPECT_TRUE(s.left_wheel_lifted());
  EXPECT_TRUE(s.project(2, true).lift_warning);
  s.observe_button_change(0x03U, 2);
  EXPECT_TRUE(s.right_wheel_lifted());
  EXPECT_FALSE(s.traction_allowed(true, true, true, 3, true));
  EXPECT_TRUE(s.traction_allowed(true, true, true, 3, false));
  EXPECT_TRUE(s.left_wheel_lifted());
  EXPECT_TRUE(s.right_wheel_lifted());
  s.observe_motor_outputs(true, false);
  EXPECT_FALSE(s.traction_allowed(true, true, true, 3, false));
}
