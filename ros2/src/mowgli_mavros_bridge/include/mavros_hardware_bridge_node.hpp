#pragma once

#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <geometry_msgs/msg/twist_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>

#include "mowgli_mavros_bridge/blade_control.hpp"
#include "mowgli_mavros_bridge/esc_telemetry_tracker.hpp"
#include "mowgli_mavros_bridge/firmware_provider.hpp"
#include "mowgli_mavros_bridge/readiness_state.hpp"
#include "mowgli_mavros_bridge/safety_state.hpp"
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <mavros_esc_wheel_odometry/msg/esc_observation.hpp>
#include <mavros_msgs/msg/manual_control.hpp>
#include <mavros_msgs/msg/mavlink.hpp>
#include <mavros_msgs/msg/rc_out.hpp>
#include <mavros_msgs/msg/state.hpp>
#include <mavros_msgs/msg/sys_status.hpp>
#include <mavros_msgs/srv/command_bool.hpp>
#include <mavros_msgs/srv/command_long.hpp>
#include <mavros_msgs/srv/set_mode.hpp>
#include <mowgli_interfaces/msg/emergency.hpp>
#include <mowgli_interfaces/msg/gnss_status.hpp>
#include <mowgli_interfaces/msg/high_level_status.hpp>
#include <mowgli_interfaces/msg/power.hpp>
#include <mowgli_interfaces/msg/status.hpp>
#include <mowgli_interfaces/srv/emergency_stop.hpp>
#include <mowgli_interfaces/srv/mower_control.hpp>
#include <std_srvs/srv/trigger.hpp>

namespace mowgli_mavros_bridge
{

class MavrosHardwareBridgeNode : public rclcpp::Node
{
public:
  explicit MavrosHardwareBridgeNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  void create_publishers();
  void create_subscriptions();
  void create_services();
  void create_timers();
  void create_clients();

  void on_cmd_vel(const geometry_msgs::msg::TwistStamped::SharedPtr msg);
  void on_high_level_status(const mowgli_interfaces::msg::HighLevelStatus::SharedPtr msg);

  void on_mavros_state(const mavros_msgs::msg::State::SharedPtr msg);
  void on_mavros_sys_status(const mavros_msgs::msg::SysStatus::SharedPtr msg);
  void on_mavlink_source(const mavros_msgs::msg::Mavlink::SharedPtr msg);
  void on_mavros_imu(const sensor_msgs::msg::Imu::SharedPtr msg);
  void on_power(const mowgli_interfaces::msg::Power::SharedPtr msg);
  void on_esc_telemetry(const mavros_esc_wheel_odometry::msg::EscObservation::SharedPtr msg);
  void on_gnss_status(const mowgli_interfaces::msg::GnssStatus::SharedPtr msg);
  void on_wheel_odom(const nav_msgs::msg::Odometry::SharedPtr msg);

  void on_mower_control(
      const std::shared_ptr<rmw_request_id_t> header,
      const std::shared_ptr<mowgli_interfaces::srv::MowerControl::Request> request);

  void on_emergency_stop(
      const std::shared_ptr<mowgli_interfaces::srv::EmergencyStop::Request> request,
      std::shared_ptr<mowgli_interfaces::srv::EmergencyStop::Response> response);
  void on_reboot_board(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                       std::shared_ptr<std_srvs::srv::Trigger::Response> response);

  void publish_status();
  void publish_emergency();
  void publish_readiness();

  bool send_arm_command(bool arm);
  bool request_blade_neutral();
  void drive_blade_locked();
  void reply_blade(const std::shared_ptr<rmw_request_id_t>& header, bool success);
  bool send_mode_command(const std::string& mode);
  bool request_blade_disarm();
  void publish_neutral_manual_control();
  bool request_hold_and_blade_disarm();
  bool high_level_motion_active(uint8_t state) const;

private:
  std::mutex mutex_;
  std::unique_ptr<FirmwareProvider> firmware_provider_;

  double status_publish_rate_hz_{10.0};
  double manual_control_linear_scale_{1000.0};
  double manual_control_yaw_scale_{1000.0};
  bool mowing_enabled_{true};
  double battery_observation_timeout_s_{5.0};
  double esc_observation_timeout_s_{3.0};
  bool gnss_required_{true};
  bool wheel_odometry_required_{false};
  int64_t tilt_emergency_ms_{500};
  int64_t imu_inclination_threshold_{56};
  int64_t tilt_started_ns_{0};
  bool tilt_disarm_sent_{false};
  bool double_lift_disarm_sent_{false};
  bool rain_detected_{false};
  bool esc_power_{true};
  bool raspberry_pi_power_{true};
  bool sound_module_available_{false};
  bool sound_module_busy_{false};
  bool ui_board_available_{false};

