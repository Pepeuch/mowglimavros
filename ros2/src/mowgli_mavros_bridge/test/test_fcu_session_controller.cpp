#include "mowgli_mavros_bridge/fcu_session_controller.hpp"
#include <gtest/gtest.h>
using Adapter = mowgli_mavros_bridge::FcuSessionController;
using Kind = Adapter::Kind;
struct FcuTest : testing::Test
{
  Adapter c;
  void idle()
  {
    c.observe(true, false, "HOLD");
  }
  Adapter::Command next(int64_t t)
  {
    auto op = c.next(t, true, true, true);
    EXPECT_TRUE(op);
    return op.value();
  }
  void run(const std::string& mode = "GUIDED")
  {
    idle();
    c.demand(mode, true);
    auto set = next(1);
    EXPECT_EQ(set.kind, Kind::Mode);
    EXPECT_EQ(set.mode, mode);
    c.complete(set.serial, true, 2);
    EXPECT_FALSE(c.ready());
    EXPECT_FALSE(c.next(3, true, true, true));
    c.observe(true, false, mode);
    auto arm = next(4);
    EXPECT_EQ(arm.kind, Kind::Arm);
    EXPECT_TRUE(arm.arm);
    c.complete(arm.serial, true, 5);
    EXPECT_FALSE(c.ready());
    c.observe(true, true, mode);
    EXPECT_TRUE(c.ready());
  }
};
TEST_F(FcuTest, IdleDeduplicatesHoldDisarm)
{
  idle();
  for (int i = 0; i < 100; ++i)
    EXPECT_FALSE(c.next(i, true, true, true));
}
TEST_F(FcuTest, GuidedModeThenConfirmedArm)
{
  run();
}
TEST_F(FcuTest, ManualModeThenConfirmedArm)
{
  run("MANUAL");
}
TEST_F(FcuTest, BladeTestArmsInHold)
{
  idle();
  c.demand("HOLD", true);
  auto arm = next(1);
  EXPECT_EQ(arm.kind, Kind::Arm);
  c.complete(arm.serial, true, 2);
  EXPECT_FALSE(c.ready());
  c.observe(true, true, "HOLD");
  EXPECT_TRUE(c.ready());
}
TEST_F(FcuTest, StateBeforeAckAlsoWorks)
{
  idle();
  c.demand("HOLD", true);
  auto arm = next(1);
  c.observe(true, true, "HOLD");
  EXPECT_FALSE(c.ready());
  c.complete(arm.serial, true, 2);
  EXPECT_TRUE(c.ready());
}
TEST_F(FcuTest, ActiveTicksNeverRepeatRequests)
{
  run();
  for (int i = 6; i < 10000; ++i)
  {
    c.demand("GUIDED", true);
    EXPECT_FALSE(c.next(i, true, true, true));
  }
}
TEST_F(FcuTest, EndHoldThenDisarm)
{
  run();
  c.demand("HOLD", false);
  auto mode = next(6);
  EXPECT_EQ(mode.kind, Kind::Mode);
  c.complete(mode.serial, true, 7);
  EXPECT_FALSE(c.next(8, true, true, true));
  c.observe(true, true, "HOLD");
  auto arm = next(9);
  EXPECT_FALSE(arm.arm);
  c.complete(arm.serial, true, 10);
  c.observe(true, false, "HOLD");
  EXPECT_TRUE(c.ready());
}
TEST_F(FcuTest, RefusedArmDoesNotRetryOnTicks)
{
  idle();
  c.demand("HOLD", true);
  auto arm = next(1);
  c.complete(arm.serial, false, 2);
  EXPECT_EQ(c.error(), "arming:request_refused");
  for (int i = 3; i < 10000; ++i)
  {
    c.demand("HOLD", true);
    EXPECT_FALSE(c.next(i, true, true, true));
  }
}
TEST_F(FcuTest, ModeRefusalPreventsActivation)
{
  idle();
  c.demand("GUIDED", true);
  auto mode = next(1);
  c.complete(mode.serial, false, 2);
  EXPECT_TRUE(c.failed());
  for (int i = 3; i < 100; ++i)
    EXPECT_FALSE(c.next(i, true, true, true));
}
TEST_F(FcuTest, ModeTimeoutPreventsActivationWithoutRetry)
{
  idle();
  c.demand("GUIDED", true);
  next(1);
  c.tick(3002);
  EXPECT_EQ(c.error(), "mode:confirmation_timeout");
  EXPECT_FALSE(c.next(3003, true, true, true));
}
TEST_F(FcuTest, SafetyPreemptsPendingMode)
{
  idle();
  c.demand("GUIDED", true);
  auto old = next(1);
  c.demand("HOLD", false, true);
  auto disarm = next(2);
  EXPECT_EQ(disarm.kind, Kind::Arm);
  EXPECT_FALSE(disarm.arm);
  c.complete(old.serial, true, 3);
  EXPECT_TRUE(c.current(disarm.serial));
}
TEST_F(FcuTest, SafetyPreemptsPendingArmAndReconcilesLateArm)
{
  idle();
  c.demand("HOLD", true);
  auto old = next(1);
  c.demand("HOLD", false, true);
  auto stop = next(2);
  c.complete(old.serial, true, 3);
  EXPECT_TRUE(c.current(stop.serial));
  c.complete(stop.serial, true, 4);
  c.observe(true, true, "HOLD");
  auto corrective = next(5);
  EXPECT_FALSE(corrective.arm);
  c.complete(corrective.serial, true, 6);
  c.observe(true, false, "HOLD");
  EXPECT_FALSE(c.next(7, true, true, true));
}
TEST_F(FcuTest, SafetyDisarmSurvivesPriorModeTimeout)
{
  idle();
  c.demand("GUIDED", true);
  next(1);
  c.tick(3002);
  c.demand("HOLD", false, true);
  auto disarm = next(3003);
  EXPECT_EQ(disarm.kind, Kind::Arm);
  EXPECT_FALSE(disarm.arm);
}
TEST_F(FcuTest, HoldFailureStillDisarms)
{
  run();
  c.demand("HOLD", false);
  auto mode = next(6);
  c.complete(mode.serial, false, 7);
  auto arm = next(8);
  EXPECT_FALSE(arm.arm);
}
TEST_F(FcuTest, MissingHoldEndpointStillDisarms)
{
  c.observe(true, true, "MANUAL");
  auto cmd = c.next(1, false, true, true);
  ASSERT_TRUE(cmd);
  EXPECT_EQ(cmd->kind, Kind::Arm);
  EXPECT_FALSE(cmd->arm);
}
TEST_F(FcuTest, ReleasedEmergencyUsesCurrentMowgliNextDemand)
{
  run();
  c.demand("HOLD", false, true);
  auto disarm = next(6);
  c.complete(disarm.serial, true, 7);
  c.observe(true, false, "HOLD");
  c.demand("GUIDED", true);
  auto mode = next(8);
  EXPECT_EQ(mode.mode, "GUIDED");
}
TEST_F(FcuTest, ReconnectRetainsFunctionalDemandWithoutNewIntent)
{
  run();
  c.observe(false, false, "");
  EXPECT_FALSE(c.ready());
  EXPECT_FALSE(c.next(6, true, true, true));
  c.observe(true, false, "GUIDED");
  auto arm = next(7);
  EXPECT_TRUE(arm.arm);
}
TEST_F(FcuTest, SupersededRefusalDoesNotBlockNewModeOrDisarm)
{
  idle();
  c.demand("GUIDED", true);
  auto old = next(1);
  c.demand("MANUAL", true);
  EXPECT_FALSE(c.next(2, true, true, true));
  c.complete(old.serial, false, 3);
  auto mode = next(4);
  EXPECT_EQ(mode.mode, "MANUAL");
}
TEST_F(FcuTest, ExistingSafetyGateBlocksArmButNotDisarm)
{
  idle();
  c.demand("HOLD", true);
  EXPECT_FALSE(c.next(1, true, true, false));
  auto arm = next(2);
  EXPECT_TRUE(arm.arm);
  c.demand("HOLD", false, true);
  auto stop = c.next(3, true, true, false);
  ASSERT_TRUE(stop);
  EXPECT_FALSE(stop->arm);
}
TEST_F(FcuTest, ExternalDisarmReconcilesCurrentDemandOnce)
{
  run("MANUAL");
  c.observe(true, false, "MANUAL");
  auto arm = next(6);
  EXPECT_TRUE(arm.arm);
  for (int i = 7; i < 100; ++i)
    EXPECT_FALSE(c.next(i, true, true, true));
}

