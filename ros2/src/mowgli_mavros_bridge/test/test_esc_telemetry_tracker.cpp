#include <gtest/gtest.h>
#include "mowgli_mavros_bridge/esc_telemetry_tracker.hpp"
#include "mowgli_mavros_bridge/vesc_telemetry_projection.hpp"

namespace
{
using mowgli_mavros_bridge::EscSample;
using mowgli_mavros_bridge::EscTelemetryTracker;
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
  const auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc2(
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
  const auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc2(
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
  const auto blade = mowgli_mavros_bridge::blade_telemetry_from_esc2(
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
}  // namespace
