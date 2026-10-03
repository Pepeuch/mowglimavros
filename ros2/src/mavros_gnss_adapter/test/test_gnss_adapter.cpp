#include <gtest/gtest.h>
#include "mavros_gnss_adapter/adapter_state.hpp"
#include "mavros_gnss_adapter/ellipsoid_altitude.hpp"

using namespace mavros_gnss_adapter;
constexpr int64_t second = 1000000000LL;

UniversalStatus observation(uint64_t sequence = 7, int64_t stamp = second)
{
  UniversalStatus input;
  input.stamp.sec = stamp / second;
  input.stamp.nanosec = stamp % second;
  input.source_id = "mavlink:fcu:gps1";
  input.source_incarnation = "session1";
  input.position_observation_sequence = sequence;
  input.fix_valid = true;
  input.fix_type = UniversalStatus::FIX_TYPE_FIX;
  return input;
}

sensor_msgs::msg::NavSatFix fix_at(int64_t stamp = second)
{
  sensor_msgs::msg::NavSatFix fix;
  fix.header.stamp = observation(7, stamp).stamp;
  fix.status.status = sensor_msgs::msg::NavSatStatus::STATUS_FIX;
  fix.latitude = 43.9542;
  fix.longitude = 2.2022;
  fix.altitude = 180.0;
  fix.position_covariance[8] = 4.0;
  return fix;
}

AltitudeSample altitude_at(int64_t stamp = second)
{
  return {stamp, 439542000, 22022000, 180000, 230.0};
}

TEST(Selection, BothReceiversAndDirectMode)
{
  for (const auto source : {"gps1", "gps2"}) {
    EXPECT_TRUE(canonical_enabled("mavros", source));
    EXPECT_FALSE(canonical_enabled("direct", source));
    EXPECT_EQ(map_status(observation(), source).backend, std::string("mavros_") + source);
  }
  EXPECT_THROW(canonical_enabled("bad", "gps1"), std::invalid_argument);
  EXPECT_THROW(canonical_enabled("mavros", "gps3"), std::invalid_argument);
}

TEST(Mapping, FixRtkAndBaselineEnums)
{
  const std::pair<uint8_t, uint8_t> fixes[] = {
    {UniversalStatus::FIX_TYPE_UNKNOWN, Status::FIX_TYPE_NO_FIX},
    {UniversalStatus::FIX_TYPE_NO_FIX, Status::FIX_TYPE_NO_FIX},
    {UniversalStatus::FIX_TYPE_FIX, Status::FIX_TYPE_GPS_FIX},
    {UniversalStatus::FIX_TYPE_RTK_FLOAT, Status::FIX_TYPE_RTK_FLOAT},
    {UniversalStatus::FIX_TYPE_RTK_FIXED, Status::FIX_TYPE_RTK_FIXED},
    {UniversalStatus::FIX_TYPE_DEAD_RECKONING, Status::FIX_TYPE_DEAD_RECKONING}};
  for (const auto & pair : fixes) {EXPECT_EQ(map_fix(pair.first), pair.second);}
  EXPECT_EQ(map_fix(255), Status::FIX_TYPE_NO_FIX);
  EXPECT_EQ(map_rtk(UniversalStatus::RTK_MODE_UNKNOWN), Status::RTK_MODE_UNKNOWN);
  EXPECT_EQ(map_rtk(UniversalStatus::RTK_MODE_NONE), Status::RTK_MODE_NONE);
  EXPECT_EQ(map_rtk(UniversalStatus::RTK_MODE_FLOAT), Status::RTK_MODE_FLOAT);
  EXPECT_EQ(map_rtk(UniversalStatus::RTK_MODE_FIXED), Status::RTK_MODE_FIXED);
  EXPECT_EQ(map_rtk(255), Status::RTK_MODE_UNKNOWN);
  EXPECT_EQ(map_baseline(UniversalStatus::BASELINE_STATUS_COMPUTED), Status::BASELINE_STATUS_COMPUTED);
  EXPECT_EQ(map_baseline(255), Status::BASELINE_STATUS_UNKNOWN);
}

