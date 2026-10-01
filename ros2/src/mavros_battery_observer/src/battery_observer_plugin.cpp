#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>

#include "mavros/mavros_uas.hpp"
#include "mavros/plugin.hpp"
#include "mavros/plugin_filter.hpp"
#include "mowgli_interfaces/msg/power.hpp"
#include "sensor_msgs/msg/battery_state.hpp"

#include "mavros_battery_observer/power_state.hpp"

namespace mavros_battery_observer
{
namespace
{

std::optional<double> battery_voltage(const mavlink::common::msg::BATTERY_STATUS & battery)
{
  double voltage = 0.0;
  bool available = false;

  for (const auto cell_mv : battery.voltages)
  {
    if (cell_mv == UINT16_MAX)
    {
      break;
    }
    if (cell_mv != UINT16_MAX - 1)
    {
      voltage += cell_mv / 1000.0;
      available = true;
    }
  }

  for (const auto cell_mv : battery.voltages_ext)
  {
    if (cell_mv == 0 || cell_mv == UINT16_MAX)
    {
      break;
    }
    if (cell_mv != UINT16_MAX - 1)
    {
      voltage += cell_mv / 1000.0;
      available = true;
    }
  }

  return available ? std::optional<double>(voltage) : std::nullopt;
}

std::optional<double> battery_current(const mavlink::common::msg::BATTERY_STATUS & battery)
{
  if (battery.current_battery == -1)
  {
    return std::nullopt;
  }
  return battery.current_battery / 100.0;
}

}  // namespace

class BatteryObserverPlugin : public mavros::plugin::Plugin
{
public:
  explicit BatteryObserverPlugin(mavros::plugin::UASPtr uas)
  : Plugin(uas, "battery_observer"),
    dock_battery_instance_(node->declare_parameter<int>("dock_battery_instance", 0)),
    traction_battery_instance_(node->declare_parameter<int>("traction_battery_instance", 1)),
    observation_timeout_s_(node->declare_parameter<double>("observation_timeout_s", 5.0)),
    charger_on_voltage_(node->declare_parameter<double>("charger_on_voltage", 14.0)),
    charger_off_voltage_(node->declare_parameter<double>("charger_off_voltage", 13.0)),
    state_(observation_timeout_s_, charger_on_voltage_, charger_off_voltage_)
  {
    power_pub_ = node->create_publisher<mowgli_interfaces::msg::Power>("~/power", 10);
    battery_pub_ = node->create_publisher<sensor_msgs::msg::BatteryState>("~/battery_state", 10);

    if (dock_battery_instance_ < 0 || dock_battery_instance_ > 255 ||
      traction_battery_instance_ < 0 || traction_battery_instance_ > 255 ||
      dock_battery_instance_ == traction_battery_instance_ || !state_.valid())
    {
      RCLCPP_ERROR(
        get_logger(),
        "Battery observer disabled: POWER1/POWER2 instances must be distinct 0..255, timeout positive, and charger thresholds valid (on > off >= 0).");
      enabled_ = false;
    }
  }

  Subscriptions get_subscriptions() override
  {
    return {make_handler(&BatteryObserverPlugin::handle_battery_status)};
  }

private:
  void handle_battery_status(
    const mavlink::mavlink_message_t *, mavlink::common::msg::BATTERY_STATUS & battery,
    mavros::plugin::filter::SystemAndOk)
  {
    if (!enabled_)
    {
      return;
    }

    const auto stamp = node->now();
    const auto stamp_ns = stamp.nanoseconds();
    const BatterySample sample{battery_voltage(battery), battery_current(battery), stamp_ns};

    if (battery.id == dock_battery_instance_)
    {
      state_.observe_dock(sample);
    }
    else if (battery.id == traction_battery_instance_)
    {
      state_.observe_battery(sample);
    }
    else
    {
      return;
    }

    publish_power(stamp);
    if (battery.id == traction_battery_instance_)
    {
      publish_battery_state(stamp);
    }
  }

  void publish_power(const rclcpp::Time & stamp)
  {
    const auto projection = state_.project(stamp.nanoseconds());
    mowgli_interfaces::msg::Power output;
    output.stamp = stamp;
    output.v_charge = static_cast<float>(projection.v_charge);
    output.v_battery = static_cast<float>(projection.v_battery);
    output.charge_current = static_cast<float>(projection.charge_current);
    output.charger_enabled = projection.charger_enabled;
    output.charger_status = !projection.dock_fresh ? "unavailable" :
      (projection.charger_enabled ? "charging" : "unknown");
    power_pub_->publish(output);
  }

  void publish_battery_state(const rclcpp::Time & stamp)
  {
    const auto projection = state_.project(stamp.nanoseconds());
    sensor_msgs::msg::BatteryState output;
    output.header.stamp = stamp;
    output.location = "POWER2";
    output.voltage = static_cast<float>(projection.v_battery);
    output.current = static_cast<float>(projection.battery_current);
    // MowgliNext owns the voltage-derived battery percentage. Do not promote
    // ArduPilot BATTERY_STATUS.battery_remaining as canonical SoC.
    output.percentage = std::numeric_limits<float>::quiet_NaN();
    output.present = projection.battery_fresh && std::isfinite(projection.v_battery);
    battery_pub_->publish(output);
  }

  int dock_battery_instance_{0};
  int traction_battery_instance_{1};
  double observation_timeout_s_{5.0};
  double charger_on_voltage_{14.0};
  double charger_off_voltage_{13.0};
  bool enabled_{true};
  PowerState state_;
  rclcpp::Publisher<mowgli_interfaces::msg::Power>::SharedPtr power_pub_;
  rclcpp::Publisher<sensor_msgs::msg::BatteryState>::SharedPtr battery_pub_;
};

}  // namespace mavros_battery_observer

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(
  mavros::plugin::PluginFactoryTemplate<mavros_battery_observer::BatteryObserverPlugin>,
  mavros::plugin::PluginFactory)
