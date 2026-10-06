#include <gtest/gtest.h>

#include "mavros_esc_wheel_odometry/wheel_tick_projection.hpp"

namespace mavros_esc_wheel_odometry {
TEST(WheelTickProjection,
     ProjectsSignedMotorRevolutionsWithoutMetricAssumptions) {
  WheelTickProjector projector;
  MotorTickState ticks;
  ticks.left_valid = ticks.right_valid = true;
  ticks.left_segment = ticks.right_segment = 1;
  ticks.left_sample_ns = ticks.right_sample_ns = 1;
  ASSERT_TRUE(projector.project(WheelSource::EscStatus, ticks));

  ticks.left_ticks = 1.25;
  ticks.right_ticks = -0.5;
  ticks.left_sample_ns = ticks.right_sample_ns = 2;
  const auto projected = projector.project(WheelSource::EscStatus, ticks);
  ASSERT_TRUE(projected);
  EXPECT_EQ(projected->left_count, 1250U);
  EXPECT_EQ(projected->left_direction, 1U);
  EXPECT_EQ(projected->right_count, 500U);
  EXPECT_EQ(projected->right_direction, 0U);
}

TEST(WheelTickProjection,
     SegmentAndSourceChangesRebaselineWithoutInventingTravel) {
  WheelTickProjector projector;
  MotorTickState ticks;
  ticks.left_valid = ticks.right_valid = true;
  ticks.left_segment = ticks.right_segment = 1;
  ticks.left_sample_ns = ticks.right_sample_ns = 1;
  ASSERT_TRUE(projector.project(WheelSource::EscStatus, ticks));
  ticks.left_ticks = ticks.right_ticks = 1.0;
  ticks.left_sample_ns = ticks.right_sample_ns = 2;
  ASSERT_TRUE(projector.project(WheelSource::EscStatus, ticks));

  ticks.left_segment = ticks.right_segment = 2;
  ticks.left_ticks = ticks.right_ticks = 100.0;
  const auto segment = projector.project(WheelSource::EscStatus, ticks);
  ASSERT_TRUE(segment);
  EXPECT_EQ(segment->left_count, 1000U);
  EXPECT_EQ(segment->right_count, 1000U);

  const auto source = projector.project(WheelSource::ArduPilotLegacy, ticks);
  ASSERT_TRUE(source);
  EXPECT_EQ(source->left_count, 1000U);
  EXPECT_EQ(source->right_count, 1000U);
}
TEST(WheelTickProjection, ReversePreservesMagnitudeAndKeepsDirectionAtRest) {
  WheelTickProjector p;
  MotorTickState t;
  t.left_valid = t.right_valid = true;
  t.left_sample_ns = t.right_sample_ns = 1;
  p.project(WheelSource::ArduPilotLegacy, t);
  t.left_ticks = 2;
  t.right_ticks = 3;
  t.left_sample_ns = t.right_sample_ns = 2;
  auto forward = p.project(WheelSource::ArduPilotLegacy, t);
  ASSERT_TRUE(forward);
  t.left_ticks = 1;
  t.right_ticks = 1;
  t.left_sample_ns = t.right_sample_ns = 3;
  auto reverse = p.project(WheelSource::ArduPilotLegacy, t);
  ASSERT_TRUE(reverse);
  EXPECT_EQ(reverse->left_count, 3000U);
  EXPECT_EQ(reverse->right_count, 5000U);
  EXPECT_EQ(reverse->left_direction, 0U);
  EXPECT_EQ(reverse->right_direction, 0U);
  t.left_sample_ns = t.right_sample_ns = 4;
  auto stationary = p.project(WheelSource::ArduPilotLegacy, t);
  ASSERT_TRUE(stationary);
  EXPECT_EQ(stationary->left_count, reverse->left_count);
  EXPECT_EQ(stationary->left_direction, 0U);
  EXPECT_FALSE(p.project(WheelSource::ArduPilotLegacy, t));
}
TEST(WheelTickProjection,
     ReconnectEpochAndInvalidWheelNeverResetPublishedCounts) {
  WheelTickProjector p;
  MotorTickState t;
  t.left_valid = t.right_valid = true;
  t.left_sample_ns = t.right_sample_ns = 1;
  p.project(WheelSource::EscStatus, t);
  t.left_ticks = t.right_ticks = 5;
  t.left_sample_ns = t.right_sample_ns = 2;
  p.project(WheelSource::EscStatus, t);
  EXPECT_FALSE(p.project(WheelSource::None, MotorTickState{}));
  t.epoch++;
  t.left_ticks = t.right_ticks = 0;
  t.left_sample_ns = t.right_sample_ns = 3;
  auto reconnect = p.project(WheelSource::EscStatus, t);
  ASSERT_TRUE(reconnect);
  EXPECT_EQ(reconnect->left_count, 5000U);
  EXPECT_EQ(reconnect->right_count, 5000U);
  t.left_valid = false;
  t.right_ticks = 1;
  t.right_sample_ns++;
  auto one = p.project(WheelSource::EscStatus, t);
  ASSERT_TRUE(one);
  EXPECT_FALSE(one->left_valid);
  EXPECT_TRUE(one->right_valid);
  EXPECT_EQ(one->right_count, 6000U);
  t.left_valid = true;
  t.left_ticks = 1000000;
  t.left_sample_ns++;
  auto restored = p.project(WheelSource::EscStatus, t);
  ASSERT_TRUE(restored);
  EXPECT_EQ(restored->left_count, 5000U);
}
TEST(WheelTickProjection,
     ScaleIsTransportOnlyAndCalibrationCannotChangeAccumulation) {
  WheelTickProjector p;
  MotorTickState t;
  t.left_valid = true;
  t.left_sample_ns = 1;
  p.project(WheelSource::ArduPilotLegacy, t);
  t.left_ticks = 0.125;
  t.left_sample_ns++;
  auto a = p.project(WheelSource::ArduPilotLegacy, t);
  ASSERT_TRUE(a);
  EXPECT_EQ(a->left_count, 125U);
  EXPECT_FLOAT_EQ(WheelTickProjection::factor(300), 300000.0F);
  EXPECT_NEAR(a->left_count / WheelTickProjection::factor(300), 0.125 / 300,
              1e-9);
  EXPECT_FLOAT_EQ(WheelTickProjection::factor(600), 600000.0F);
  t.left_ticks = 0.25;
  t.left_sample_ns++;
  auto b = p.project(WheelSource::ArduPilotLegacy, t);
  ASSERT_TRUE(b);
  EXPECT_EQ(b->left_count, 250U);
  EXPECT_FLOAT_EQ(WheelTickProjection::factor(0), 0.0F);
  EXPECT_FLOAT_EQ(
      WheelTickProjection::factor(std::numeric_limits<double>::infinity()),
      0.0F);
}
TEST(WheelTickProjection,
     FractionalAccumulationAndHugeDiscontinuityAreBounded) {
  WheelTickProjector p;
  MotorTickState t;
  t.left_valid = true;
  t.left_sample_ns = 1;
  p.project(WheelSource::EscStatus, t);
  for (int i = 1; i <= 10; ++i) {
    t.left_ticks = i * 0.00015;
    t.left_sample_ns++;
    auto sample = p.project(WheelSource::EscStatus, t);
    ASSERT_TRUE(sample);
    EXPECT_LE(sample->left_count, 2U);
  }
  t.left_ticks = 1e100;
  t.left_sample_ns++;
  EXPECT_FALSE(p.project(WheelSource::EscStatus, t));
  EXPECT_FALSE(p.project(WheelSource::WheelDistance, t));
}
} // namespace mavros_esc_wheel_odometry
