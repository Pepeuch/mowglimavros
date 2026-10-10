#include "mowgli_mavros_bridge/blade_control.hpp"
#include <gtest/gtest.h>

using B = mowgli_mavros_bridge::BladeControl;

class BladeTest : public ::testing::Test
{
protected:
  B b;
  int64_t time{10};
  uint32_t counter{0};
  void ack(int expected)
  {
    const auto command = b.next(time);
    ASSERT_TRUE(command);
    EXPECT_EQ(command->channel, 3);
    EXPECT_EQ(command->pwm, expected);
    b.complete(command->token, true, time);
  }
  void zeros()
  {
    for (int i = 0; i != 6; ++i)
    {
      time += 250;
      b.sample(2, time * 1000000, ++counter, true, true, 0, time);
    }
    ASSERT_TRUE(b.stopped(time));
  }
  void setup()
  {
    b.connection(true, time);
    b.permission(true, time);
    ack(1500);
    zeros();
  }
};

TEST_F(BladeTest, StartupAndOffDedup)
{
  setup();
  EXPECT_EQ(b.request(B::Direction::Off, time), B::Result::Confirmed);
  for (int i = 0; i < 10; ++i)
    EXPECT_FALSE(b.next(time));
  EXPECT_FALSE(b.enabled());
}
TEST_F(BladeTest, ForwardAndReverseConfigAndDedup)
{
  setup();
  EXPECT_EQ(b.request(B::Direction::Forward, time), B::Result::Pending);
  ack(1450);
  EXPECT_TRUE(b.enabled());
  EXPECT_EQ(b.request(B::Direction::Forward, time), B::Result::Confirmed);
  EXPECT_FALSE(b.next(time));
  b.request(B::Direction::Off, time);
  ack(1500);
  zeros();
  b.request(B::Direction::Reverse, time);
  ack(1550);
  EXPECT_TRUE(b.enabled());
  EXPECT_EQ(b.request(B::Direction::Reverse, time), B::Result::Confirmed);
  EXPECT_FALSE(b.next(time));
}
TEST_F(BladeTest, BothInversionsRequireNeutralAndRealStop)
{
  setup();
  b.request(B::Direction::Forward, time);
  ack(1450);
  for (const auto target : {B::Direction::Reverse, B::Direction::Forward})
  {
    b.request(target, time);
    ack(1500);
    EXPECT_EQ(b.state(), B::State::WAIT_STOP_BEFORE_REVERSE);
    EXPECT_FALSE(b.enabled());
    time += 100;
    b.sample(2, time * 1000000, ++counter, true, true, 100, time);
    EXPECT_FALSE(b.next(time));
    zeros();
    ack(target == B::Direction::Reverse ? 1550 : 1450);
  }
}
TEST_F(BladeTest, ReplayDoesNotProveStop)
{
  setup();
  b.force_off(time);
  ack(1500);
  const auto stamp = time * 1000000 + 1;
  b.sample(2, stamp, ++counter, true, true, 0, time);
  for (int i = 0; i < 20; ++i)
  {
    time += 100;
    b.sample(2, stamp, counter, true, true, 0, time);
  }
  EXPECT_FALSE(b.stopped(time));
  b.request(B::Direction::Forward, time);
  EXPECT_FALSE(b.next(time));
}
TEST_F(BladeTest, StaleGapAndWaitTimeoutNeverAllowOpposite)
{
  setup();
  b.request(B::Direction::Forward, time);
  ack(1450);
  b.request(B::Direction::Reverse, time);
  ack(1500);
  time += 15100;
  b.tick(time);
  EXPECT_TRUE(b.failed());
  ack(1500);  // stale observation triggers a neutral, never the opposite PWM
  EXPECT_FALSE(b.next(time));
  EXPECT_EQ(b.requested(), B::Direction::Off);
}
TEST_F(BladeTest, RejectedOnAndAckTimeoutQueueSingleNeutral)
{
  setup();
  b.request(B::Direction::Forward, time);
  auto command = b.next(time);
  ASSERT_TRUE(command);
  b.complete(command->token, false, time);
  EXPECT_TRUE(b.failed());
  ack(1500);
  EXPECT_FALSE(b.next(time));
  EXPECT_EQ(b.request(B::Direction::Forward, time), B::Result::Rejected);
  b.force_off(time);
  ack(1500);
  zeros();
  b.request(B::Direction::Forward, time);
  command = b.next(time);
  ASSERT_TRUE(command);
  time += 3001;
  b.tick(time);
  EXPECT_TRUE(b.failed());
  b.complete(command->token, true, time);
  EXPECT_TRUE(b.failed());
  ack(1500);
}
TEST_F(BladeTest, SafetyAndConnectionFenceOldCallbacks)
{
  setup();
  b.request(B::Direction::Forward, time);
  auto command = b.next(time);
  ASSERT_TRUE(command);
  b.permission(false, time);
  b.complete(command->token, true, time);
  ack(1500);
  EXPECT_FALSE(b.enabled());
  EXPECT_EQ(b.request(B::Direction::Forward, time), B::Result::Rejected);
  b.connection(false, time);
  ack(1500);  // attempted neutral independently of connection availability
  EXPECT_EQ(b.request(B::Direction::Off, time), B::Result::Rejected);
  b.connection(true, time);
  ack(1500);
}
TEST_F(BladeTest, ExternalOutputInvalidatesConfirmation)
{
  setup();
  b.output(1550, time);
  EXPECT_FALSE(b.satisfied());
  ack(1500);
}

