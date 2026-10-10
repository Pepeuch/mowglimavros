#include "mowgli_mavros_bridge/blade_off_control.hpp"
#include <gtest/gtest.h>

using mowgli_mavros_bridge::BladeOffControl;
using Decision = BladeOffControl::Decision;

TEST(BladeOff, NativeNeutralCommandOnly)
{
  BladeOffControl control;
  const auto command = control.command();
  EXPECT_EQ(command.id, 183);
  EXPECT_FLOAT_EQ(command.channel, 3);
  EXPECT_FLOAT_EQ(command.pwm, 1500);
  EXPECT_EQ(control.decide(false), Decision::SendNeutral);
  EXPECT_EQ(control.decide(true), Decision::OnInhibited);
}

TEST(BladeOff, ConfigurableWithoutDirectionOrArming)
{
  const auto command = BladeOffControl(4, 1490).command();
  EXPECT_FLOAT_EQ(command.channel, 4);
  EXPECT_FLOAT_EQ(command.pwm, 1490);
  EXPECT_THROW(BladeOffControl(0, 1500), std::invalid_argument);
  EXPECT_THROW(BladeOffControl(33, 1500), std::invalid_argument);
  EXPECT_THROW(BladeOffControl(3, 999), std::invalid_argument);
  EXPECT_THROW(BladeOffControl(3, 2001), std::invalid_argument);
}

TEST(BladeOff, NoSpamPendingOrConfirmed)
{
  BladeOffControl control;
  const auto generation = control.begin();
  for (unsigned tick = 0; tick < 100; ++tick)
  {
    EXPECT_EQ(control.decide(false), Decision::AlreadyAccepted);
    EXPECT_EQ(control.decide(true), Decision::OnInhibited);
  }
  control.complete(generation, true);
  EXPECT_EQ(control.decide(false), Decision::AlreadyAccepted);
}

TEST(BladeOff, RejectionDoesNotBecomeACommandLoop)
{
  BladeOffControl control;
  control.complete(control.begin(), false);
  for (unsigned tick = 0; tick < 100; ++tick)
  {
    EXPECT_EQ(control.decide(false), Decision::Failed);
  }
  EXPECT_EQ(control.decide(true), Decision::OnInhibited);
}

TEST(BladeOff, ReconnectInvalidatesOldAckAndAllowsFreshNeutral)
{
  BladeOffControl control;
  const auto old_generation = control.begin();
  control.reset_connection();
  const auto new_generation = control.begin();
  control.complete(old_generation, false);
  EXPECT_EQ(control.decide(false), Decision::AlreadyAccepted);
  control.complete(new_generation, true);
  EXPECT_EQ(control.decide(false), Decision::AlreadyAccepted);
  control.reset_connection();
  EXPECT_EQ(control.decide(false), Decision::SendNeutral);
  control.complete(old_generation, true);
  EXPECT_EQ(control.decide(false), Decision::SendNeutral);
}
