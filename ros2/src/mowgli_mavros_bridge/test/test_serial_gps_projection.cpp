#include <gtest/gtest.h>
#include "mowgli_mavros_bridge/serial_gps_projection.hpp"

namespace
{
using mowgli_mavros_bridge::SerialGpsRaw;
using mowgli_mavros_bridge::project_serial_gps;

TEST(SerialGpsProjection, ValidGps1Fix)
{
  auto fix = project_serial_gps(SerialGpsRaw{3, 439542266, 22022331, 180640, 68, 22,
                                             967, 1261});
  ASSERT_TRUE(fix);
  EXPECT_NEAR(fix->latitude_deg, 43.9542266, 1e-7);
  EXPECT_NEAR(fix->longitude_deg, 2.2022331, 1e-7);
  EXPECT_EQ(fix->satellites_visible, 22);
  EXPECT_DOUBLE_EQ(*fix->hdop, 0.68);
  EXPECT_DOUBLE_EQ(*fix->horizontal_accuracy_m, 0.967);
}

TEST(SerialGpsProjection, NoFixDoesNotMakeZeroZeroPosition)
{
  EXPECT_FALSE(project_serial_gps(SerialGpsRaw{0, 0, 0, 0, 0, 0, 0, 0}));
}

TEST(SerialGpsProjection, InvalidCoordinatesAreNotPublished)
{
  EXPECT_FALSE(project_serial_gps(SerialGpsRaw{3, INT32_MAX, 0, 0, 0, 0, 0, 0}));
}

TEST(SerialGpsProjection, UnknownAccuracyStaysUnknown)
{
  auto fix = project_serial_gps(SerialGpsRaw{3, 10000000, 20000000, 0,
                                             UINT16_MAX, 8, UINT32_MAX, UINT32_MAX});
  ASSERT_TRUE(fix);
  EXPECT_FALSE(fix->hdop);
  EXPECT_FALSE(fix->horizontal_accuracy_m);
}
}  // namespace