TEST(Mapping, IndependentCapabilityBitsAndValues)
{
  // Deliberately non-identical bit layouts; a numeric cast cannot pass these.
  const std::pair<uint32_t, uint32_t> bits[] = {
    {UniversalStatus::CAP_RTK_MODE, Status::CAP_RTK_MODE},
    {UniversalStatus::CAP_HORIZONTAL_ACCURACY, Status::CAP_HORIZONTAL_ACCURACY},
    {UniversalStatus::CAP_VERTICAL_ACCURACY, Status::CAP_VERTICAL_ACCURACY},
    {UniversalStatus::CAP_HDOP, Status::CAP_HDOP},
    {UniversalStatus::CAP_VDOP, Status::CAP_VDOP},
    {UniversalStatus::CAP_SATELLITES_USED, Status::CAP_SATELLITES_USED},
    {UniversalStatus::CAP_SATELLITES_VISIBLE, Status::CAP_SATELLITES_VISIBLE},
    {UniversalStatus::CAP_SATELLITES_TRACKED, Status::CAP_SATELLITES_TRACKED},
    {UniversalStatus::CAP_MEAN_CN0, Status::CAP_MEAN_CN0},
    {UniversalStatus::CAP_MAX_CN0, Status::CAP_MAX_CN0},
    {UniversalStatus::CAP_CORRECTION_AGE, Status::CAP_CORRECTION_AGE},
    {UniversalStatus::CAP_HEADING, Status::CAP_HEADING},
    {UniversalStatus::CAP_HEADING_ACCURACY, Status::CAP_HEADING_ACCURACY},
    {UniversalStatus::CAP_DUAL_ANTENNA_HEADING, Status::CAP_DUAL_ANTENNA_STATUS},
    {UniversalStatus::CAP_INTERFERENCE_STATE, Status::CAP_INTERFERENCE_STATUS},
    {UniversalStatus::CAP_JAMMING_STATE, Status::CAP_JAMMING_STATUS},
    {UniversalStatus::CAP_DIFFERENTIAL_CORRECTIONS, Status::CAP_DIFFERENTIAL_CORRECTIONS},
    {UniversalStatus::CAP_CORRECTIONS_ACTIVE, Status::CAP_CORRECTIONS_ACTIVE},
    {UniversalStatus::CAP_DUAL_ANTENNA_BASELINE, Status::CAP_DUAL_ANTENNA_BASELINE},
    {UniversalStatus::CAP_BASELINE_AZIMUTH, Status::CAP_BASELINE_AZIMUTH},
    {UniversalStatus::CAP_BASELINE_PITCH, Status::CAP_BASELINE_PITCH},
    {UniversalStatus::CAP_BASELINE_LENGTH, Status::CAP_BASELINE_LENGTH},
    {UniversalStatus::CAP_BASELINE_SOLUTION_STATUS, Status::CAP_BASELINE_SOLUTION_STATUS}};
  for (const auto & pair : bits) {
    EXPECT_EQ(map_flags(pair.first), pair.second);
    auto input = observation();
    input.capability_flags = pair.first;
    input.value_flags = pair.first;
    EXPECT_EQ(map_status(input, "gps1").value_flags, pair.second);
    input.value_flags = 0;
    EXPECT_EQ(map_status(input, "gps1").capability_flags, pair.second);
    EXPECT_EQ(map_status(input, "gps1").value_flags, 0u);
  }
  EXPECT_EQ(map_flags(1u << 31), 0u);
  auto input = observation();
  input.value_flags = UniversalStatus::CAP_HDOP;
  EXPECT_EQ(map_status(input, "gps1").value_flags, 0u);
}

TEST(Mapping, RichFieldsAndUnknownCorrectionDiagnostics)
{
  auto input = observation(UINT64_MAX - 1);
  input.fix_type = UniversalStatus::FIX_TYPE_RTK_FIXED;
  input.rtk_mode = UniversalStatus::RTK_MODE_FIXED;
  input.hdop = 0.54F;
  input.vdop = 0.8F;
  input.horizontal_accuracy_m = 0.01F;
  input.vertical_accuracy_m = 0.02F;
  input.satellites_used = 26;
  input.satellites_visible = 30;
  input.satellites_tracked = 28;
  input.differential_corrections = true;
  input.corrections_active = true;
  input.correction_age_s = 1.2F;
  input.mean_cn0_db_hz = 40;
  input.max_cn0_db_hz = 48;
  input.baseline_length_m = 1.5F;
  input.dual_antenna_baseline = true;
  const auto output = map_status(input, "gps2");
  EXPECT_EQ(output.position_observation_sequence, input.position_observation_sequence);
  EXPECT_EQ(output.hdop, input.hdop);
  EXPECT_EQ(output.vdop, input.vdop);
  EXPECT_EQ(output.horizontal_accuracy_m, input.horizontal_accuracy_m);
  EXPECT_EQ(output.vertical_accuracy_m, input.vertical_accuracy_m);
  EXPECT_EQ(output.satellites_used, 26);
  EXPECT_EQ(output.satellites_visible, 30);
  EXPECT_EQ(output.satellites_tracked, 28);
  EXPECT_TRUE(output.differential_corrections);
  EXPECT_TRUE(output.corrections_active);
  EXPECT_EQ(output.correction_age_s, input.correction_age_s);
  EXPECT_EQ(output.mean_cn0_db_hz, 40);
  EXPECT_EQ(output.max_cn0_db_hz, 48);
  EXPECT_TRUE(output.dual_antenna_baseline);
  EXPECT_EQ(output.baseline_length_m, 1.5F);
  EXPECT_TRUE(std::isnan(output.quality_percent));
  EXPECT_EQ(output.capability_flags & (Status::CAP_CORRECTION_STREAM | Status::CAP_MSM_SUMMARY |
    Status::CAP_CORRECTION_TRANSPORT | Status::CAP_CORRECTION_FLOW | Status::CAP_CORRECTION_SEMANTIC), 0u);
  EXPECT_EQ(output.correction_transport_status, Status::CORRECTION_TRANSPORT_STATUS_UNKNOWN);
  EXPECT_TRUE(output.correction_source.empty());
}

