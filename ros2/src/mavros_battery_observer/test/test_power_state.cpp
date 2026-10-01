#include <cmath>
#include <gtest/gtest.h>

#include "mavros_battery_observer/power_state.hpp"

namespace
{
using mavros_battery_observer::BatterySample;
using mavros_battery_observer::PowerState;
constexpr int64_t kSecond = 1000000000LL;

TEST(PowerState, Power1VoltageMeansCharging)
{
  PowerState state(1.0, 14.0, 13.0);
  state.observe_dock(BatterySample{29.4, 1.5, 100});
  const auto out = state.project(100);
  EXPECT_TRUE(out.dock_fresh);
  EXPECT_TRUE(out.charger_enabled);
  EXPECT_DOUBLE_EQ(out.v_charge, 29.4);
}

TEST(PowerState, Power2CurrentIsNegativeInMowgliConvention)
{
  PowerState state(1.0, 14.0, 13.0);
  state.observe_battery(BatterySample{28.0, 0.5, 100});
  const auto out = state.project(100);
  EXPECT_TRUE(out.battery_fresh);
  EXPECT_DOUBLE_EQ(out.v_battery, 28.0);
  EXPECT_DOUBLE_EQ(out.battery_current, -0.5);
}

TEST(PowerState, ChargeCurrentIsPower1MinusRawPower2)
{
  PowerState state(1.0, 14.0, 13.0);
  state.observe_dock(BatterySample{29.4, 1.5, 100});
  state.observe_battery(BatterySample{28.0, 0.5, 100});
  EXPECT_DOUBLE_EQ(state.project(100).charge_current, 1.0);
}

TEST(PowerState, ResidualVoltageBelowThresholdIsNotCharging)
{
  PowerState state(1.0, 14.0, 13.0);
  state.observe_dock(BatterySample{8.0, 0.0, 100});
  const auto out = state.project(100);
  EXPECT_TRUE(out.dock_fresh);
  EXPECT_FALSE(out.charger_enabled);
  EXPECT_DOUBLE_EQ(out.v_charge, 8.0);
}

TEST(PowerState, ChargerPresenceUsesVoltageHysteresis)
{
  PowerState state(1.0, 14.0, 13.0);

  state.observe_dock(BatterySample{14.1, 0.0, 100});
  EXPECT_TRUE(state.project(100).charger_enabled);

  // Stay present inside the 13-14 V hysteresis band.
  state.observe_dock(BatterySample{13.5, 0.0, 200});
  EXPECT_TRUE(state.project(200).charger_enabled);

  state.observe_dock(BatterySample{12.9, 0.0, 300});
  EXPECT_FALSE(state.project(300).charger_enabled);

  // Stay absent inside the hysteresis band until the ON threshold is crossed.
  state.observe_dock(BatterySample{13.8, 0.0, 400});
  EXPECT_FALSE(state.project(400).charger_enabled);
}

TEST(PowerState, StaleDataBecomesUnavailable)
{
  PowerState state(1.0, 14.0, 13.0);
  state.observe_dock(BatterySample{29.4, 1.5, 100});
  state.observe_battery(BatterySample{28.0, 0.5, 100});
  const auto out = state.project(kSecond + 101);
  EXPECT_FALSE(out.dock_fresh);
  EXPECT_FALSE(out.battery_fresh);
  EXPECT_FALSE(out.charger_enabled);
  EXPECT_TRUE(std::isnan(out.charge_current));
}

TEST(PowerState, StaleChargerMustCrossOnThresholdAgain)
{
  PowerState state(1.0, 14.0, 13.0);
  state.observe_dock(BatterySample{29.4, 1.5, 100});
  EXPECT_TRUE(state.project(100).charger_enabled);

  EXPECT_FALSE(state.project(kSecond + 101).charger_enabled);

  state.observe_dock(BatterySample{13.5, 0.0, kSecond + 200});
  EXPECT_FALSE(state.project(kSecond + 200).charger_enabled);
  state.observe_dock(BatterySample{14.1, 0.0, kSecond + 300});
  EXPECT_TRUE(state.project(kSecond + 300).charger_enabled);
}

TEST(PowerState, ResetDropsPreviousGeneration)
{
  PowerState state(1.0, 14.0, 13.0);
  state.observe_battery(BatterySample{28.0, 0.5, 100});
  state.reset();
  EXPECT_FALSE(state.project(100).battery_fresh);
}
}  // namespace
