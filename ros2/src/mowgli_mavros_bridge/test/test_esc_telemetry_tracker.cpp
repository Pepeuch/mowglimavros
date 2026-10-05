#include <gtest/gtest.h>
#include <limits>
#include "mavros_esc_wheel_odometry/observation_engine.hpp"
#include "mowgli_mavros_bridge/esc_telemetry_tracker.hpp"
#include "mowgli_mavros_bridge/vesc_telemetry_projection.hpp"

namespace
{
struct EscSample
{
  int32_t rpm; float voltage, current, totalcurrent, temperature; uint16_t count;
};
using Message = mavros_esc_wheel_odometry::msg::EscObservation;
Message canonical(const mavros_esc_wheel_odometry::EscObservationData & d)
{
  Message m;
  m.header.stamp.sec = d.stamp_ns / 1000000000LL;
  m.header.stamp.nanosec = d.stamp_ns % 1000000000LL;
  m.metadata_stamp.sec = d.metadata_stamp_ns / 1000000000LL;
  m.metadata_stamp.nanosec = d.metadata_stamp_ns % 1000000000LL;
  m.esc_index = d.index; m.source = static_cast<uint8_t>(d.source); m.valid = d.valid;
  m.rpm = d.rpm; m.rpm_valid = d.rpm_valid; m.rpm_direction_valid = d.rpm_direction_valid;
  m.voltage = d.voltage; m.voltage_valid = d.voltage_valid; m.current = d.current;
  m.current_valid = d.current_valid;
  m.temperature = d.temperature; m.temperature_valid = d.temperature_valid;
  m.totalcurrent = d.totalcurrent; m.totalcurrent_valid = d.totalcurrent_valid;
  m.count = d.count; m.count_valid = d.count_valid;
  m.failure_flags = d.failure_flags; m.failure_flags_valid = d.failure_flags_valid;
  m.error_count = d.error_count; m.error_count_valid = d.error_count_valid;
  return m;
}
// Keep the original legacy assertions on the real normalizer + canonical tracker.
class EscTelemetryTracker : public mowgli_mavros_bridge::EscTelemetryTracker
{
public:
  explicit EscTelemetryTracker(double timeout = 3.0)
  : mowgli_mavros_bridge::EscTelemetryTracker(timeout),
    engine_(mavros_esc_wheel_odometry::ObservationConfig{})
  {engine_.connection(true);}
  void observe(unsigned index, EscSample sample, int64_t now)
  {
    if (index >= 12) {return;}
    samples_[index] = sample;
    mavros_esc_wheel_odometry::LegacyEscPacket packet; packet.index = index / 4 * 4;
    for (unsigned i = 0; i < 4; ++i) {
      const auto & s = samples_[packet.index + i];
      packet.counts[i] = s.count; packet.rpm[i] = s.rpm; packet.voltage[i] = s.voltage;
      packet.current[i] = s.current; packet.temperature[i] = s.temperature;
      packet.totalcurrent[i] = s.totalcurrent;
    }
    engine_.legacy_esc(packet, now);
    mowgli_mavros_bridge::EscTelemetryTracker::observe(canonical(engine_.esc(index, now)), now);
  }
  void reset() {engine_.connection(true); mowgli_mavros_bridge::EscTelemetryTracker::reset();}

private:
  std::array<EscSample, 12> samples_{};
  mavros_esc_wheel_odometry::ObservationEngine engine_;
};
constexpr int64_t kSecond = 1000000000LL;

TEST(EscTelemetryTracker, StoppedMotorIsOnlineWhileCountAdvances)
{
  EscTelemetryTracker tracker;
  tracker.observe(0, EscSample{0, 25, 0, 0, 40, 1}, kSecond);
  tracker.observe(0, EscSample{0, 25, 0, 0, 40, 2}, 2 * kSecond);
  const auto state = tracker.project(0, 4 * kSecond);
  EXPECT_TRUE(state.online);
  EXPECT_EQ(state.sample.rpm, 0);
  EXPECT_EQ(state.age_ms, 2000);
}

TEST(EscTelemetryTracker, RunningMotorReportsRawUnsignedMagnitude)
{
  EscTelemetryTracker tracker;
  tracker.observe(1, EscSample{1234, 25, 2, 0.2F, 42, 9}, kSecond);
  const auto state = tracker.project(1, kSecond);
  EXPECT_TRUE(state.online);
  EXPECT_EQ(state.sample.rpm, 1234);
  EXPECT_FLOAT_EQ(state.sample.current, 2.0F);
}

TEST(EscTelemetryTracker, FrozenCountDoesNotRefresh)
{
  EscTelemetryTracker tracker;
  tracker.observe(0, EscSample{0, 25, 0, 0, 40, 1}, kSecond);
  tracker.observe(0, EscSample{0, 25, 0, 0, 40, 1}, 4 * kSecond);
  EXPECT_TRUE(tracker.project(0, 5 * kSecond).stale);
}

TEST(EscTelemetryTracker, CounterWrapRefreshes)
{
  EscTelemetryTracker tracker;
  tracker.observe(0, EscSample{0, 25, 0, 0, 40, 65535}, kSecond);
  tracker.observe(0, EscSample{0, 25, 0, 0, 40, 0}, 2 * kSecond);
  EXPECT_TRUE(tracker.project(0, 4 * kSecond).online);
}

TEST(EscTelemetryTracker, ReconnectClearsFreshness)
{
  EscTelemetryTracker tracker;
  tracker.observe(2, EscSample{0, 25, 0, 0, 40, 1}, kSecond);
  tracker.reset();
  EXPECT_FALSE(tracker.project(2, kSecond).online);
  EXPECT_FALSE(tracker.project(2, kSecond).observed);
}

TEST(EscTelemetryTracker, FourthEmptySlotIsIgnored)
{
  EscTelemetryTracker tracker;
  tracker.observe(3, EscSample{0, 0, 0, 0, 0, 0}, kSecond);
  EXPECT_FALSE(tracker.project(3, kSecond).observed);
}

TEST(VescTelemetryProjection, Esc2StoppedIsFreshBladeTelemetry)
{
  EscTelemetryTracker tracker;
  tracker.observe(2, EscSample{0, 27.5F, 0.1F, 1.2F, 35.0F, 10}, kSecond);
  const auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc(
      tracker.project(2, kSecond));
  EXPECT_TRUE(blade.available);
  EXPECT_EQ(blade.status, mowgli_mavros_bridge::kMowerEscStatusStopped);
  EXPECT_FLOAT_EQ(blade.rpm, 0.0F);
  EXPECT_FLOAT_EQ(blade.current, 0.1F);
  EXPECT_FLOAT_EQ(blade.temperature, 35.0F);
  EXPECT_EQ(blade.stamp_ns, kSecond);
}

TEST(VescTelemetryProjection, Esc2RunningPublishesBladeRpmMagnitude)
{
  EscTelemetryTracker tracker;
  tracker.observe(2, EscSample{2450, 27.2F, 4.5F, 2.0F, 48.0F, 20}, kSecond);
  const auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc(
      tracker.project(2, kSecond));
  EXPECT_TRUE(blade.available);
  EXPECT_EQ(blade.status, mowgli_mavros_bridge::kMowerEscStatusRunning);
  EXPECT_FLOAT_EQ(blade.rpm, 2450.0F);
  EXPECT_FLOAT_EQ(blade.current, 4.5F);
  EXPECT_FLOAT_EQ(blade.temperature, 48.0F);
}

TEST(VescTelemetryProjection, StaleEsc2DoesNotFabricateBladeTelemetry)
{
  EscTelemetryTracker tracker(0.5);
  tracker.observe(2, EscSample{2400, 27.0F, 4.0F, 2.0F, 45.0F, 30}, kSecond);
  const auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc(
      tracker.project(2, 2 * kSecond));
  EXPECT_FALSE(blade.available);
  EXPECT_EQ(blade.status, mowgli_mavros_bridge::kMowerEscStatusUnavailable);
  EXPECT_EQ(blade.stamp_ns, 0);
}

TEST(VescTelemetryProjection, WheelEscTelemetryRemainsTrackerOnly)
{
  EscTelemetryTracker tracker;
  tracker.observe(0, EscSample{1200, 27.0F, 2.0F, 0.5F, 40.0F, 40}, kSecond);
  tracker.observe(1, EscSample{1300, 27.0F, 2.1F, 0.5F, 41.0F, 41}, kSecond);
  EXPECT_TRUE(tracker.project(0, kSecond).online);
  EXPECT_TRUE(tracker.project(1, kSecond).online);
  // No wheel-to-odometry projection exists in this backend bridge path.
}

TEST(CanonicalBlade, MissingTemperaturePreservesRpmCurrentAndFreshTimestamp)
{
  mowgli_mavros_bridge::EscTelemetryTracker tracker;
  Message m; m.esc_index = 7; m.source = m.SOURCE_COMMON;
  m.header.stamp.sec = 1; m.valid = true; m.rpm = -2450; m.rpm_valid = true;
  m.rpm_direction_valid = true; m.current = 4.5F; m.current_valid = true;
  m.temperature_valid = false;
  EXPECT_TRUE(std::isfinite(m.temperature));  // No internal NaN availability sentinel.
  tracker.observe(m, kSecond);
  auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(7, kSecond));
  EXPECT_TRUE(blade.available); EXPECT_FLOAT_EQ(blade.rpm, 2450);
      EXPECT_FLOAT_EQ(blade.current, 4.5F);
  EXPECT_EQ(blade.stamp_ns, kSecond); EXPECT_TRUE(std::isnan(blade.temperature)); // Existing public Status has no flag.
  m.metadata_stamp.sec = 2; m.temperature = 25.34F; m.temperature_valid = true;
  tracker.observe(m, 2 * kSecond);
  blade = mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(7, 2 * kSecond));
  EXPECT_TRUE(blade.available); EXPECT_FLOAT_EQ(blade.temperature, 25.34F);
      EXPECT_EQ(blade.stamp_ns, kSecond);
}
TEST(CanonicalBlade, MetadataCannotRefreshRpmAndExpiresIndependently)
{
  mowgli_mavros_bridge::EscTelemetryTracker tracker;
  Message m; m.esc_index = 2; m.source = m.SOURCE_COMMON; m.valid = true;
  m.header.stamp.sec = 3; m.metadata_stamp.sec = 1;
  m.rpm = 100; m.rpm_valid = true; m.current = 2; m.current_valid = true;
  m.temperature = 20; m.temperature_valid = true; tracker.observe(m, 3 * kSecond);
  auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(2, 5 * kSecond));
  EXPECT_TRUE(blade.available); EXPECT_TRUE(std::isnan(blade.temperature));
  m.metadata_stamp.sec = 7; tracker.observe(m, 7 * kSecond);
  blade = mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(2, 7 * kSecond));
  EXPECT_FALSE(blade.available);
}
TEST(CanonicalBlade, InvalidCurrentMissingStatusAndFutureStampNeverBecomeAvailable)
{
  mowgli_mavros_bridge::EscTelemetryTracker tracker;
  Message m; m.source = m.SOURCE_COMMON; m.esc_index = 2; m.valid = true;
  m.header.stamp.sec = 1; m.rpm = 100; m.rpm_valid = true;
  tracker.observe(m, kSecond);
  EXPECT_FALSE(mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(2,
      kSecond)).available);
  m.current_valid = true; m.current = std::numeric_limits<float>::quiet_NaN();
      tracker.observe(m, kSecond);
  EXPECT_FALSE(mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(2,
      kSecond)).available);
  m.current = 2; m.header.stamp.sec = 10; tracker.observe(m, kSecond);
  EXPECT_FALSE(mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(2,
      kSecond)).available);
}
TEST(CanonicalBlade, Int32MinimumRpmMagnitudeDoesNotOverflow)
{
  mowgli_mavros_bridge::EscTelemetryTracker tracker;
  Message m; m.source = m.SOURCE_COMMON; m.esc_index = 2; m.valid = true;
  m.header.stamp.sec = 1; m.rpm = std::numeric_limits<int32_t>::min(); m.rpm_valid = true;
  m.current = 2; m.current_valid = true; tracker.observe(m, kSecond);
  const auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc(tracker.project(2, kSecond));
  EXPECT_TRUE(blade.available); EXPECT_GT(blade.rpm, 0); EXPECT_TRUE(std::isfinite(blade.rpm));
}
}  // namespace
