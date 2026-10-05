#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <stdexcept>
#include "mavros_esc_wheel_odometry/observation_engine.hpp"
using namespace mavros_esc_wheel_odometry;
namespace
{
constexpr int64_t S = 1000000000LL;
ObservationConfig config(std::string source = "auto")
{
  ObservationConfig c; c.geometry = {0, 1, 0.1, 0.1, 0.5};
  c.left_wheel_index = 0; c.right_wheel_index = 1; c.legacy_enabled = true; c.source = source;
  return c;
}
ObservationEngine engine(std::string source = "auto")
{ObservationEngine e(config(source)); e.connection(true); return e;}
CommonStatusPacket status(uint64_t us, int32_t left = 60, int32_t right = 60, unsigned index = 0)
{
  CommonStatusPacket p; p.time_us = us; p.index = index;
  p.rpm = {left, right, 0, 0}; p.voltage = {24, 25, 26, 0}; p.current = {1, 2, 3, 0}; return p;
}
DistancePacket distance(uint64_t us, double left, double right, uint8_t component = 201)
{
  DistancePacket p; p.time_us = us; p.count = 2; p.distance[0] = left; p.distance[1] = right;
  p.component_id = component; return p;
}
LegacyEscPacket legacy(uint16_t left, uint16_t right, uint16_t magnitude = 65000)
{
  LegacyEscPacket p; p.counts = {left, right, 0, 0}; p.rpm.fill(magnitude);
  p.voltage.fill(24); p.current.fill(2); p.temperature.fill(40); return p;
}
TEST(CommonEsc, SignedRpmAndVoltageCurrentWorkWithoutInfo)
{
  auto e = engine("esc_status"); auto out = e.common_status(status(1000000, -60, 60), S);
  ASSERT_TRUE(out); EXPECT_NEAR(out->linear_x_mps, 0, 1e-12);
      EXPECT_NEAR(out->angular_z_rps, 0.8 * M_PI, 1e-12);
  EXPECT_EQ(out->sample_stamp_ns, S); EXPECT_EQ(out->receipt_stamp_ns, S);
  auto esc = e.esc(0, S); EXPECT_TRUE(esc.rpm_direction_valid); EXPECT_EQ(esc.rpm, -60);
  EXPECT_TRUE(esc.voltage_valid); EXPECT_FLOAT_EQ(esc.voltage, 24);
  EXPECT_TRUE(esc.current_valid); EXPECT_FLOAT_EQ(esc.current, 1);
      EXPECT_FALSE(esc.temperature_valid);
}
TEST(CommonEsc, InfoScalingCountIndexFailuresAndUnavailableTemperature)
{
  auto e = engine(); e.common_status(status(1000000), S);
  CommonInfoPacket p; p.time_us = 1000000; p.count = 3; p.temperature = {2534, 32767, 0, 10000};
  p.failure_flags = {2, 3, 4, 5}; p.error_count = {10, 11, 12, 13}; e.common_info(p, S);
  EXPECT_NEAR(e.esc(0, S).temperature, 25.34, 1e-5); EXPECT_TRUE(e.esc(0, S).temperature_valid);
  EXPECT_FALSE(e.esc(1, S).temperature_valid); EXPECT_TRUE(e.esc(2, S).temperature_valid);
  EXPECT_FLOAT_EQ(e.esc(2, S).temperature, 0); EXPECT_FALSE(e.esc(3, S).valid);
  EXPECT_EQ(e.esc(2, S).failure_flags, 4); EXPECT_EQ(e.esc(2, S).error_count, 12);
  EXPECT_TRUE(e.esc(2, S).failure_flags_valid); EXPECT_TRUE(e.esc(2, S).error_count_valid);
}
TEST(CommonEsc, InfoAloneNeverMakesRpmOrCurrentValid)
{
  auto e = engine(); CommonInfoPacket p; p.time_us = 1000000; p.count = 3; p.temperature.fill(2000);
  e.common_info(p, S); const auto esc = e.esc(2, S);
  EXPECT_FALSE(esc.valid); EXPECT_FALSE(esc.rpm_valid); EXPECT_FALSE(esc.current_valid);
  EXPECT_TRUE(esc.temperature_valid); EXPECT_FLOAT_EQ(esc.temperature, 20);
  EXPECT_EQ(esc.source, EscSource::Common);
}
TEST(CommonEsc, IndependentInfoFreshnessAndFrozenTimestamps)
{
  auto e = engine(); CommonInfoPacket p; p.time_us = 1000000; p.count = 3; p.temperature.fill(2000);
  e.common_info(p, S); e.common_status(status(1000000), S);
  EXPECT_FALSE(e.common_status(status(1000000), 3 * S)); // same measurement cannot refresh
  EXPECT_FALSE(e.esc(0, 5 * S).valid);
  e.common_status(status(5000000), 5 * S);
  EXPECT_TRUE(e.esc(0, 5 * S).valid); EXPECT_FALSE(e.esc(0, 5 * S).temperature_valid);
}
TEST(CommonEsc, DifferentGroupsWaitForBothAndNeverDuplicate)
{
  auto c = config("esc_status"); c.geometry.left_esc_slot = 4; c.geometry.right_esc_slot = 8;
  ObservationEngine e(c); e.connection(true);
  EXPECT_FALSE(e.common_status(status(1000000, -60, 0, 4), S));
  auto out = e.common_status(status(1000000, 60, 0, 8), S + 1);
  ASSERT_TRUE(out); EXPECT_GT(out->angular_z_rps, 0); EXPECT_NEAR(out->linear_x_mps, 0, 1e-12);
  EXPECT_FALSE(e.common_status(status(1000000, 60, 0, 8), 2 * S));
  EXPECT_FALSE(e.common_status(status(2000000, -60, 0, 4), 2 * S));
  EXPECT_TRUE(e.common_status(status(2000000, 60, 0, 8), 2 * S + 1));
}
TEST(CommonEsc, RejectsInvalidIndexesAndContradictorySameMeasurement)
{
  auto e = engine("esc_status"); EXPECT_FALSE(e.common_status(status(1000000, 60, 60, 1), S));
  ASSERT_TRUE(e.common_status(status(1000000), S));
  EXPECT_FALSE(e.common_status(status(1000000, -60, 60), S + 1));
  EXPECT_FALSE(e.esc(0, S + 1).valid);
  EXPECT_FALSE(e.common_status(status(0), 2 * S));
}
TEST(CommonEsc, ClockResetDoesNotPublishTheResetSample)
{
  auto e = engine("esc_status"); ASSERT_TRUE(e.common_status(status(2000000), 2 * S));
  EXPECT_FALSE(e.common_status(status(100), 3 * S));
  EXPECT_TRUE(e.common_status(status(1000000), 4 * S));
}
TEST(CommonEsc, InvalidFloatFieldsHaveFlagsNotNaNSentinels)
{
  auto e = engine(); auto p = status(1000000);
      p.voltage[0] = std::numeric_limits<float>::quiet_NaN();
  p.current[0] = std::numeric_limits<float>::infinity(); e.common_status(p, S);
      auto out = e.esc(0, S);
  EXPECT_TRUE(out.rpm_valid); EXPECT_FALSE(out.voltage_valid); EXPECT_FALSE(out.current_valid);
  EXPECT_TRUE(std::isfinite(out.voltage)); EXPECT_TRUE(std::isfinite(out.current));
}
TEST(Distance, InitialBaselineThenForwardReverseAndRotation)
{
  auto e = engine("wheel_distance"); EXPECT_FALSE(e.wheel_distance(distance(1000000, 10, 10), S));
  auto forward = e.wheel_distance(distance(2000000, 11, 11), 2 * S); ASSERT_TRUE(forward);
  EXPECT_DOUBLE_EQ(forward->linear_x_mps, 1); EXPECT_DOUBLE_EQ(forward->angular_z_rps, 0);
  auto reverse = e.wheel_distance(distance(3000000, 10, 10), 3 * S); ASSERT_TRUE(reverse);
  EXPECT_DOUBLE_EQ(reverse->linear_x_mps, -1);
  auto turn = e.wheel_distance(distance(4000000, 9, 11), 4 * S); ASSERT_TRUE(turn);
  EXPECT_DOUBLE_EQ(turn->linear_x_mps, 0); EXPECT_DOUBLE_EQ(turn->angular_z_rps, 4);
}
TEST(Distance, MeasurementTimeNotReceiptJitterDeterminesSpeed)
{
  auto e = engine("wheel_distance"); e.wheel_distance(distance(1000000, 0, 0), 10 * S);
  auto out = e.wheel_distance(distance(2000000, 1, 1), 10 * S + 1000000); ASSERT_TRUE(out);
  EXPECT_DOUBLE_EQ(out->linear_x_mps, 1); EXPECT_EQ(out->sample_stamp_ns, 2 * S);
  EXPECT_EQ(out->receipt_stamp_ns, 10 * S + 1000000);
  e.poll(14 * S); EXPECT_EQ(e.active_source(), WheelSource::None); // receipt controls stale
}
TEST(Distance, ReconnectAndFcuClockResetRebaseline)
{
  auto e = engine(); e.wheel_distance(distance(1000000, 1000, 1000), S);
  e.connection(false); EXPECT_FALSE(e.wheel_distance(distance(2000000, 0, 0), 2 * S));
  e.connection(true); EXPECT_FALSE(e.wheel_distance(distance(100, 0, 0), 3 * S));
  ASSERT_TRUE(e.wheel_distance(distance(1000100, 1, 1), 4 * S));
  EXPECT_FALSE(e.wheel_distance(distance(100, 0, 0), 5 * S));
  EXPECT_TRUE(e.wheel_distance(distance(1000100, 1, 1), 6 * S));
}
TEST(Distance, StaleReappearanceAndComponentTakeoverStartNewBaselines)
{
  auto e = engine(); e.wheel_distance(distance(1000000, 100, 100, 201), S);
  EXPECT_FALSE(e.wheel_distance(distance(2000000, 0, 0, 202), 2 * S)); // live owner retained
  e.poll(5 * S); EXPECT_EQ(e.active_source(), WheelSource::None);
  EXPECT_FALSE(e.wheel_distance(distance(6000000, 0, 0, 202), 6 * S));
  EXPECT_TRUE(e.wheel_distance(distance(7000000, 1, 1, 202), 7 * S));
  auto c = config(); c.wheel_distance_component_id = 201; ObservationEngine fixed(c);
      fixed.connection(true);
  EXPECT_FALSE(fixed.wheel_distance(distance(1000000, 0, 0, 202), S));
      EXPECT_EQ(fixed.active_source(), WheelSource::None);
}
TEST(Distance, NonFiniteInsufficientCountAndTimeContradictions)
{
  auto e = engine("wheel_distance"); auto p = distance(1000000, 0, 0); p.count = 1;
  EXPECT_FALSE(e.wheel_distance(p, S)); p.count = 17; EXPECT_FALSE(e.wheel_distance(p, S));
  p = distance(1000000, std::numeric_limits<double>::quiet_NaN(), 0);
      EXPECT_FALSE(e.wheel_distance(p, S));
  p = distance(1000000, 0, std::numeric_limits<double>::infinity());
      EXPECT_FALSE(e.wheel_distance(p, S));
  EXPECT_EQ(e.active_source(), WheelSource::None);
  e.wheel_distance(distance(1000000, 0, 0), S);
  EXPECT_FALSE(e.wheel_distance(distance(1000000, 1, 1), 2 * S));
  EXPECT_EQ(e.active_source(), WheelSource::None);
  EXPECT_FALSE(e.wheel_distance(distance(0, 0, 0), 3 * S));
  EXPECT_FALSE(e.wheel_distance(distance(UINT64_MAX, 0, 0), 3 * S));
}
TEST(Distance, ImplausibleSilentCounterJumpIsRejected)
{
  auto e = engine(); e.wheel_distance(distance(1000000, 1000, 1000), S);
  EXPECT_FALSE(e.wheel_distance(distance(2000000, 0, 0), 2 * S));
  EXPECT_EQ(e.active_source(), WheelSource::None);
  EXPECT_FALSE(e.wheel_distance(distance(3000000, 1, 1), 3 * S));
  EXPECT_TRUE(e.wheel_distance(distance(4000000, 2, 2), 4 * S));
}
TEST(Sources, AutoPriorityFallbackReappearanceAndNoDoublePublication)
{
  auto e = engine(); unsigned publications = 0;
  e.legacy_esc(legacy(10, 20), S); e.legacy_esc(legacy(11, 21), 2 * S);
  publications += e.legacy_rpm(-60, -60, 2 * S).has_value();
      EXPECT_EQ(e.active_source(), WheelSource::ArduPilotLegacy);
  publications += e.common_status(status(2000000), 2 * S + 1).has_value();
      EXPECT_EQ(e.active_source(), WheelSource::EscStatus);
  EXPECT_FALSE(e.wheel_distance(distance(2000000, 0, 0), 2 * S + 2));
      EXPECT_EQ(e.active_source(), WheelSource::WheelDistance);
  EXPECT_FALSE(e.common_status(status(3000000), 3 * S));
  publications += e.wheel_distance(distance(3000000, 1, 1), 3 * S + 1).has_value();
  EXPECT_FALSE(e.wheel_distance(distance(3000000, 1, 1), 3 * S + 2));
  publications += e.common_status(status(7000000), 7 * S).has_value();
      EXPECT_EQ(e.active_source(), WheelSource::EscStatus);
  e.legacy_esc(legacy(12, 22), 11 * S);
  publications += e.legacy_rpm(-60, -60, 11 * S).has_value();
      EXPECT_EQ(e.active_source(), WheelSource::ArduPilotLegacy);
  EXPECT_FALSE(e.wheel_distance(distance(11000000, 500, 500), 11 * S + 1));
  publications += e.wheel_distance(distance(12000000, 499, 499), 12 * S).has_value();
  EXPECT_EQ(e.active_source(), WheelSource::WheelDistance); EXPECT_EQ(publications, 6U);
  e.poll(16 * S); EXPECT_EQ(e.active_source(), WheelSource::None);
}
TEST(Sources, ExplicitSelectionNeverFallsBack)
{
  auto e = engine("wheel_distance"); EXPECT_FALSE(e.common_status(status(1000000), S));
  e.legacy_esc(legacy(10, 20), S); e.legacy_esc(legacy(11, 21), 2 * S);
  EXPECT_FALSE(e.legacy_rpm(60, 60, 2 * S)); EXPECT_EQ(e.active_source(), WheelSource::None);
}
TEST(LegacySafety, UnsignedTelemetryRpmCannotCreateOrReverseWheelDirection)
{
  auto e = engine("ardupilot_legacy"); e.legacy_esc(legacy(10, 20, 65000), S);
  e.legacy_esc(legacy(11, 21, 0), 2 * S); EXPECT_EQ(e.active_source(), WheelSource::None);
  auto reverse = e.legacy_rpm(-60, -60, 2 * S); ASSERT_TRUE(reverse);
      EXPECT_LT(reverse->linear_x_mps, 0);
  e.legacy_esc(legacy(12, 22, 65000), 3 * S);
  auto forward = e.legacy_rpm(60, 60, 3 * S); ASSERT_TRUE(forward);
      EXPECT_GT(forward->linear_x_mps, 0);
}
TEST(LegacySafety, FrozenCountersAndStaleCounterRpmPairAreRejected)
{
  auto e = engine("ardupilot_legacy"); e.legacy_esc(legacy(65535, 65535), S);
  e.legacy_esc(legacy(0, 0), 2 * S); EXPECT_TRUE(e.legacy_rpm(0, 0, 2 * S));
  e.legacy_esc(legacy(0, 0), 6 * S); EXPECT_FALSE(e.legacy_rpm(0, 0, 6 * S));
  EXPECT_FALSE(e.esc(0, 6 * S).valid);
  e.legacy_esc(legacy(1, 1), 7 * S); EXPECT_FALSE(e.legacy_rpm(60, 60, 11 * S));
}
TEST(Sources, PhysicallyEquivalentInputsProduceSameDifferentialResult)
{
  auto l = engine("ardupilot_legacy"), c = engine("esc_status"), d = engine("wheel_distance");
  l.legacy_esc(legacy(1, 1), S); l.legacy_esc(legacy(2, 2), 2 * S);
  auto lo = l.legacy_rpm(-60, 120, 2 * S);
      auto co = c.common_status(status(2000000, -60, 120), 2 * S);
  const double left = rpm_to_mps(-60, 0.1), right = rpm_to_mps(120, 0.1);
  d.wheel_distance(distance(1000000, 0, 0), S);
      auto od = d.wheel_distance(distance(2000000, left, right), 2 * S);
  ASSERT_TRUE(lo); ASSERT_TRUE(co); ASSERT_TRUE(od);
  EXPECT_DOUBLE_EQ(lo->linear_x_mps, co->linear_x_mps);
      EXPECT_NEAR(lo->linear_x_mps, od->linear_x_mps, 1e-12);
  EXPECT_DOUBLE_EQ(lo->angular_z_rps, co->angular_z_rps);
      EXPECT_NEAR(lo->angular_z_rps, od->angular_z_rps, 1e-12);
}
TEST(Config, RejectsUnsafeMappingsAndAcceptsDistanceWithoutRadii)
{
  auto c = config(); c.geometry.left_esc_slot = c.geometry.right_esc_slot;
      EXPECT_THROW((void)ObservationEngine(c), std::invalid_argument);
  c = config(); c.left_wheel_index = c.right_wheel_index;
      EXPECT_THROW((void)ObservationEngine(c), std::invalid_argument);
  c = config(); c.geometry.track_width_m = std::numeric_limits<double>::infinity();
      EXPECT_THROW((void)ObservationEngine(c), std::invalid_argument);
  c = config(); c.geometry.left_radius_m = std::numeric_limits<double>::quiet_NaN();
      EXPECT_THROW((void)ObservationEngine(c), std::invalid_argument);
  c = config("wheel_distance"); c.geometry.left_radius_m = c.geometry.right_radius_m = 0;
  ObservationEngine e(c); e.connection(true); EXPECT_TRUE(e.wheel_configured());
  e.wheel_distance(distance(1000000, 0, 0), S);
      EXPECT_TRUE(e.wheel_distance(distance(2000000, 1, 1), 2 * S));
}
TEST(Core, MeasurementMonotonicityIsIndependentFromReceiptTime)
{
  WheelOdometryCore core(config().geometry);
  EXPECT_TRUE(core.receive_motion({1, 1, S, 10 * S, WheelSource::WheelDistance, true}));
  EXPECT_FALSE(core.receive_motion({1, 1, S, 11 * S, WheelSource::WheelDistance, true}));
  EXPECT_FALSE(core.receive_motion({1, 1, S - 1, 12 * S, WheelSource::WheelDistance, true}));
  EXPECT_TRUE(core.receive_motion({1, 1, 2 * S, 12 * S, WheelSource::WheelDistance, true}));
  EXPECT_FALSE(core.receive_motion({1, 1, 3 * S, 11 * S, WheelSource::WheelDistance, true}));
}
TEST(CommonEsc, MotorGearingIsConfiguredWithoutChangingSignedRawObservationOrLegacy)
{
  auto c = config("esc_status");
      c.left_esc_rpm_to_wheel_ratio = c.right_esc_rpm_to_wheel_ratio = 0.5;
  ObservationEngine e(c); e.connection(true);
      auto out = e.common_status(status(1000000, -120, -120), S);
  ASSERT_TRUE(out); EXPECT_NEAR(out->linear_x_mps, -0.2 * M_PI, 1e-12);
  EXPECT_EQ(e.esc(0, S).rpm, -120); EXPECT_TRUE(e.esc(0, S).rpm_direction_valid);
  c.source = "ardupilot_legacy"; ObservationEngine l(c); l.connection(true);
  l.legacy_esc(legacy(1, 1), S); l.legacy_esc(legacy(2, 2), 2 * S);
  auto lo = l.legacy_rpm(-60, -60, 2 * S); ASSERT_TRUE(lo);
  EXPECT_DOUBLE_EQ(lo->linear_x_mps, out->linear_x_mps);
}
TEST(CommonEsc, ContradictoryInfoInvalidatesMetadataNotStatus)
{
  auto e = engine(); e.common_status(status(1000000), S);
  CommonInfoPacket p; p.count = 3; p.time_us = 1000000; p.temperature.fill(2500);
      e.common_info(p, S);
  EXPECT_TRUE(e.esc(2, S).temperature_valid);
  p.temperature[2] = 3000; e.common_info(p, 2 * S);
  EXPECT_TRUE(e.esc(2, 2 * S).rpm_valid); EXPECT_FALSE(e.esc(2, 2 * S).temperature_valid);
  p.time_us = 2000000; e.common_info(p, 2 * S);
  EXPECT_TRUE(e.esc(2, 2 * S).temperature_valid); EXPECT_FLOAT_EQ(e.esc(2, 2 * S).temperature, 30);
}
TEST(Config, GeometryChangesRetainEscReportsButRequireFreshWheelSamples)
{
  auto old = engine("esc_status"); ASSERT_TRUE(old.common_status(status(1000000), S));
  auto c = config("esc_status"); c.geometry.left_radius_m = c.geometry.right_radius_m = 0.2;
  ObservationEngine updated(c); updated.retain_esc_observations(old, S + 1);
  EXPECT_EQ(updated.esc(0, S + 1).rpm, 60); EXPECT_TRUE(updated.esc(0, S + 1).valid);
  EXPECT_FALSE(updated.common_status(status(1000000), S + 2));
  auto out = updated.common_status(status(2000000), 2 * S); ASSERT_TRUE(out);
  EXPECT_NEAR(out->linear_x_mps, 0.4 * M_PI, 1e-12);
}
TEST(CommonEsc, ConfiguredOrientationNeverChangesRawSignedRpm)
{
  auto c = config("esc_status");
      c.left_esc_rpm_to_wheel_ratio = c.right_esc_rpm_to_wheel_ratio = -0.5;
  ObservationEngine e(c); e.connection(true);
      auto out = e.common_status(status(1000000, -120, -120), S);
  ASSERT_TRUE(out); EXPECT_GT(out->linear_x_mps, 0); EXPECT_EQ(e.esc(0, S).rpm, -120);
  EXPECT_TRUE(e.esc(0, S).rpm_direction_valid);
  c.left_esc_rpm_to_wheel_ratio = 0;
      EXPECT_THROW((void)ObservationEngine(c), std::invalid_argument);
}
}  // namespace

