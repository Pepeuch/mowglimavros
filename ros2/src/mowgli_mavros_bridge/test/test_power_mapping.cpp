#include <gtest/gtest.h>
#include <cmath>
#include <optional>
#include "mowgli_mavros_bridge/power_mapping.hpp"

namespace {
using mowgli_mavros_bridge::BatteryInput;
using mowgli_mavros_bridge::PowerMapping;
BatteryInput obs(int id, int64_t ns, std::optional<double> voltage = 24.0,
                 std::optional<double> current = -2.0, std::optional<double> pct = 0.5,
                 uint8_t charge = 0) { return {id, voltage, current, pct, charge, ns}; }

TEST(PowerMapping, TractionOnlyAndDockAbsence) { PowerMapping m(7, 3, 1); m.observe(obs(3, 10)); auto p=m.project(10); EXPECT_TRUE(p.traction_fresh); EXPECT_FALSE(p.dock_fresh); EXPECT_DOUBLE_EQ(p.v_battery,24); EXPECT_TRUE(std::isnan(p.v_charge)); }
TEST(PowerMapping, MapsConfiguredInstancesNotArrivalOrder) { PowerMapping m(7,3,1); m.observe(obs(3,10,25,4)); m.observe(obs(7,11,30,-2)); auto p=m.project(11); EXPECT_DOUBLE_EQ(p.v_battery,25); EXPECT_DOUBLE_EQ(p.v_charge,30); EXPECT_DOUBLE_EQ(p.dock_current_raw,-2); EXPECT_DOUBLE_EQ(p.charge_current,-6); }
TEST(PowerMapping, IgnoresThirdBattery) { PowerMapping m(7,3,1); m.observe(obs(9,1,99,9)); auto p=m.project(1); EXPECT_FALSE(p.traction_fresh); EXPECT_FALSE(p.dock_fresh); }
TEST(PowerMapping, DockDisappearsWithoutInvalidatingTraction) { PowerMapping m(7,3,1); m.observe(obs(7,100)); m.observe(obs(3,1100000000)); auto p=m.project(1100000000); EXPECT_TRUE(p.traction_fresh); EXPECT_FALSE(p.dock_fresh); }
TEST(PowerMapping, TractionStaleFailsClosed) { PowerMapping m(7,3,1); m.observe(obs(3,1)); EXPECT_FALSE(m.project(1000000002).traction_fresh); }
TEST(PowerMapping, RepeatedValuesAreFreshObservations) { PowerMapping m(7,3,1); m.observe(obs(3,1)); m.observe(obs(3,900000000)); EXPECT_TRUE(m.project(1500000000).traction_fresh); }
TEST(PowerMapping, CachedValuesDoNotBecomeFresh) { PowerMapping m(7,3,1); m.observe(obs(3,1)); EXPECT_FALSE(m.project(2000000000).traction_fresh); }
TEST(PowerMapping, UnavailableFieldsStayUnknown) { PowerMapping m(7,3,1); m.observe(obs(3,1,std::nullopt,std::nullopt,std::nullopt)); auto p=m.project(1); EXPECT_TRUE(std::isnan(p.v_battery)); m.observe(obs(7,1,std::nullopt,std::nullopt)); p=m.project(1); EXPECT_TRUE(std::isnan(p.charge_current)); }
TEST(PowerMapping, Power1MinusPower2UsesRawMeasurements) { PowerMapping m(7,3,1); m.observe(obs(7,1,30,-3)); m.observe(obs(3,1,25,5)); auto p=m.project(1); EXPECT_DOUBLE_EQ(p.dock_current_raw,-3); EXPECT_DOUBLE_EQ(p.traction_current_raw,5); EXPECT_DOUBLE_EQ(p.charge_current,-8); }
TEST(PowerMapping, NetCurrentUnknownUntilDockMonitorCalibrated) { PowerMapping m(7,3,1); m.observe(obs(7,1,30,-1.5)); m.observe(obs(3,1,25,0.4)); auto p=m.project(1); EXPECT_DOUBLE_EQ(p.dock_current_raw,-1.5); EXPECT_DOUBLE_EQ(p.traction_current_raw,0.4); EXPECT_DOUBLE_EQ(p.charge_current,-1.9); EXPECT_DOUBLE_EQ(p.traction_current,-0.4); EXPECT_TRUE(std::isnan(p.battery_net_current)); }
TEST(PowerMapping, NetCurrentUnknownWithoutDock) { PowerMapping m(7,3,1); m.observe(obs(3,1,25,0.4)); auto p=m.project(1); EXPECT_DOUBLE_EQ(p.traction_current,-0.4); EXPECT_TRUE(std::isnan(p.battery_net_current)); }
TEST(PowerMapping, DockStaleMakesNetUnknown) { PowerMapping m(7,3,1); m.observe(obs(7,1,30,-1.5)); m.observe(obs(3,2*1000000000LL,25,0.4)); auto p=m.project(2*1000000000LL); EXPECT_TRUE(std::isnan(p.charge_current)); EXPECT_TRUE(std::isnan(p.battery_net_current)); }
TEST(PowerMapping, PositivePower1VoltageMeansDockedAndCharging) { PowerMapping m(7,3,1); m.observe(obs(7,1,30,2,0.5,1)); EXPECT_TRUE(m.project(1).charger_enabled); EXPECT_EQ(m.project(1).dock_charge_state_raw,1); m.observe(obs(7,2,0,0,0.5,7)); EXPECT_FALSE(m.project(2).charger_enabled); EXPECT_EQ(m.project(2).dock_charge_state_raw,7); m.observe(obs(7,3,std::nullopt,0,0.5,7)); EXPECT_FALSE(m.project(3).charger_enabled); m.observe(obs(7,4,24,0,0.5,6)); EXPECT_TRUE(m.project(4).charger_enabled); EXPECT_FALSE(m.project(1000000005).charger_enabled); }
TEST(PowerMapping, ResetDropsPreReconnectData) { PowerMapping m(7,3,1); m.observe(obs(3,1)); m.reset(); EXPECT_FALSE(m.project(1).traction_fresh); }
TEST(PowerMapping, RejectsInvalidOrEqualMapping) { EXPECT_FALSE(PowerMapping(-1,3,1).valid()); EXPECT_FALSE(PowerMapping(3,3,1).valid()); }
}  // namespace
