#include <array>
#include <cmath>

#include <gtest/gtest.h>

#include "mavros_esc_wheel_odometry/wheel_odometry_core.hpp"

namespace
{
using mavros_esc_wheel_odometry::WheelGeometry;
using mavros_esc_wheel_odometry::WheelOdometryCore;

WheelOdometryCore make_core()
{
  return WheelOdometryCore(WheelGeometry{0, 1, 0.1, 0.1, 0.5});
}

void baseline(WheelOdometryCore & core, uint16_t left = 10, uint16_t right = 20)
{
  core.receive_esc_counts(0, {left, right, 0, 0});
}

TEST(WheelOdometryCore, InitialBaselineDoesNotPublish)
{
  auto core = make_core();
  baseline(core);
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(WheelOdometryCore, InvalidGeometryCannotPublish)
{
  WheelOdometryCore core(WheelGeometry{0, 1, 0.0, 0.1, 0.5});
  EXPECT_FALSE(core.valid());
  core.receive_esc_counts(0, {10, 20, 0, 0});
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(WheelOdometryCore, RequiresBothCountersThenOneSubsequentRpm)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 20, 0, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(10.0, 10.0, 2);
  ASSERT_TRUE(observation);
  EXPECT_EQ(observation->receipt_stamp_ns, 2);
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 3));
}

TEST(WheelOdometryCore, RightCounterOnlyDoesNotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {10, 21, 0, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(WheelOdometryCore, LeftCounterOnlyDoesNotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 20, 0, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(WheelOdometryCore, RpmBeforeCountersIsRejected)
{
  auto core = make_core();
  baseline(core);
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_TRUE(core.receive_rpm(10.0, 10.0, 2));
}

TEST(WheelOdometryCore, IdenticalCountsAndFrozenZeroDoNotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {10, 20, 0, 0});
  EXPECT_FALSE(core.receive_rpm(0.0, 0.0, 1));
}

TEST(WheelOdometryCore, CounterWrapAndRealZeroAreFresh)
{
  auto core = make_core();
  baseline(core, 65535, 65535);
  core.receive_esc_counts(0, {0, 0, 0, 0});
  const auto observation = core.receive_rpm(0.0, 0.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_DOUBLE_EQ(observation->linear_x_mps, 0.0);
  EXPECT_DOUBLE_EQ(observation->angular_z_rps, 0.0);
}

TEST(WheelOdometryCore, CounterProgressionIsModuloUint16)
{
  EXPECT_TRUE(WheelOdometryCore::counter_advanced(65535, 0));
  EXPECT_TRUE(WheelOdometryCore::counter_advanced(10, 9));
  EXPECT_FALSE(WheelOdometryCore::counter_advanced(10, 10));
}

TEST(WheelOdometryCore, RepeatedGenuineRpmValuesRemainFresh)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_TRUE(core.receive_rpm(20.0, 20.0, 1));
  core.receive_esc_counts(0, {12, 22, 0, 0});
  EXPECT_TRUE(core.receive_rpm(20.0, 20.0, 2));
}

TEST(WheelOdometryCore, FrozenCountersAfterObservationDoNotRepublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_TRUE(core.receive_rpm(0.0, 0.0, 1));
  EXPECT_FALSE(core.receive_rpm(0.0, 0.0, 2));
}

TEST(WheelOdometryCore, SignedDifferentialKinematics)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 999, 999});
  const auto forward = core.receive_rpm(60.0, 60.0, 1);
  ASSERT_TRUE(forward);
  EXPECT_NEAR(forward->linear_x_mps, 0.2 * M_PI, 1e-12);
  EXPECT_NEAR(forward->angular_z_rps, 0.0, 1e-12);

  core.receive_esc_counts(0, {12, 22, 1000, 1000});
  const auto reverse = core.receive_rpm(-60.0, -60.0, 2);
  ASSERT_TRUE(reverse);
  EXPECT_NEAR(reverse->linear_x_mps, -0.2 * M_PI, 1e-12);

  core.receive_esc_counts(0, {13, 23, 1001, 1001});
  const auto rotation = core.receive_rpm(-60.0, 60.0, 3);
  ASSERT_TRUE(rotation);
  EXPECT_NEAR(rotation->linear_x_mps, 0.0, 1e-12);
  EXPECT_NEAR(rotation->angular_z_rps, 0.8 * M_PI, 1e-12);
}

TEST(WheelOdometryCore, UnequalRpmProducesCurvedMotion)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto curved = core.receive_rpm(60.0, 120.0, 1);
  ASSERT_TRUE(curved);
  EXPECT_GT(curved->linear_x_mps, 0.0);
  EXPECT_GT(curved->angular_z_rps, 0.0);
}

TEST(WheelOdometryCore, EqualNegativeRpmProducesReverse)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(-60.0, -60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_LT(observation->linear_x_mps, 0.0);
  EXPECT_NEAR(observation->angular_z_rps, 0.0, 1e-12);
}

TEST(WheelOdometryCore, OppositeSignedRpmRotatesInPlace)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(-60.0, 60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_NEAR(observation->linear_x_mps, 0.0, 1e-12);
  EXPECT_GT(observation->angular_z_rps, 0.0);
}

TEST(WheelOdometryCore, PhysicalRadiusIsAppliedPerWheel)
{
  WheelOdometryCore core(WheelGeometry{0, 1, 0.1, 0.2, 0.5});
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(60.0, 60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_NEAR(observation->linear_x_mps, 0.3 * M_PI, 1e-12);
}

TEST(WheelOdometryCore, TrackWidthScalesAngularVelocity)
{
  WheelOdometryCore core(WheelGeometry{0, 1, 0.1, 0.1, 1.0});
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(-60.0, 60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_NEAR(observation->angular_z_rps, 0.4 * M_PI, 1e-12);
}

TEST(WheelOdometryCore, ThirdEscIsIgnoredAndResetRequiresNewBaseline)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {10, 20, 1, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
  core.reset();
  core.receive_esc_counts(0, {11, 21, 1, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 2));
  core.receive_esc_counts(0, {12, 22, 1, 0});
  EXPECT_TRUE(core.receive_rpm(10.0, 10.0, 3));
}

TEST(WheelOdometryCore, DelayedPreResetRpmCannotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  core.reset();
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(WheelOdometryCore, GroupOffsetsMapExactSlots)
{
  WheelOdometryCore core(WheelGeometry{4, 8, 0.1, 0.1, 0.5});
  core.receive_esc_counts(4, {10, 0, 0, 0});
  core.receive_esc_counts(8, {20, 0, 0, 0});
  core.receive_esc_counts(4, {11, 0, 0, 0});
  core.receive_esc_counts(8, {21, 0, 0, 0});
  EXPECT_TRUE(core.receive_rpm(10.0, 10.0, 1));
}

}  // namespace