TEST(Freshness, CachedRtkDoesNotRenewPositionAndStaleDoesNotAdvanceSequence)
{
  AdapterState state("gps1");
  auto input = observation();
  auto output = state.observe_status(input, second);
  EXPECT_TRUE(output.fix_valid);
  input.stamp.sec = 3;
  input.satellites_used = 27;
  output = state.observe_status(input, 3 * second);
  EXPECT_EQ(output.header.stamp.sec, 1);
  EXPECT_EQ(output.satellites_used, 27);
  EXPECT_FALSE(state.expire(4 * second));
  const auto stale = state.expire(4 * second + 1);
  ASSERT_TRUE(stale);
  EXPECT_FALSE(stale->fix_valid);
  EXPECT_EQ(stale->fix_type, Status::FIX_TYPE_NO_FIX);
  EXPECT_EQ(stale->value_flags, 0u);
  EXPECT_EQ(stale->position_observation_sequence, 7u);
  EXPECT_EQ(stale->header.stamp.sec, 1);
  EXPECT_FALSE(state.observe_status(input, 5 * second).fix_valid);
  EXPECT_TRUE(state.observe_status(observation(8, 6 * second), 6 * second).fix_valid);
}

TEST(Freshness, NoFixZeroSequenceRestartAndRegression)
{
  AdapterState state("gps2");
  auto input = observation(0);
  EXPECT_FALSE(state.observe_status(input, second).fix_valid);
  EXPECT_TRUE(state.observe_status(observation(), second).fix_valid);
  input = observation(8);
  input.fix_type = UniversalStatus::FIX_TYPE_NO_FIX;
  input.fix_valid = false;
  EXPECT_FALSE(state.observe_status(input, 2 * second).fix_valid);
  input = observation(1, 3 * second);
  EXPECT_FALSE(state.observe_status(input, 3 * second).fix_valid);
  input.source_incarnation = "session2";
  EXPECT_TRUE(state.observe_status(input, 3 * second).fix_valid);
}

TEST(Altitude, EllipsoidPreservedForBothSourcesAndBothCallbackOrders)
{
  for (const auto source : {"gps1", "gps2"}) {
    for (const bool raw_first : {false, true}) {
      AdapterState state(source);
      if (raw_first) {state.observe_altitude(altitude_at());}
      state.observe_fix(fix_at(), second);
      state.observe_status(observation(), second);
      if (!raw_first) {state.observe_altitude(altitude_at());}
      EXPECT_TRUE(state.take_fixes(second).empty());
      const auto fixes = state.take_fixes(second + second / 5);
      ASSERT_EQ(fixes.size(), 1u);
      EXPECT_DOUBLE_EQ(fixes[0].altitude, 230.0);
      EXPECT_DOUBLE_EQ(fixes[0].latitude, fix_at().latitude);
      EXPECT_EQ(fixes[0].position_covariance, fix_at().position_covariance);
      state.observe_fix(fix_at(), second + second / 4);
      EXPECT_TRUE(state.take_fixes(2 * second).empty());
    }
  }
}

TEST(Altitude, MissingAmbiguousOldOrWrongPositionNeverFallsBackToMsl)
{
  for (int scenario = 0; scenario < 6; ++scenario) {
    AdapterState state("gps1");
    state.observe_status(observation(), second);
    auto altitude = altitude_at();
    if (scenario == 1) {altitude.ellipsoid_m.reset();}
    if (scenario == 2) {altitude.receipt_ros_ns -= second;}
    if (scenario == 3) {altitude.latitude_e7 += 100;}
    if (scenario == 4) {altitude.altitude_msl_mm += 1000;}
    if (scenario != 0) {state.observe_altitude(altitude);}
    if (scenario == 5) {state.observe_altitude(altitude);}
    state.observe_fix(fix_at(), second);
    const auto fixes = state.take_fixes(second + second / 5);
    ASSERT_EQ(fixes.size(), 1u);
    EXPECT_TRUE(std::isnan(fixes[0].altitude)) << scenario;
  }
}