TEST_F(FcuTest, RefusedSafetyDisarmIsDiagnosedWithoutTickRetries)
{
  c.observe(true, true, "HOLD");
  c.demand("HOLD", false, true);
  const auto stop = next(1);
  c.complete(stop.serial, false, 2);
  EXPECT_EQ(c.error(), "arming:request_refused");
  for (int i = 3; i < 10000; ++i)
  {
    c.demand("HOLD", false, true);
    c.observe(true, true, "HOLD");
    EXPECT_FALSE(c.next(i, true, true, false));
  }
}

TEST_F(FcuTest, SafetyDisarmStillRunsAfterArmTimeout)
{
  idle();
  c.demand("HOLD", true);
  next(1);
  c.tick(3002);
  EXPECT_EQ(c.error(), "arming:confirmation_timeout");
  c.demand("HOLD", false, true);
  const auto stop = next(3003);
  EXPECT_EQ(stop.kind, Kind::Arm);
  EXPECT_FALSE(stop.arm);
}

TEST_F(FcuTest, SupersededArmRefusalCannotBlockSessionEndDisarm)
{
  idle();
  c.demand("HOLD", true);
  const auto arm = next(1);
  c.demand("HOLD", false);
  c.observe(true, true, "HOLD");
  c.complete(arm.serial, false, 2);
  const auto stop = next(3);
  EXPECT_EQ(stop.kind, Kind::Arm);
  EXPECT_FALSE(stop.arm);
}