TEST(Distance, EqualReceiptTimeStillUsesAdvancingMeasurementTime)
{
  auto e = engine("wheel_distance");
  EXPECT_FALSE(e.wheel_distance(distance(1000000, 0, 0), S));
  auto out = e.wheel_distance(distance(2000000, 1, 1), S);
  ASSERT_TRUE(out);
  EXPECT_DOUBLE_EQ(out->linear_x_mps, 1);
  EXPECT_EQ(out->sample_stamp_ns, 2 * S);
  EXPECT_EQ(out->receipt_stamp_ns, S);
}

TEST(CommonEsc, MeasurementMonotonicityAllowsEqualReceiptTime)
{
  auto e = engine("esc_status");
  ASSERT_TRUE(e.common_status(status(1000000), S));
  auto out = e.common_status(status(2000000), S);
  ASSERT_TRUE(out);
  EXPECT_EQ(out->sample_stamp_ns, 2 * S);
  EXPECT_EQ(out->receipt_stamp_ns, S);
  EXPECT_FALSE(e.common_status(status(2000000), S + 1));
}

TEST(SourceEpochs, StatusResetPreservesActiveDistanceAndLegacy)
{
  auto e = engine();
  e.legacy_esc(legacy(10, 10), S);
  e.legacy_esc(legacy(11, 11), S + 1);
  ASSERT_TRUE(e.legacy_rpm(60, 60, S + 2));
  ASSERT_TRUE(e.common_status(status(2000000), 2 * S));
  EXPECT_FALSE(e.wheel_distance(distance(2000000, 10, 10), 2 * S + 1));
  EXPECT_FALSE(e.common_status(status(100), 2 * S + 2));
  EXPECT_EQ(e.active_source(), WheelSource::WheelDistance);
  EXPECT_TRUE(e.esc(0, 2 * S + 2).valid);
  auto out = e.wheel_distance(distance(3000000, 11, 11), 3 * S);
  ASSERT_TRUE(out); EXPECT_DOUBLE_EQ(out->linear_x_mps, 1);
  e.poll(6 * S + 1);
  e.legacy_esc(legacy(12, 12), 6 * S + 2);
  ASSERT_TRUE(e.legacy_rpm(60, 60, 6 * S + 3));
  EXPECT_EQ(e.active_source(), WheelSource::ArduPilotLegacy);
}
TEST(SourceEpochs, InfoResetPreservesDistanceStatusAndUnrelatedMetadata)
{
  auto e = engine();
  ASSERT_TRUE(e.common_status(status(2000000), S));
  CommonInfoPacket p; p.count = 8; p.time_us = 2000000; p.temperature.fill(2500);
  e.common_info(p, S + 1);
  auto other = p; other.index = 4; e.common_info(other, S + 2);
  EXPECT_FALSE(e.wheel_distance(distance(2000000, 10, 10), S + 3));
  p.time_us = 100; e.common_info(p, S + 4);
  EXPECT_EQ(e.active_source(), WheelSource::WheelDistance);
  EXPECT_TRUE(e.esc(0, S + 4).valid);
  EXPECT_FALSE(e.esc(0, S + 4).temperature_valid);
  EXPECT_TRUE(e.esc(4, S + 4).temperature_valid);
  ASSERT_TRUE(e.wheel_distance(distance(3000000, 11, 11), 2 * S));
}
TEST(SourceEpochs, DistanceResetPreservesCommonObservationsAndMotion)
{
  auto e = engine("esc_status");
  ASSERT_TRUE(e.common_status(status(1000000), S));
  CommonInfoPacket p; p.count = 3; p.time_us = 1000000; p.temperature.fill(2500);
  e.common_info(p, S + 1);
  EXPECT_FALSE(e.wheel_distance(distance(2000000, 10, 10), S + 2));
  EXPECT_FALSE(e.wheel_distance(distance(100, 0, 0), S + 3));
  EXPECT_EQ(e.active_source(), WheelSource::EscStatus);
  EXPECT_TRUE(e.esc(0, S + 3).valid); EXPECT_TRUE(e.esc(0, S + 3).temperature_valid);
  EXPECT_FALSE(e.common_status(status(1000000), S + 4));
  ASSERT_TRUE(e.common_status(status(2000000), 2 * S));
}
TEST(SourceEpochs, DisconnectAndRosClockRollbackResetEverySource)
{
  auto e = engine();
  e.legacy_esc(legacy(1, 1), S);
  e.common_status(status(1000000), S + 1);
  e.wheel_distance(distance(1000000, 10, 10), S + 2);
  e.connection(false);
  EXPECT_EQ(e.active_source(), WheelSource::None);
  EXPECT_FALSE(e.touched(0)); EXPECT_FALSE(e.esc(0, S + 3).valid);
  e.connection(true);
  EXPECT_FALSE(e.wheel_distance(distance(2000000, 0, 0), 2 * S));
  EXPECT_FALSE(e.common_status(status(2000000), 2 * S + 1));
  e.poll(S);
  EXPECT_EQ(e.active_source(), WheelSource::None);
  EXPECT_FALSE(e.touched(0)); EXPECT_FALSE(e.esc(0, S).valid);
  EXPECT_FALSE(e.wheel_distance(distance(3000000, 999, 999), S + 1));
}
TEST(CommonPair, SeparateGroupsUseDedicatedSkewNotFreshness)
{
  auto c = config("esc_status"); c.geometry.left_esc_slot = 4; c.geometry.right_esc_slot = 8;
  ObservationEngine e(c); e.connection(true);
  EXPECT_FALSE(e.common_status(status(1000000, 60, 0, 4), S));
  EXPECT_TRUE(e.common_status(status(1100000, 60, 0, 8), S + 1));
  EXPECT_FALSE(e.common_status(status(2000000, 60, 0, 4), 2 * S));
  EXPECT_FALSE(e.common_status(status(2400000, 60, 0, 8), 2 * S + 1));
  EXPECT_EQ(e.active_source(), WheelSource::None);
  EXPECT_TRUE(e.common_status(status(2200000, 60, 0, 4), 2 * S + 2));
}
TEST(CommonPair, TransitionDoesNotReuseCachedWheelWithNewWheel)
{
  auto c = config(); c.geometry.left_esc_slot = 4; c.geometry.right_esc_slot = 8;
  ObservationEngine e(c); e.connection(true);
  e.common_status(status(1000000, 60, 0, 4), S);
  ASSERT_TRUE(e.common_status(status(1000000, 60, 0, 8), S + 1));
  EXPECT_FALSE(e.wheel_distance(distance(1000000, 0, 0), S + 2));
  EXPECT_FALSE(e.common_status(status(4000000, 60, 0, 4), 4 * S));
  EXPECT_FALSE(e.common_status(status(4000000, 60, 0, 8), 4 * S + 1));
  e.poll(4 * S + 10); // WD expired; cached COMMON cannot become a new motion sample.
  EXPECT_EQ(e.active_source(), WheelSource::EscStatus);
  EXPECT_FALSE(e.common_status(status(4100000, 60, 0, 4), 4 * S + 11));
  EXPECT_TRUE(e.common_status(status(4100000, 60, 0, 8), 4 * S + 12));
}
TEST(CommonPair, InfoIsNotSubjectToWheelPairSkew)
{
  auto e = engine(); e.common_status(status(2000000), 2 * S);
  CommonInfoPacket p; p.time_us = 1000000; p.count = 3; p.temperature.fill(2500);
  e.common_info(p, 2 * S + 1);
  EXPECT_TRUE(e.esc(0, 2 * S + 1).temperature_valid);
}
TEST(CommonPair, RejectsInvalidSkewConfigurations)
{
  for (double value : {0.0, -1.0, 3.1, std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::quiet_NaN()})
  {
    auto c = config(); c.common_pair_max_skew_s = value;
    EXPECT_THROW((void)ObservationEngine(c), std::invalid_argument);
  }
  auto c = config(); c.common_pair_max_skew_s = 3.0;
  EXPECT_NO_THROW((void)ObservationEngine(c));
}
TEST(CommonOwner, DifferentComponentsCannotMixStatusOrInfo)
{
  auto c = config("esc_status"); c.geometry.left_esc_slot = 0; c.geometry.right_esc_slot = 4;
  ObservationEngine e(c); e.connection(true);
  auto a = status(1000000, 60, 0, 0); a.component_id = 10;
  auto b = status(1000000, -60, 0, 4); b.component_id = 20;
  EXPECT_FALSE(e.common_status(a, S)); EXPECT_FALSE(e.common_status(b, S + 1));
  EXPECT_FALSE(e.touched(4)); EXPECT_EQ(e.esc(0, S + 1).rpm, 60);
  b.index = 0; e.common_status(b, S + 2);
  EXPECT_EQ(e.esc(0, S + 2).rpm, 60);
  CommonInfoPacket info; info.time_us = 1000000; info.count = 8;
  info.temperature.fill(2500); info.component_id = 20;
  e.common_info(info, S + 3);
  EXPECT_FALSE(e.esc(0, S + 3).temperature_valid);
  info.component_id = 10; e.common_info(info, S + 4);
  EXPECT_TRUE(e.esc(0, S + 4).temperature_valid);
}
TEST(CommonOwner, TakeoverAfterExpiryClearsOldStatusAndMetadataOnly)
{
  auto e = engine();
  auto a = status(1000000); a.component_id = 10; e.common_status(a, S);
  CommonInfoPacket p; p.time_us = 1000000; p.count = 3;
  p.component_id = 10; p.temperature.fill(2500); e.common_info(p, S + 1);
  e.wheel_distance(distance(4000000, 10, 10), 4 * S);
  auto b = status(4000000, -60, -60); b.component_id = 20;
  EXPECT_FALSE(e.common_status(b, 4 * S + 2));
  EXPECT_EQ(e.active_source(), WheelSource::WheelDistance);
  EXPECT_EQ(e.esc(0, 4 * S + 2).rpm, -60);
  EXPECT_FALSE(e.esc(0, 4 * S + 2).temperature_valid);
  p.time_us = 4000000; e.common_info(p, 4 * S + 3);
  EXPECT_FALSE(e.esc(0, 4 * S + 3).temperature_valid);
  p.component_id = 20; e.common_info(p, 4 * S + 4);
  EXPECT_TRUE(e.esc(0, 4 * S + 4).temperature_valid);
  ASSERT_TRUE(e.wheel_distance(distance(5000000, 11, 11), 5 * S));
}
TEST(CommonOwner, SameComponentInfoCandidateEnrichesStatusAndRestrictionIsEnforced)
{
  auto c = config("esc_status"); c.esc_component_id = 20;
  ObservationEngine e(c); e.connection(true);
  CommonInfoPacket p; p.time_us = 1000000; p.count = 3;
  p.component_id = 10; p.temperature.fill(2500);
  e.common_info(p, S); EXPECT_FALSE(e.touched(0));
  p.component_id = 20; e.common_info(p, S + 1);
  auto a = status(1000000); a.component_id = 10;
  EXPECT_FALSE(e.common_status(a, S + 2)); EXPECT_FALSE(e.esc(0, S + 2).valid);
  a.component_id = 20; ASSERT_TRUE(e.common_status(a, S + 3));
  EXPECT_TRUE(e.esc(0, S + 3).temperature_valid);
  c.esc_component_id = 256; EXPECT_THROW((void)ObservationEngine(c), std::invalid_argument);
}

