#include <chrono>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>

#include "mavros/mavros_uas.hpp"
#include "mavros/plugin.hpp"
#include "mavros/plugin_filter.hpp"
#include "mavros_gnss_adapter/adapter_state.hpp"
#include "mavros_gnss_adapter/ellipsoid_altitude.hpp"

namespace mavros_gnss_adapter
{
namespace
{
std::string environment(const char * name, const char * fallback)
{
  const char * value = std::getenv(name);
  std::string result = value ? value : fallback;
  const auto begin = result.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) {return "";}
  result = result.substr(begin, result.find_last_not_of(" \t\r\n") - begin + 1);
  for (auto & ch : result) {ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));}
  return result;
}

int64_t steady_now()
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}
}  // namespace

class GnssAdapterPlugin : public mavros::plugin::Plugin
{
public:
  explicit GnssAdapterPlugin(mavros::plugin::UASPtr uas)
  : Plugin(uas, "mowgli_gnss"), source_(environment("GNSS_MAVROS_SOURCE", "gps1")),
    state_(source_)
  {
    // No canonical endpoint exists in direct mode, even if MAVROS auto-loads us.
    enabled_ = canonical_enabled(environment("GNSS_SOURCE", "direct"), source_);
    if (!enabled_) {return;}
    status_pub_ = node->create_publisher<Status>("/gps/status", 10);
    fix_pub_ = node->create_publisher<sensor_msgs::msg::NavSatFix>("/gps/fix", 10);
    const auto root = "/mavros/universal_gnss/" + source_;
    status_sub_ = node->create_subscription<UniversalStatus>(
      root + "/status", rclcpp::SensorDataQoS(),
      [this](const UniversalStatus & status) {
        std::lock_guard<std::mutex> lock(mutex_);
        status_pub_->publish(state_.observe_status(status, steady_now()));
      });
    fix_sub_ = node->create_subscription<sensor_msgs::msg::NavSatFix>(
      root + "/fix", rclcpp::SensorDataQoS(),
      [this](const sensor_msgs::msg::NavSatFix & fix) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.observe_fix(fix, steady_now());
      });
    timer_ = node->create_wall_timer(std::chrono::milliseconds(20), [this]() {
      std::lock_guard<std::mutex> lock(mutex_);
      const auto now = steady_now();
      if (auto status = state_.expire(now)) {status_pub_->publish(*status);}
      for (const auto & fix : state_.take_fixes(now)) {fix_pub_->publish(fix);}
    });
  }

  Subscriptions get_subscriptions() override
  {
    if (!enabled_) {return {};}
    if (source_ == "gps1") {return {make_handler(&GnssAdapterPlugin::altitude_gps1)};}
    return {make_handler(&GnssAdapterPlugin::altitude_gps2)};
  }

private:
  template<typename Raw>
  void altitude(const mavlink::mavlink_message_t * wire, const Raw & raw, size_t offset)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto height = ellipsoid_altitude(wire->len, offset, raw);
    state_.observe_altitude({node->now().nanoseconds(), raw.lat, raw.lon, raw.alt, height});
  }

  void altitude_gps1(const mavlink::mavlink_message_t * wire,
    mavlink::common::msg::GPS_RAW_INT & raw, mavros::plugin::filter::SystemAndOk)
  {
    altitude(wire, raw, 30);  // GPS_RAW_INT extension field offset in MAVLink common.
  }

  void altitude_gps2(const mavlink::mavlink_message_t * wire,
    mavlink::common::msg::GPS2_RAW & raw, mavros::plugin::filter::SystemAndOk)
  {
    altitude(wire, raw, 37);  // GPS2_RAW: yaw extension precedes alt_ellipsoid.
  }

  std::string source_;
  bool enabled_{false};
  std::mutex mutex_;
  AdapterState state_;
  rclcpp::Publisher<Status>::SharedPtr status_pub_;
  rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr fix_pub_;
  rclcpp::Subscription<UniversalStatus>::SharedPtr status_sub_;
  rclcpp::Subscription<sensor_msgs::msg::NavSatFix>::SharedPtr fix_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
};
}  // namespace mavros_gnss_adapter

#include "mavros/mavros_plugin_register_macro.hpp"
MAVROS_PLUGIN_REGISTER(mavros_gnss_adapter::GnssAdapterPlugin)