TEST_F(FcuTest, LateModeConfirmationFulfillsCurrentDemandWithoutRetry)
{
  idle();
  c.demand("GUIDED", true);
  const auto mode = next(1);
  c.tick(3002);
  EXPECT_TRUE(c.failed());
  c.complete(mode.serial, true, 3003);
  EXPECT_FALSE(c.next(3004, true, true, true));
  c.observe(true, false, "GUIDED");
  const auto arm = next(3005);
  EXPECT_EQ(arm.kind, Kind::Arm);
  EXPECT_TRUE(arm.arm);
  c.complete(arm.serial, true, 3006);
  c.observe(true, true, "GUIDED");
  EXPECT_TRUE(c.ready());
  EXPECT_FALSE(c.failed());
}

TEST_F(FcuTest, PendingAcknowledgedAndObservedDisarmAreDistinct)
{
  c.observe(true, true, "MANUAL");
  c.demand("HOLD", false, true);
  auto disarm = c.next(1, false, true, false);
  ASSERT_TRUE(disarm);
  EXPECT_TRUE(c.disarm_pending());
  EXPECT_TRUE(c.command_pending());
  EXPECT_FALSE(c.command_acknowledged());
  c.complete(disarm->serial, true, 2);
  EXPECT_TRUE(c.command_acknowledged());
  EXPECT_TRUE(c.confirmed_arm());
  EXPECT_FALSE(c.ready());
  c.observe(true, false, "MANUAL");
  EXPECT_FALSE(c.command_pending());
  EXPECT_FALSE(c.command_acknowledged());
  EXPECT_FALSE(c.confirmed_arm());
  EXPECT_FALSE(c.ready());  // HOLD still missing
}
