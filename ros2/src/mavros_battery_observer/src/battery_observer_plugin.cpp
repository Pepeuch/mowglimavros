#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>

#include "mavros/mavros_uas.hpp"
#include "mavros/plugin.hpp"
#include "mavros/plugin_filter.hpp"
#include "mavros_battery_observer/msg/battery_status.hpp"

namespace mavros_battery_observer
{
class BatteryObserverPlugin : public mavros::plugin::Plugin
{
public:
  explicit BatteryObserverPlugin(mavros::plugin::UASPtr uas) : Plugin(uas, "battery_observer")
  {
    publisher_ = node->create_publisher<msg::BatteryStatus>("~/status", rclcpp::SensorDataQoS());
  }

  Subscriptions get_subscriptions() override
  {
    return {make_handler(&BatteryObserverPlugin::handle_battery_status)};
  }

private:
  void handle_battery_status(const mavlink::mavlink_message_t *, mavlink::common::msg::BATTERY_STATUS & battery,
                             mavros::plugin::filter::SystemAndOk)
  {
    msg::BatteryStatus output;
    output.header.stamp = node->now();
    output.id = battery.id;
    output.current_available = battery.current_battery != -1;
    output.current = output.current_available ? battery.current_battery / 100.0F : std::numeric_limits<float>::quiet_NaN();
    output.percentage_available = battery.battery_remaining >= 0 && battery.battery_remaining <= 100;
    output.percentage = output.percentage_available ? battery.battery_remaining / 100.0F : std::numeric_limits<float>::quiet_NaN();
    output.voltage = 0.0F;
    output.voltage_available = false;
    for (const auto cell_mv : battery.voltages) {
      if (cell_mv == UINT16_MAX) break;
      if (cell_mv != UINT16_MAX - 1) { output.voltage += cell_mv / 1000.0F; output.voltage_available = true; }
    }
    for (const auto cell_mv : battery.voltages_ext) {
      if (cell_mv == 0 || cell_mv == UINT16_MAX) break;
      if (cell_mv != UINT16_MAX - 1) { output.voltage += cell_mv / 1000.0F; output.voltage_available = true; }
    }
    if (!output.voltage_available) output.voltage = std::numeric_limits<float>::quiet_NaN();
    output.charge_state = battery.charge_state;
    publisher_->publish(output);
  }
  rclcpp::Publisher<msg::BatteryStatus>::SharedPtr publisher_;
};
}  // namespace mavros_battery_observer

#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(mavros::plugin::PluginFactoryTemplate<mavros_battery_observer::BatteryObserverPlugin>, mavros::plugin::PluginFactory)
