#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "mavros/mavros_uas.hpp"
#include "mavros/plugin.hpp"
#include "mavros/plugin_filter.hpp"
#include "mavros_msgs/msg/state.hpp"
#include "nav_msgs/msg/odometry.hpp"

#include "mavros_esc_wheel_odometry/wheel_odometry_core.hpp"

namespace mavros_esc_wheel_odometry
{

class EscWheelOdometryPlugin : public mavros::plugin::Plugin
{
public:
  explicit EscWheelOdometryPlugin(mavros::plugin::UASPtr uas)
  : Plugin(uas, "esc_wheel_odometry"),
    core_(WheelGeometry{
      static_cast<int>(node->declare_parameter<int>("left_esc_slot", -1)),
      static_cast<int>(node->declare_parameter<int>("right_esc_slot", -1)),
      node->declare_parameter<double>("left_wheel_radius_m", 0.0),
      node->declare_parameter<double>("right_wheel_radius_m", 0.0),
      node->declare_parameter<double>("track_width_m", 0.0)})
  {
    left_rpm_instance_ = static_cast<int>(node->declare_parameter<int>("left_rpm_instance", -1));
    right_rpm_instance_ = static_cast<int>(node->declare_parameter<int>("right_rpm_instance", -1));
    expected_esc_telem_mav_offset_ =
      static_cast<int>(node->declare_parameter<int>("expected_esc_telem_mav_offset", -1));
    frame_id_ = node->declare_parameter<std::string>("frame_id", "odom");
    child_frame_id_ = node->declare_parameter<std::string>("child_frame_id", "base_link");
    velocity_stddev_ = node->declare_parameter<double>("velocity_stddev_mps", 0.1);

    if (!core_.valid() || (left_rpm_instance_ != 1 && left_rpm_instance_ != 2) ||
      (right_rpm_instance_ != 1 && right_rpm_instance_ != 2) ||
      left_rpm_instance_ == right_rpm_instance_ || expected_esc_telem_mav_offset_ != 0 ||
      frame_id_.empty() || child_frame_id_.empty() || velocity_stddev_ < 0.0) {
      RCLCPP_ERROR(
        get_logger(),
        "ESC wheel odometry disabled: configure distinct left/right ESC slots in [0, 11], "
        "positive wheel radii/track width, RPM fields 1/2 assigned once each, nonempty frames, "
        "nonnegative velocity_stddev_mps, and expected_esc_telem_mav_offset=0. "
        "Verify ArduPilot ESC_TELEM_MAV_OFS=0 before enabling.");
      return;
    }

    odom_pub_ = node->create_publisher<nav_msgs::msg::Odometry>("~/wheel_odom", 10);
    enable_connection_cb();
    RCLCPP_INFO(get_logger(), "ESC wheel odometry enabled; RPM #226 is gated by selected ESC telemetry counts.");
  }

  Subscriptions get_subscriptions() override
  {
    return {
      make_handler(&EscWheelOdometryPlugin::handle_rpm),
      make_handler(&EscWheelOdometryPlugin::handle_esc_telemetry_1_to_4),
      make_handler(&EscWheelOdometryPlugin::handle_esc_telemetry_5_to_8),
      make_handler(&EscWheelOdometryPlugin::handle_esc_telemetry_9_to_12),
    };
  }

private:
  template<typename EscTelemetryMessage>
  void handle_esc_telemetry(const EscTelemetryMessage & telemetry, int group_offset)
  {
    std::array<uint16_t, 4> counts{};
    for (size_t index = 0; index < counts.size(); ++index) {
      counts[index] = telemetry.count[index];
    }
    core_.receive_esc_counts(group_offset, counts);
  }

  void handle_rpm(
    const mavlink::mavlink_message_t * /* message */,
    mavlink::ardupilotmega::msg::RPM & rpm,
    mavros::plugin::filter::SystemAndOk /* filter */)
  {
    const double left_rpm = left_rpm_instance_ == 1 ? rpm.rpm1 : rpm.rpm2;
    const double right_rpm = right_rpm_instance_ == 1 ? rpm.rpm1 : rpm.rpm2;
    const auto observation = core_.receive_rpm(left_rpm, right_rpm, node->now().nanoseconds());
    if (!observation || !odom_pub_) {
      return;
    }

    nav_msgs::msg::Odometry odom;
    odom.header.stamp.sec = static_cast<int32_t>(observation->receipt_stamp_ns / 1000000000LL);
    odom.header.stamp.nanosec =
      static_cast<uint32_t>(observation->receipt_stamp_ns % 1000000000LL);
    odom.header.frame_id = frame_id_;
    odom.child_frame_id = child_frame_id_;
    odom.twist.twist.linear.x = observation->linear_x_mps;
    odom.twist.twist.angular.z = observation->angular_z_rps;
    const double variance = velocity_stddev_ * velocity_stddev_;
    odom.twist.covariance[0] = variance;
    odom.twist.covariance[7] = variance;
    odom.twist.covariance[35] = variance;
    odom.pose.covariance[0] = 1.0e6;
    odom.pose.covariance[7] = 1.0e6;
    odom.pose.covariance[14] = 1.0e6;
    odom.pose.covariance[21] = 1.0e6;
    odom.pose.covariance[28] = 1.0e6;
    odom.pose.covariance[35] = 1.0e6;
    odom_pub_->publish(odom);
  }

  void handle_esc_telemetry_1_to_4(
    const mavlink::mavlink_message_t * /* message */,
    mavlink::ardupilotmega::msg::ESC_TELEMETRY_1_TO_4 & telemetry,
    mavros::plugin::filter::SystemAndOk /* filter */)
  {
    handle_esc_telemetry(telemetry, 0);
  }

  void handle_esc_telemetry_5_to_8(
    const mavlink::mavlink_message_t * /* message */,
    mavlink::ardupilotmega::msg::ESC_TELEMETRY_5_TO_8 & telemetry,
    mavros::plugin::filter::SystemAndOk /* filter */)
  {
    handle_esc_telemetry(telemetry, 4);
  }

  void handle_esc_telemetry_9_to_12(
    const mavlink::mavlink_message_t * /* message */,
    mavlink::ardupilotmega::msg::ESC_TELEMETRY_9_TO_12 & telemetry,
    mavros::plugin::filter::SystemAndOk /* filter */)
  {
    handle_esc_telemetry(telemetry, 8);
  }

  void connection_cb(bool /* connected */) override
  {
    core_.reset();
  }

  WheelOdometryCore core_;
  std::string frame_id_;
  std::string child_frame_id_;
  int left_rpm_instance_{-1};
  int right_rpm_instance_{-1};
  int expected_esc_telem_mav_offset_{-1};
  double velocity_stddev_{0.1};
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
};

}  // namespace mavros_esc_wheel_odometry

#include <mavros/mavros_plugin_register_macro.hpp>
MAVROS_PLUGIN_REGISTER(mavros_esc_wheel_odometry::EscWheelOdometryPlugin)