  bool mow_enabled_{false};
  BladeControl blade_control_;
  struct BladeReply
  {
    std::shared_ptr<rmw_request_id_t> header;
    uint64_t revision;
    int64_t started_ms;
  };
  std::vector<BladeReply> blade_replies_;
  std::map<uint64_t, int64_t> blade_rpc_ids_;
  int64_t last_fcu_receipt_ms_{-1}, last_hardware_safety_ms_{-1};
  int64_t last_blade_output_stamp_{0};
  struct BladeWireExpectation
  {
    int pwm;
    int64_t until_ms;
  };
  std::deque<BladeWireExpectation> blade_wire_expectations_;
  uint8_t mow_direction_{0};

  SafetyState safety_state_;

  bool is_charging_{false};
  bool charger_enabled_{false};
  std::string charger_status_{"unknown"};

  ReadinessState readiness_{5.0};
  EscTelemetryTracker esc_tracker_{3.0};
  int64_t right_esc_slot_{0}, left_esc_slot_{1}, blade_esc_slot_{2};
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_validation_callback_;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr parameter_apply_callback_;

  uint8_t mower_esc_status_{0};
  float mower_esc_temperature_{0.0F};
  float mower_esc_current_{0.0F};
  float mower_motor_temperature_{0.0F};
  float mower_motor_rpm_{0.0F};

  mavros_msgs::msg::State mavros_state_{};
  sensor_msgs::msg::Imu last_imu_{};
  mowgli_interfaces::msg::Power last_power_{};
  int64_t last_power_receipt_ns_{0};
  mowgli_interfaces::msg::HighLevelStatus last_high_level_status_{};

  rclcpp::Publisher<mowgli_interfaces::msg::Status>::SharedPtr pub_status_;
  rclcpp::Publisher<mowgli_interfaces::msg::Emergency>::SharedPtr pub_emergency_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr pub_imu_;
  rclcpp::Publisher<mavros_msgs::msg::ManualControl>::SharedPtr pub_manual_control_;
  rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr pub_readiness_;

  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr sub_cmd_vel_;
  rclcpp::Subscription<mowgli_interfaces::msg::HighLevelStatus>::SharedPtr sub_hl_status_;
  rclcpp::Subscription<mavros_msgs::msg::State>::SharedPtr sub_mavros_state_;
  rclcpp::Subscription<mavros_msgs::msg::SysStatus>::SharedPtr sub_mavros_sys_status_;
  rclcpp::Subscription<mavros_msgs::msg::RCOut>::SharedPtr sub_blade_output_;
  rclcpp::Subscription<mavros_msgs::msg::Mavlink>::SharedPtr sub_blade_wire_;
  rclcpp::Subscription<mavros_msgs::msg::Mavlink>::SharedPtr sub_mavlink_source_;
  rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr sub_mavros_imu_;
  rclcpp::Subscription<mowgli_interfaces::msg::Power>::SharedPtr sub_power_;
  rclcpp::Subscription<mavros_esc_wheel_odometry::msg::EscObservation>::SharedPtr
      sub_esc_telemetry_;
  rclcpp::Subscription<mowgli_interfaces::msg::GnssStatus>::SharedPtr sub_gnss_status_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr sub_wheel_odom_;

  rclcpp::Service<mowgli_interfaces::srv::MowerControl>::SharedPtr srv_mower_control_;
  rclcpp::Service<mowgli_interfaces::srv::EmergencyStop>::SharedPtr srv_emergency_stop_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr srv_reboot_board_;

  rclcpp::Client<mavros_msgs::srv::CommandBool>::SharedPtr cli_arm_;
  rclcpp::Client<mavros_msgs::srv::SetMode>::SharedPtr cli_set_mode_;
  rclcpp::Client<mavros_msgs::srv::CommandLong>::SharedPtr cli_command_long_;

  rclcpp::TimerBase::SharedPtr timer_status_;
  rclcpp::TimerBase::SharedPtr timer_diagnostics_;
  rclcpp::TimerBase::SharedPtr timer_blade_;
};

}  // namespace mowgli_mavros_bridge
