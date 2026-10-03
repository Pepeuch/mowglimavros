#include <gtest/gtest.h>
#include "mowgli_mavros_bridge/readiness_state.hpp"

namespace {
using mowgli_mavros_bridge::ReadinessState;
using mowgli_mavros_bridge::Readiness;
constexpr int64_t kSecond = 1000000000LL;

void recover(ReadinessState & state, int64_t stamp) {
  state.connection(true);
  state.imu(stamp);
  state.gnss(stamp, 1, "source-a", true);
  state.wheel(stamp);
  state.traction(stamp, true);
}

TEST(ReadinessState, StartupDisconnected) {
  ReadinessState state(1.0);
  const Readiness readiness = state.project(0);
  EXPECT_FALSE(readiness.connected);
  EXPECT_FALSE(readiness.ready);
}

TEST(ReadinessState, ConnectedWithoutObservationsIsNotReady) {
  ReadinessState state(1.0);
  state.connection(true);
  const Readiness readiness = state.project(0);
  EXPECT_TRUE(readiness.connected);
  EXPECT_FALSE(readiness.ready);
}

TEST(ReadinessState, RequiredSourcesRecoverAndDockIsNonBlocking) {
  ReadinessState state(1.0);
  recover(state, 100);
  EXPECT_TRUE(state.project(100).ready);
  // Dock liveness is intentionally not an input to ReadinessState.
  EXPECT_TRUE(state.project(100).ready);
}

TEST(ReadinessState, FreshZeroSpeedWheelObservationIsValid) {
  ReadinessState state(1.0);
  recover(state, 100);
  // Odometry carries no speed validity bit: a new zero-speed message is fresh.
  state.wheel(200);
  EXPECT_TRUE(state.project(200).wheel_fresh);
}

TEST(ReadinessState, TractionStaleFailsClosed) {
  ReadinessState state(1.0);
  recover(state, 100);
  const Readiness readiness = state.project(100 + kSecond + 1);
  EXPECT_FALSE(readiness.traction_fresh);
  EXPECT_FALSE(readiness.ready);
}

TEST(ReadinessState, WheelStaleIsInformationalByDefault) {
  ReadinessState state(1.0);
  recover(state, 100);
  const Readiness readiness = state.project(100 + kSecond + 1);
  EXPECT_FALSE(readiness.wheel_fresh);
  EXPECT_FALSE(readiness.ready);  // Other required sources are stale too.
}

TEST(ReadinessState, WheelOptionalWithAllRequiredSourcesFresh) {
  ReadinessState state(1.0);
  state.connection(true);
  state.imu(2 * kSecond);
  state.traction(2 * kSecond, true);
  state.gnss(2 * kSecond, 1, "serial-gps", true);
  EXPECT_FALSE(state.project(2 * kSecond).wheel_fresh);
  EXPECT_TRUE(state.project(2 * kSecond).ready);
}

TEST(ReadinessState, WheelRequiredOnlyWhenConfigured) {
  ReadinessState state(1.0, true, true);
  state.connection(true);
  state.imu(2 * kSecond);
  state.traction(2 * kSecond, true);
  state.gnss(2 * kSecond, 1, "serial-gps", true);
  EXPECT_FALSE(state.project(2 * kSecond).ready);
  state.wheel(2 * kSecond);
  EXPECT_TRUE(state.project(2 * kSecond).ready);
}

TEST(ReadinessState, ImuStaleBlocksReady) {
  ReadinessState state(1.0);
  state.connection(true);
  state.imu(kSecond);
  state.traction(3 * kSecond, true);
  state.gnss(3 * kSecond, 1, "serial-gps", true);
  EXPECT_FALSE(state.project(3 * kSecond).ready);
}

TEST(ReadinessState, CachedGnssStatusDoesNotCreateObservation) {
  ReadinessState state(1.0);
  state.connection(true);
  state.gnss(100, 1, "source-a", true);
  state.gnss(900 * 1000 * 1000LL, 1, "source-a", true);
  const Readiness readiness = state.project(kSecond + 101);
  EXPECT_FALSE(readiness.gnss_fresh);
}

TEST(ReadinessState, RestartedGnssSequenceRecoversAfterOldDataExpires) {
  ReadinessState state(1.0);
  state.connection(true);
  state.gnss(100, 100, "mavros_gps1", true);
  state.gnss(200, 1, "mavros_gps1", true);
  EXPECT_FALSE(state.project(100 + kSecond + 1).gnss_fresh);
  state.gnss(100 + kSecond + 2, 1, "mavros_gps1", true);
  EXPECT_TRUE(state.project(100 + kSecond + 2).gnss_fresh);
}

TEST(ReadinessState, ReconnectDropsPreviousGeneration) {
  ReadinessState state(1.0);
  recover(state, 100);
  state.connection(false);
  state.connection(true);
  EXPECT_FALSE(state.project(200).ready);
  state.wheel(200);
  state.traction(200, true);
  EXPECT_FALSE(state.project(200).ready);
  state.imu(200);
  state.gnss(200, 1, "source-b", true);
  EXPECT_TRUE(state.project(200).ready);
}

TEST(ReadinessState, SourceArrivalOrderDoesNotMatter) {
  ReadinessState state(1.0);
  state.connection(true);
  state.traction(100, true);
  state.imu(100);
  state.wheel(100);
  state.gnss(100, 1, "source-a", true);
  EXPECT_TRUE(state.project(100).ready);
}

}  // namespace
