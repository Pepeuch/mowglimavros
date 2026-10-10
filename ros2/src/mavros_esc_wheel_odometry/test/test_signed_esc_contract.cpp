#include "mavros_esc_wheel_odometry/observation_engine.hpp"
#include <gtest/gtest.h>

using namespace mavros_esc_wheel_odometry;

TEST(SignedEscContract, CommonPreservesBothSignsForEveryEsc)
{
  ObservationEngine engine(ObservationConfig{});
  engine.connection(true);
  for (int direction : {1, -1})
  {
    const int64_t stamp = direction == 1 ? 1000000000LL : 2000000000LL;
    CommonStatusPacket packet;
    packet.time_us = stamp / 1000;
    packet.rpm = {direction * 120, direction * 240, direction * 360, 0};
    packet.voltage.fill(24);
    packet.current.fill(1);
    engine.common_status(packet, stamp);
    for (unsigned slot = 0; slot < 3; ++slot)
    {
      const auto observation = engine.esc(slot, stamp);
      EXPECT_TRUE(observation.valid);
      EXPECT_TRUE(observation.rpm_valid);
      EXPECT_TRUE(observation.rpm_direction_valid);
      EXPECT_EQ(observation.rpm, packet.rpm[slot]);
    }
  }
}

TEST(SignedEscContract, LegacyNeverInventsDirectionForAnyEsc)
{
  ObservationEngine engine(ObservationConfig{});
  engine.connection(true);
  LegacyEscPacket packet;
  packet.counts = {1, 1, 1, 0};
  packet.rpm = {120, 240, 360, 0};
  packet.voltage.fill(24);
  packet.current.fill(1);
  engine.legacy_esc(packet, 1000000000LL);
  for (unsigned slot = 0; slot < 3; ++slot)
  {
    const auto observation = engine.esc(slot, 1000000000LL);
    EXPECT_TRUE(observation.valid);
    EXPECT_TRUE(observation.rpm_valid);
    EXPECT_FALSE(observation.rpm_direction_valid);
    EXPECT_EQ(observation.rpm, packet.rpm[slot]);
  }
}