TEST_F(BladeTest, OffPreemptsPendingOnWithoutWaitingForItsAck)
{
  setup();
  b.request(B::Direction::Forward, time);
  const auto on = b.next(time);
  ASSERT_TRUE(on);
  b.request(B::Direction::Off, time);
  const auto off = b.next(time);
  ASSERT_TRUE(off);
  EXPECT_EQ(off->pwm, 1500);
  b.complete(on->token, true, time);
  EXPECT_FALSE(b.enabled());
  b.complete(off->token, true, time);
  EXPECT_TRUE(b.satisfied());
}

TEST_F(BladeTest, StalePendingOnImmediatelyNeutralizesAndFencesItsAck)
{
  setup();
  b.request(B::Direction::Forward, time);
  const auto on = b.next(time);
  ASSERT_TRUE(on);
  time += 1001;
  b.tick(time);
  EXPECT_TRUE(b.failed());
  const auto off = b.next(time);
  ASSERT_TRUE(off);
  EXPECT_EQ(off->pwm, 1500);
  b.complete(on->token, true, time);
  EXPECT_FALSE(b.enabled());
  b.complete(off->token, true, time);
}

TEST_F(BladeTest, StopDeadlineCannotCancelAProvenStopWhileOnAckIsPending)
{
  setup();
  b.request(B::Direction::Forward, time);
  ack(1450);
  b.request(B::Direction::Reverse, time);
  ack(1500);
  for (int i = 0; i < 150; ++i)
  {
    time += 90;
    b.sample(2, time * 1000000, ++counter, true, true, 200, time);
  }
  for (int i = 0; i < 6; ++i)
  {
    time += 200;
    b.sample(2, time * 1000000, ++counter, true, true, 0, time);
  }
  const auto on = b.next(time);
  ASSERT_TRUE(on);
  EXPECT_EQ(on->pwm, 1550);
  time += 400;
  b.tick(time);
  EXPECT_FALSE(b.failed());
  b.complete(on->token, true, time);
  EXPECT_TRUE(b.enabled());
}
TEST_F(BladeTest, ConfigurableNoDirectionInference)
{
  B::Config c;
  c.forward = 1600;
  c.reverse = 1400;
  B other(c);
  other.connection(true, time);
  other.permission(true, time);
  auto command = other.next(time);
  other.complete(command->token, true, time);
  for (int i = 0; i < 6; ++i)
  {
    time += 250;
    other.sample(1, time * 1000000, 0, false, true, 0, time);
  }
  other.request(B::Direction::Forward, time);
  command = other.next(time);
  ASSERT_TRUE(command);
  EXPECT_EQ(command->pwm, 1600);
  c.forward = 1500;
  EXPECT_THROW(B invalid(c), std::invalid_argument);
}

TEST_F(BladeTest, ServiceDeadlineNeutralizesAndKeepsFailureLatched)
{
  setup();
  b.request(B::Direction::Forward, time);
  const auto on = b.next(time);
  ASSERT_TRUE(on);
  b.service_timeout(time);
  ack(1500);
  b.complete(on->token, true, time);
  EXPECT_TRUE(b.failed());
  EXPECT_FALSE(b.enabled());
  EXPECT_EQ(b.request(B::Direction::Forward, time), B::Result::Rejected);
  EXPECT_FALSE(b.next(time));
}

TEST_F(BladeTest, UnknownSourceAndCounterlessLegacyCannotEstablishStop)
{
  b.connection(true, time);
  b.permission(true, time);
  ack(1500);
  for (int i = 0; i < 6; ++i)
  {
    time += 250;
    b.sample(0, time * 1000000, ++counter, true, true, 0, time);
    b.sample(2, time * 1000000, counter, false, true, 0, time);
  }
  EXPECT_FALSE(b.stopped(time));
  EXPECT_EQ(b.request(B::Direction::Forward, time), B::Result::Rejected);
}

TEST_F(BladeTest, OffAtTenHzDeduplicatesButSafetyNeutralIsNeverSuppressed)
{
  setup();
  for (int i = 0; i < 30; ++i)
  {
    time += 100;
    b.sample(2, time * 1000000, ++counter, true, true, 0, time);
    EXPECT_EQ(b.request(B::Direction::Off, time), B::Result::Confirmed);
    EXPECT_FALSE(b.next(time));
  }
  b.force_off(time);  // A new safety demand overrides the normal OFF cache.
  ack(1500);
  b.force_off(time);
  ack(1500);
}

TEST_F(BladeTest, RepeatedUnauthorizedOnHasNoCommandAndNoArmingAuthority)
{
  setup();
  b.permission(false, time);
  ack(1500);  // Permission loss explicitly neutralizes once.
  for (int i = 0; i < 30; ++i)
  {
    time += 100;
    b.sample(2, time * 1000000, ++counter, true, true, 0, time);
    EXPECT_EQ(b.request(B::Direction::Forward, time), B::Result::Rejected);
    EXPECT_FALSE(b.next(time));
  }
}
