#include <gtest/gtest.h>
#include "mowgli_mavros_bridge/readiness_state.hpp"

namespace {
using mowgli_mavros_bridge::ReadinessState;
using mowgli_mavros_bridge::Readiness;
constexpr int64_t kSecond = 1000000000LL;

void recover(ReadinessState & state, int64_t stamp) {
  state.connection(true);
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

TEST(ReadinessState, WheelStaleFailsClosed) {
  ReadinessState state(1.0);
  recover(state, 100);
  const Readiness readiness = state.project(100 + kSecond + 1);
  EXPECT_FALSE(readiness.wheel_fresh);
  EXPECT_FALSE(readiness.ready);
}

TEST(ReadinessState, CachedGnssStatusDoesNotCreateObservation) {
  ReadinessState state(1.0);
  state.connection(true);
  state.gnss(100, 1, "source-a", true);
  state.gnss(900 * 1000 * 1000LL, 1, "source-a", true);
  const Readiness readiness = state.project(kSecond + 101);
  EXPECT_FALSE(readiness.gnss_fresh);
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
  state.gnss(200, 1, "source-b", true);
  EXPECT_TRUE(state.project(200).ready);
}

TEST(ReadinessState, SourceArrivalOrderDoesNotMatter) {
  ReadinessState state(1.0);
  state.connection(true);
  state.traction(100, true);
  state.wheel(100);
  state.gnss(100, 1, "source-a", true);
  EXPECT_TRUE(state.project(100).ready);
}

}  // namespace
