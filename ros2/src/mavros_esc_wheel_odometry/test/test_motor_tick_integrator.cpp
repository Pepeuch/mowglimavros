#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include "mavros_esc_wheel_odometry/motor_tick_integrator.hpp"
using namespace mavros_esc_wheel_odometry;
namespace
{
constexpr int64_t S = 1000000000LL;
TEST(MotorTicks, FractionalSignedMotorRevolutionsAreNotQuantized)
{
  MotorTickIntegrator t;
  ASSERT_TRUE(t.observe(0, 15, S, 10 * S));
  ASSERT_TRUE(t.observe(0, 15, 2 * S, 10 * S + 1));
  ASSERT_TRUE(t.observe(1, -30, S, 10 * S));
  ASSERT_TRUE(t.observe(1, -30, 2 * S, 10 * S + 1));
  EXPECT_DOUBLE_EQ(t.state().left_ticks, 0.25);
  EXPECT_DOUBLE_EQ(t.state().right_ticks, -0.5);
}
TEST(MotorTicks, IntegratesAccelerationAndReverseWithoutGearRatio)
{
  MotorTickIntegrator t;
  t.observe(0, 0, S, S); t.observe(0, 120, 2 * S, 2 * S);
  EXPECT_DOUBLE_EQ(t.state().left_ticks, 1);
  t.observe(0, -120, 3 * S, 3 * S);
  EXPECT_DOUBLE_EQ(t.state().left_ticks, 1);
  t.observe(0, -120, 4 * S, 4 * S);
  EXPECT_DOUBLE_EQ(t.state().left_ticks, -1);
}
TEST(MotorTicks, SeparateWheelMeasurementIntervalsIgnoreReceiptJitter)
{
  MotorTickIntegrator t;
  t.observe(0, 60, S, 10 * S); t.observe(1, 120, S, 10 * S);
  t.observe(0, 60, 2 * S, 10 * S + 1); t.observe(1, 120, 3 * S, 10 * S + 2);
  EXPECT_DOUBLE_EQ(t.state().left_ticks, 1); EXPECT_DOUBLE_EQ(t.state().right_ticks, 4);
  auto m = t.motion(10, WheelSource::EscStatus); ASSERT_TRUE(m);
  EXPECT_DOUBLE_EQ(m->left_mps, 0.1); EXPECT_DOUBLE_EQ(m->right_mps, 0.2);
}
TEST(MotorTicks, NoCalibrationDisablesMetricsButRetainsRawTicks)
{
  MotorTickIntegrator t;
  for (unsigned i : {0U, 1U}) {
    t.observe(i, 60, S, S); t.observe(i, 60, 2 * S, 2 * S);
                                                                                 }
  EXPECT_FALSE(t.motion(0, WheelSource::ArduPilotLegacy));
  EXPECT_FALSE(t.motion(-1, WheelSource::ArduPilotLegacy));
  EXPECT_DOUBLE_EQ(t.state().left_ticks, 1);
  auto m = t.motion(2, WheelSource::ArduPilotLegacy); ASSERT_TRUE(m);
  EXPECT_DOUBLE_EQ(m->left_mps, 0.5);
  // A known straight RTK distance would fit ticks_per_meter=delta_ticks/distance.
  EXPECT_DOUBLE_EQ(t.state().left_ticks / 0.5, 2);
}
TEST(MotorTicks, FrozenRegressingAndInvalidSamplesDoNotMutateTicks)
{
  MotorTickIntegrator t; t.observe(0, 60, S, S); t.observe(0, 60, 2 * S, 2 * S);
  EXPECT_FALSE(t.observe(0, 120, 2 * S, 3 * S)); EXPECT_FALSE(t.observe(0, 120, S, 3 * S));
  EXPECT_FALSE(t.observe(0, 120, 3 * S, S));
  EXPECT_FALSE(t.observe(0, std::numeric_limits<double>::quiet_NaN(), 3 * S, 3 * S));
  EXPECT_FALSE(t.observe(0, std::numeric_limits<double>::infinity(), 3 * S, 3 * S));
  EXPECT_DOUBLE_EQ(t.state().left_ticks, 1);
}
TEST(MotorTicks, StaleGapStartsNewSegmentWithoutInventingTravel)
{
  MotorTickIntegrator t; t.observe(0, 60, S, S); t.observe(0, 60, 2 * S, 2 * S);
  t.observe(0, 600, 10 * S, 10 * S); EXPECT_DOUBLE_EQ(t.state().left_ticks, 1);
  EXPECT_EQ(t.state().left_segment, 2U);
  t.observe(0, 600, 11 * S, 11 * S); EXPECT_DOUBLE_EQ(t.state().left_ticks, 11);
  t.reset(); EXPECT_DOUBLE_EQ(t.state().left_ticks, 0); EXPECT_EQ(t.state().epoch, 1U);
}
TEST(MotorTicks, RetentionPreservesRawCountsWhileNewTimeoutApplies)
{
  MotorTickIntegrator old; old.observe(0, 60, S, S); old.observe(0, 60, 2 * S, 2 * S);
  MotorTickIntegrator next(S / 2); next.retain_counts(old);
  next.observe(0, 60, 3 * S, 3 * S);
  EXPECT_DOUBLE_EQ(next.state().left_ticks, 1); EXPECT_EQ(next.state().left_segment, 2U);
}
}  // namespace