TEST(CommonPair, SkewBoundaryIsInclusiveAndIndependentFromReceiptSkew)
{
  auto c = config("esc_status"); c.geometry.left_esc_slot = 0; c.geometry.right_esc_slot = 4;
  ObservationEngine e(c); e.connection(true);
  e.common_status(status(1000000, 60, 0, 0), S);
  ASSERT_TRUE(e.common_status(status(1250000, 60, 0, 4), 2 * S));
  EXPECT_FALSE(e.common_status(status(2000000, 60, 0, 0), 2 * S + 1));
  EXPECT_FALSE(e.common_status(status(2250001, 60, 0, 4), 2 * S + 2));
}
TEST(CommonOwner, RepeatedFramesDoNotKeepAnExpiredOwnerAlive)
{
  auto e = engine();
  auto a = status(1000000); a.component_id = 10;
  e.common_status(a, S); e.common_status(a, 3 * S);
  auto b = status(1000000, -60, -60); b.component_id = 20;
  e.common_status(b, 4 * S + 1);
  EXPECT_EQ(e.esc(0, 4 * S + 1).rpm, -60);
}
TEST(SourceEpochs, InfoResetDuplicateCannotRestoreMetadataFreshness)
{
  auto e = engine(); e.common_status(status(2000000), S);
  CommonInfoPacket p; p.time_us = 2000000; p.count = 3; p.temperature.fill(2500);
  e.common_info(p, S + 1);
  p.time_us = 100; e.common_info(p, S + 2); e.common_info(p, S + 3);
  EXPECT_FALSE(e.esc(0, S + 3).temperature_valid);
  p.time_us = 1000100; e.common_info(p, S + 4);
  EXPECT_TRUE(e.esc(0, S + 4).temperature_valid);
  EXPECT_TRUE(e.esc(0, S + 4).rpm_valid);
}