TEST(Altitude, NoFixStaleAndIncarnationChangeDiscardPendingFix)
{
  for (int scenario = 0; scenario < 3; ++scenario) {
    AdapterState state("gps1");
    state.observe_status(observation(), second);
    state.observe_altitude(altitude_at());
    state.observe_fix(fix_at(), second);
    if (scenario == 0) {
      auto input = observation(8);
      input.fix_valid = false;
      state.observe_status(input, second);
    } else if (scenario == 1) {
      state.expire(5 * second);
    } else {
      auto input = observation(1);
      input.source_incarnation = "new";
      state.observe_status(input, second);
    }
    EXPECT_TRUE(state.take_fixes(second + second / 5).empty());
  }
}

TEST(Altitude, SeveralPositionsWithinDeliveryWindowAreNotDropped)
{
  AdapterState state("gps1");
  for (int i = 0; i < 3; ++i) {
    const auto stamp = second + i * second / 20;
    state.observe_status(observation(7 + i, stamp), stamp);
    state.observe_altitude(altitude_at(stamp));
    state.observe_fix(fix_at(stamp), stamp);
  }
  const auto fixes = state.take_fixes(second + second / 2);
  ASSERT_EQ(fixes.size(), 3u);
  for (const auto & fix : fixes) {EXPECT_DOUBLE_EQ(fix.altitude, 230.0);}
}

TEST(Altitude, ZeroAndNegativeEllipsoidAreValidWhenAvailable)
{
  for (double height : {0.0, -25.4}) {
    AdapterState state("gps2");
    state.observe_status(observation(), second);
    auto sample = altitude_at();
    sample.ellipsoid_m = height;
    state.observe_altitude(sample);
    state.observe_fix(fix_at(), second);
    const auto fixes = state.take_fixes(second + second / 5);
    ASSERT_EQ(fixes.size(), 1u);
    EXPECT_DOUBLE_EQ(fixes[0].altitude, height);
  }
}

TEST(AltitudeWire, RequiresAllFourFieldBytesForBothReceivers)
{
  struct Raw {int32_t alt_ellipsoid;};
  const Raw raw{230000};
  ASSERT_EQ(sizeof(raw.alt_ellipsoid), 4u);
  for (const size_t offset : {30u, 37u}) {
    for (size_t bytes = 0; bytes <= 4; ++bytes) {
      SCOPED_TRACE(::testing::Message() << "offset=" << offset << " bytes=" << bytes);
      const auto height = ellipsoid_altitude(offset + bytes, offset, raw);
      EXPECT_EQ(height.has_value(), bytes == 4);
      AdapterState state(offset == 30 ? "gps1" : "gps2");
      state.observe_status(observation(), second);
      auto sample = altitude_at();
      sample.ellipsoid_m = height;
      state.observe_altitude(sample);
      state.observe_fix(fix_at(), second);
      const auto fixes = state.take_fixes(second + second / 5);
      ASSERT_EQ(fixes.size(), 1u);
      if (bytes == 4) {
        ASSERT_TRUE(height);
        EXPECT_DOUBLE_EQ(*height, 230.0);
        EXPECT_DOUBLE_EQ(fixes[0].altitude, 230.0);
      } else {
        EXPECT_TRUE(std::isnan(fixes[0].altitude));
      }
    }
    EXPECT_FALSE(ellipsoid_altitude(offset - 1, offset, raw));
    EXPECT_DOUBLE_EQ(*ellipsoid_altitude(offset + 4, offset, Raw{-25400}), -25.4);
  }
}

TEST(AltitudeWire, ZeroFieldRemainsUnknownEvenWithCompleteOrLongerPayload)
{
  struct Raw {int32_t alt_ellipsoid;};
  for (const size_t offset : {30u, 37u}) {
    for (const size_t length : {offset, offset + 1, offset + 2, offset + 3, offset + 4, offset + 20}) {
      SCOPED_TRACE(::testing::Message() << "offset=" << offset << " length=" << length);
      const auto height = ellipsoid_altitude(length, offset, Raw{0});
      EXPECT_FALSE(height);
      AdapterState state(offset == 30 ? "gps1" : "gps2");
      state.observe_status(observation(), second);
      auto sample = altitude_at();
      sample.ellipsoid_m = height;
      state.observe_altitude(sample);
      state.observe_fix(fix_at(), second);
      const auto fixes = state.take_fixes(second + second / 5);
      ASSERT_EQ(fixes.size(), 1u);
      EXPECT_TRUE(std::isnan(fixes[0].altitude));
    }
  }
}
