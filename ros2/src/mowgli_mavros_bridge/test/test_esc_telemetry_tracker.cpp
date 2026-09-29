#include <gtest/gtest.h>
#include "mowgli_mavros_bridge/esc_telemetry_tracker.hpp"

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
}  // namespace