TEST(CommonPair, TransitionThroughNoneStillRequiresBothNewWheels)
{
  auto c = config(); c.geometry.left_esc_slot = 0; c.geometry.right_esc_slot = 4;
  ObservationEngine e(c); e.connection(true);
  e.wheel_distance(distance(1000000, 0, 0), S);
  EXPECT_FALSE(e.common_status(status(4000000, 60, 0, 0), 4 * S));
  EXPECT_FALSE(e.common_status(status(4400000, 60, 0, 4), 4 * S + 1));
  e.poll(4 * S + 2);
  EXPECT_EQ(e.active_source(), WheelSource::None);
  EXPECT_FALSE(e.common_status(status(4400000, 60, 0, 0), 4 * S + 3));
  // Both wheels have now advanced past the WD exit; one valid pair may publish.
  EXPECT_TRUE(e.common_status(status(4500000, 60, 0, 4), 4 * S + 4));
  // Updating only the other wheel cannot publish that same right sample twice.
  EXPECT_FALSE(e.common_status(status(4500000, 60, 0, 0), 4 * S + 5));
}

TEST(CommonOwner, InfoCandidateCannotBlockFirstStatusFromAnotherComponent)
{
  auto e = engine("esc_status");
  CommonInfoPacket info; info.time_us = 1000000; info.count = 3;
  info.component_id = 10; info.temperature.fill(2500);
  e.common_info(info, S);
  EXPECT_TRUE(e.esc(0, S).temperature_valid);
  auto b = status(1000000, -60, -60); b.component_id = 20;
  auto out = e.common_status(b, S + 1);
  ASSERT_TRUE(out);
  EXPECT_EQ(e.active_source(), WheelSource::EscStatus);
  EXPECT_LT(out->linear_x_mps, 0);
  EXPECT_TRUE(e.esc(0, S + 1).rpm_valid);
  EXPECT_EQ(e.esc(0, S + 1).rpm, -60);
  EXPECT_FALSE(e.esc(0, S + 1).temperature_valid);
  EXPECT_FALSE(e.esc(0, S + 1).failure_flags_valid);
  e.common_info(info, S + 2);
  EXPECT_FALSE(e.esc(0, S + 2).temperature_valid);
  auto a = status(2000000, 120, 120); a.component_id = 10;
  EXPECT_FALSE(e.common_status(a, S + 3));
  EXPECT_EQ(e.esc(0, S + 3).rpm, -60);
}
TEST(CommonOwner, InfoCannotRenewStatusOwnershipOrTakeItOver)
{
  auto e = engine("esc_status");
  auto a = status(1000000); a.component_id = 10;
  ASSERT_TRUE(e.common_status(a, S));
  CommonInfoPacket info; info.time_us = 3000000; info.count = 3;
  info.component_id = 10; info.temperature.fill(2500);
  e.common_info(info, 3 * S);
  info.component_id = 30; info.time_us = 4000000;
  e.common_info(info, 4 * S + 1);
  // Only STATUS may take over an expired STATUS owner; foreign INFO is ignored.
  EXPECT_FALSE(e.esc(0, 4 * S + 1).valid);
  auto b = status(4000000, -60, -60); b.component_id = 20;
  auto out = e.common_status(b, 4 * S + 2);
  ASSERT_TRUE(out);
  EXPECT_EQ(e.esc(0, 4 * S + 2).rpm, -60);
  EXPECT_FALSE(e.esc(0, 4 * S + 2).temperature_valid);
}
