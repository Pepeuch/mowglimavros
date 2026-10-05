#include <array>
#include <cmath>

#include <gtest/gtest.h>

#include "mavros_esc_wheel_odometry/wheel_odometry_core.hpp"
#include "mavros_esc_wheel_odometry/legacy_wheel_adapter.hpp"

namespace
{
using mavros_esc_wheel_odometry::WheelGeometry;
// Same twenty legacy assertions, now testing the adapter + generic core pipeline.
class WheelOdometryCore
{
public:
  explicit WheelOdometryCore(WheelGeometry geometry)
  : adapter_(geometry), core_(geometry), ticks_(), tpm_(geometry.ticks_per_meter) {}
  bool valid() const {return adapter_.valid() && core_.valid() && tpm_ > 0;}
  void reset() {adapter_.reset(); core_.reset(); ticks_.reset();}
  static bool counter_advanced(uint16_t a, uint16_t b)
  {return mavros_esc_wheel_odometry::LegacyWheelAdapter::counter_advanced(a, b);}
  void receive_esc_counts(int offset, const std::array<uint16_t, 4> & counts)
  {adapter_.receive_esc_counts(offset, counts, 1);}
  std::optional<mavros_esc_wheel_odometry::WheelObservation> receive_rpm(
    double l, double r,
    int64_t stamp)
  {
    auto raw = adapter_.receive_rpm(l, r, stamp);
    if (!raw) {return std::nullopt;}
    ticks_.observe(0, raw->left_rpm, raw->sample_stamp_ns, raw->receipt_stamp_ns);
    ticks_.observe(1, raw->right_rpm, raw->sample_stamp_ns, raw->receipt_stamp_ns);
    auto motion = ticks_.motion(tpm_, mavros_esc_wheel_odometry::WheelSource::ArduPilotLegacy);
    return motion ? core_.receive_motion(*motion) : std::nullopt;
  }

private:
  mavros_esc_wheel_odometry::LegacyWheelAdapter adapter_;
  mavros_esc_wheel_odometry::WheelOdometryCore core_;
  mavros_esc_wheel_odometry::MotorTickIntegrator ticks_;
  double tpm_;
};

WheelOdometryCore make_core()
{
  return WheelOdometryCore(WheelGeometry{0, 1, 0.5, 1.0 / (0.2 * M_PI)});
}

void baseline(WheelOdometryCore & core, uint16_t left = 10, uint16_t right = 20)
{
  core.receive_esc_counts(0, {left, right, 0, 0});
}

TEST(LegacyPairingAndTicks, InitialBaselineDoesNotPublish)
{
  auto core = make_core();
  baseline(core);
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(LegacyPairingAndTicks, InvalidGeometryCannotPublish)
{
  WheelOdometryCore core(WheelGeometry{0, 1, 0.5, 0.0});
  EXPECT_FALSE(core.valid());
  core.receive_esc_counts(0, {10, 20, 0, 0});
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(LegacyPairingAndTicks, RequiresBothCountersThenOneSubsequentRpm)
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

TEST(LegacyPairingAndTicks, RightCounterOnlyDoesNotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {10, 21, 0, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(LegacyPairingAndTicks, LeftCounterOnlyDoesNotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 20, 0, 0});
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(LegacyPairingAndTicks, RpmBeforeCountersIsRejected)
{
  auto core = make_core();
  baseline(core);
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_TRUE(core.receive_rpm(10.0, 10.0, 2));
}

TEST(LegacyPairingAndTicks, IdenticalCountsAndFrozenZeroDoNotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {10, 20, 0, 0});
  EXPECT_FALSE(core.receive_rpm(0.0, 0.0, 1));
}

TEST(LegacyPairingAndTicks, CounterWrapAndRealZeroAreFresh)
{
  auto core = make_core();
  baseline(core, 65535, 65535);
  core.receive_esc_counts(0, {0, 0, 0, 0});
  const auto observation = core.receive_rpm(0.0, 0.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_DOUBLE_EQ(observation->linear_x_mps, 0.0);
  EXPECT_DOUBLE_EQ(observation->angular_z_rps, 0.0);
}

TEST(LegacyPairingAndTicks, CounterProgressionIsModuloUint16)
{
  EXPECT_TRUE(WheelOdometryCore::counter_advanced(65535, 0));
  EXPECT_TRUE(WheelOdometryCore::counter_advanced(10, 9));
  EXPECT_FALSE(WheelOdometryCore::counter_advanced(10, 10));
}

TEST(LegacyPairingAndTicks, RepeatedGenuineRpmValuesRemainFresh)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_TRUE(core.receive_rpm(20.0, 20.0, 1));
  core.receive_esc_counts(0, {12, 22, 0, 0});
  EXPECT_TRUE(core.receive_rpm(20.0, 20.0, 2));
}

TEST(LegacyPairingAndTicks, FrozenCountersAfterObservationDoNotRepublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  EXPECT_TRUE(core.receive_rpm(0.0, 0.0, 1));
  EXPECT_FALSE(core.receive_rpm(0.0, 0.0, 2));
}

TEST(LegacyPairingAndTicks, SignedDifferentialKinematics)
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

TEST(LegacyPairingAndTicks, UnequalRpmProducesCurvedMotion)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto curved = core.receive_rpm(60.0, 120.0, 1);
  ASSERT_TRUE(curved);
  EXPECT_GT(curved->linear_x_mps, 0.0);
  EXPECT_GT(curved->angular_z_rps, 0.0);
}

TEST(LegacyPairingAndTicks, EqualNegativeRpmProducesReverse)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(-60.0, -60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_LT(observation->linear_x_mps, 0.0);
  EXPECT_NEAR(observation->angular_z_rps, 0.0, 1e-12);
}

TEST(LegacyPairingAndTicks, OppositeSignedRpmRotatesInPlace)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(-60.0, 60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_NEAR(observation->linear_x_mps, 0.0, 1e-12);
  EXPECT_GT(observation->angular_z_rps, 0.0);
}

TEST(LegacyPairingAndTicks, TicksPerMeterConvertsBothMotorRates)
{
  WheelOdometryCore core(WheelGeometry{0, 1, 0.5, 1.0 / (0.3 * M_PI)});
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(60.0, 60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_NEAR(observation->linear_x_mps, 0.3 * M_PI, 1e-12);
}

TEST(LegacyPairingAndTicks, TrackWidthScalesAngularVelocity)
{
  WheelOdometryCore core(WheelGeometry{0, 1, 1.0, 1.0 / (0.2 * M_PI)});
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  const auto observation = core.receive_rpm(-60.0, 60.0, 1);
  ASSERT_TRUE(observation);
  EXPECT_NEAR(observation->angular_z_rps, 0.4 * M_PI, 1e-12);
}

TEST(LegacyPairingAndTicks, ThirdEscIsIgnoredAndResetRequiresNewBaseline)
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

TEST(LegacyPairingAndTicks, DelayedPreResetRpmCannotPublish)
{
  auto core = make_core();
  baseline(core);
  core.receive_esc_counts(0, {11, 21, 0, 0});
  core.reset();
  EXPECT_FALSE(core.receive_rpm(10.0, 10.0, 1));
}

TEST(LegacyPairingAndTicks, GroupOffsetsMapExactSlots)
{
  WheelOdometryCore core(WheelGeometry{4, 8, 0.5, 1.0 / (0.2 * M_PI)});
  core.receive_esc_counts(4, {10, 0, 0, 0});
  core.receive_esc_counts(8, {20, 0, 0, 0});
  core.receive_esc_counts(4, {11, 0, 0, 0});
  core.receive_esc_counts(8, {21, 0, 0, 0});
  EXPECT_TRUE(core.receive_rpm(10.0, 10.0, 1));
}

}  // namespace
