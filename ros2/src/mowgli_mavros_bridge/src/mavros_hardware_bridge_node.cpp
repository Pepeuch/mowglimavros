#include "mavros_hardware_bridge_node.hpp"
#include "mowgli_mavros_bridge/vesc_telemetry_projection.hpp"
#include "mowgli_mavros_bridge/rover_manual_control.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
namespace mowgli_mavros_bridge
{

using namespace std::chrono_literals;

MavrosHardwareBridgeNode::MavrosHardwareBridgeNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("hardware_bridge", options)
{
  // Set only by the backend launch after its single firmware resolution.
  // Standalone bridge execution preserves its existing Rover conversion.
  const char * resolved_firmware = std::getenv("MAVROS_RESOLVED_FIRMWARE");
  firmware_provider_ = make_firmware_provider(
      resolved_firmware ? resolved_firmware : "ardupilot");

  status_publish_rate_hz_ = declare_parameter<double>("status_publish_rate_hz", 10.0);
  manual_control_enabled_ = declare_parameter<bool>("manual_control_enabled", false);
  neutral_manual_control_enabled_ =
      declare_parameter<bool>("neutral_manual_control_enabled", false);
  manual_control_linear_scale_ = declare_parameter<double>("manual_control_linear_scale", 1000.0);
  manual_control_yaw_scale_ = declare_parameter<double>("manual_control_yaw_scale", 1000.0);
  blade_control_enabled_ = declare_parameter<bool>("blade_control_enabled", false);
  battery_observation_timeout_s_ = declare_parameter<double>("battery_observation_timeout_s", 5.0);
  const auto readiness_timeout_s = declare_parameter<double>("readiness_observation_timeout_s", 5.0);
  gnss_required_ = declare_parameter<bool>("gnss_required", true);
  wheel_odometry_required_ = declare_parameter<bool>("wheel_odometry_required", false);
  readiness_ = ReadinessState(readiness_timeout_s, gnss_required_, wheel_odometry_required_);
  esc_observation_timeout_s_ = declare_parameter<double>("esc_observation_timeout_s", 3.0);
  esc_tracker_ = EscTelemetryTracker(esc_observation_timeout_s_);
  right_esc_slot_ = declare_parameter<int64_t>("right_esc_slot", 0);
  left_esc_slot_ = declare_parameter<int64_t>("left_esc_slot", 1);
  blade_esc_slot_ = declare_parameter<int64_t>("blade_esc_slot", 2);
  auto roles_valid = [](int64_t right, int64_t left, int64_t blade) {
    const int64_t roles[] = {right, left, blade};
    for (int64_t role : roles) {if (role < -1 || role >= 64) {return false;}}
    return (right < 0 || left < 0 || right != left) &&
      (right < 0 || blade < 0 || right != blade) && (left < 0 || blade < 0 || left != blade);
  };
  if (!roles_valid(right_esc_slot_, left_esc_slot_, blade_esc_slot_)) {
    throw std::invalid_argument("ESC role mappings must be distinct indexes in [0,63], or -1 disabled");
  }
  esc_mapping_callback_ = add_on_set_parameters_callback(
    [this, roles_valid](const std::vector<rclcpp::Parameter> & parameters) {
      std::lock_guard<std::mutex> lock(mutex_);
      int64_t right = right_esc_slot_, left = left_esc_slot_, blade = blade_esc_slot_;
      rcl_interfaces::msg::SetParametersResult result; result.successful = false;
      try {
        for (const auto & parameter : parameters) {
          if (parameter.get_name() == "right_esc_slot") {right = parameter.as_int();}
          if (parameter.get_name() == "left_esc_slot") {left = parameter.as_int();}
          if (parameter.get_name() == "blade_esc_slot") {blade = parameter.as_int();}
        }
        if (!roles_valid(right, left, blade)) {result.reason = "invalid or overlapping ESC role mapping"; return result;}
        result.successful = true;
      } catch (const std::exception & error) {result.reason = error.what();}
      return result;
    });
  esc_mapping_apply_ = add_post_set_parameters_callback(
    [this](const std::vector<rclcpp::Parameter> & parameters) {
      std::lock_guard<std::mutex> lock(mutex_); bool changed = false;
      for (const auto & p : parameters) {
        if (p.get_name() == "right_esc_slot") {right_esc_slot_ = p.as_int(); changed = true;}
        if (p.get_name() == "left_esc_slot") {left_esc_slot_ = p.as_int(); changed = true;}
        if (p.get_name() == "blade_esc_slot") {blade_esc_slot_ = p.as_int(); changed = true;}
      }
      if (changed) {esc_tracker_.reset();}
    });
  const auto emergency_defaults = firmware_provider_->default_emergency_policy();
  emergency_mode_ = declare_parameter<std::string>("emergency_mode", emergency_defaults.mode);
  emergency_disarm_ = declare_parameter<bool>("emergency_disarm", emergency_defaults.disarm);
  rain_detected_ = declare_parameter<bool>("rain_detected_default", false);
  esc_power_ = declare_parameter<bool>("esc_power_default", false);
  raspberry_pi_power_ = declare_parameter<bool>("raspberry_pi_power_default", true);

  create_publishers();
  create_subscriptions();
  create_services();
  create_clients();
  create_timers();

  if (!manual_control_enabled_)
  {
    RCLCPP_WARN(
        get_logger(),
        "manual_control_enabled=false: /cmd_vel commands will be ignored until MAVROS manual control mapping is validated.");
  }
  if (!blade_control_enabled_)
  {
    RCLCPP_WARN(
        get_logger(),
        "blade_control_enabled=false: mower_control remains provisional and will report failure.");
  }
  RCLCPP_INFO(get_logger(), "MAVROS hardware bridge started.");
}

void MavrosHardwareBridgeNode::create_publishers()
{
  pub_status_ = create_publisher<mowgli_interfaces::msg::Status>("~/status", 10);
  pub_emergency_ = create_publisher<mowgli_interfaces::msg::Emergency>("~/emergency", 10);
  pub_imu_ = create_publisher<sensor_msgs::msg::Imu>("~/imu/data_raw", 10);
  pub_manual_control_ =
      create_publisher<mavros_msgs::msg::ManualControl>("/mavros/manual_control/send", 10);
  pub_readiness_ = create_publisher<diagnostic_msgs::msg::DiagnosticArray>("/diagnostics", 10);
}

void MavrosHardwareBridgeNode::create_subscriptions()
{
  auto default_qos = rclcpp::SystemDefaultsQoS();
  auto sensor_qos = rclcpp::SensorDataQoS();

  sub_cmd_vel_ = create_subscription<geometry_msgs::msg::TwistStamped>(
      "/cmd_vel",
      default_qos,
      std::bind(&MavrosHardwareBridgeNode::on_cmd_vel, this, std::placeholders::_1));

  sub_hl_status_ = create_subscription<mowgli_interfaces::msg::HighLevelStatus>(
      "/behavior_tree_node/high_level_status",
      default_qos,
      std::bind(&MavrosHardwareBridgeNode::on_high_level_status, this, std::placeholders::_1));

  sub_mavros_state_ = create_subscription<mavros_msgs::msg::State>(
      "/mavros/state",
      default_qos,
      std::bind(&MavrosHardwareBridgeNode::on_mavros_state, this, std::placeholders::_1));

  sub_mavros_imu_ = create_subscription<sensor_msgs::msg::Imu>(
      "/mavros/imu/data",
      sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_mavros_imu, this, std::placeholders::_1));

  sub_power_ = create_subscription<mowgli_interfaces::msg::Power>(
      "/hardware_bridge/power", sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_power, this, std::placeholders::_1));
  sub_esc_telemetry_ = create_subscription<mavros_esc_wheel_odometry::msg::EscObservation>(
      "/mavros/esc_wheel_odometry/esc_observation", rclcpp::SensorDataQoS().keep_last(64),
      std::bind(&MavrosHardwareBridgeNode::on_esc_telemetry, this, std::placeholders::_1));
  sub_gnss_status_ = create_subscription<mowgli_interfaces::msg::GnssStatus>(
      "/gps/status", sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_gnss_status, this, std::placeholders::_1));
  sub_wheel_odom_ = create_subscription<nav_msgs::msg::Odometry>(
      "/wheel_odom", sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_wheel_odom, this, std::placeholders::_1));

}

void MavrosHardwareBridgeNode::create_services()
{
  srv_mower_control_ = create_service<mowgli_interfaces::srv::MowerControl>(
      "~/mower_control",
      std::bind(&MavrosHardwareBridgeNode::on_mower_control,
                this,
                std::placeholders::_1,
                std::placeholders::_2));

  srv_emergency_stop_ = create_service<mowgli_interfaces::srv::EmergencyStop>(
      "~/emergency_stop",
      std::bind(&MavrosHardwareBridgeNode::on_emergency_stop,
                this,
                std::placeholders::_1,
                std::placeholders::_2));
}

void MavrosHardwareBridgeNode::create_clients()
{
  cli_arm_ = create_client<mavros_msgs::srv::CommandBool>("/mavros/cmd/arming");
  cli_set_mode_ = create_client<mavros_msgs::srv::SetMode>("/mavros/set_mode");
}

void MavrosHardwareBridgeNode::create_timers()
{
  const auto period = std::chrono::duration<double>(1.0 / std::max(1.0, status_publish_rate_hz_));

  timer_status_ = create_wall_timer(std::chrono::duration_cast<std::chrono::milliseconds>(period),
                                    [this]()
                                    {
                                      publish_status();
                                      publish_emergency();
                                    });
  timer_diagnostics_ = create_wall_timer(1s, [this]() {
    publish_readiness();
  });
}

void MavrosHardwareBridgeNode::on_cmd_vel(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
  if (!manual_control_allowed(*msg, manual_control_enabled_, neutral_manual_control_enabled_))
  {
    RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000,
        "Ignoring /cmd_vel: drive is disabled, neutral test not enabled, or command invalid.");
    return;
  }

  // The Rover axes are fixed by ArduPilot; output polarity and scaling still
  // require physical validation before this publisher can be enabled.
  auto cmd = firmware_provider_->manual_control_from_twist(
      *msg, manual_control_linear_scale_, manual_control_yaw_scale_);
  cmd.header.stamp = now();
  pub_manual_control_->publish(cmd);
}

void MavrosHardwareBridgeNode::on_high_level_status(
    const mowgli_interfaces::msg::HighLevelStatus::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  last_high_level_status_ = *msg;
}

void MavrosHardwareBridgeNode::on_mavros_state(const mavros_msgs::msg::State::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (mavros_state_.connected && !msg->connected) {
    last_power_receipt_ns_ = 0;
    last_power_ = mowgli_interfaces::msg::Power{};
    esc_tracker_.reset();
    is_charging_ = false;
    charger_enabled_ = false;
    charger_status_ = "unknown";
  }
  readiness_.connection(msg->connected);
  mavros_state_ = *msg;
}

void MavrosHardwareBridgeNode::on_mavros_imu(const sensor_msgs::msg::Imu::SharedPtr msg)
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    last_imu_ = *msg;
    readiness_.imu(now().nanoseconds());
  }
  pub_imu_->publish(*msg);
}

void MavrosHardwareBridgeNode::on_power(
    const mowgli_interfaces::msg::Power::SharedPtr msg)
{
  const auto receipt_ns = now().nanoseconds();
  std::lock_guard<std::mutex> lock(mutex_);
  last_power_ = *msg;
  last_power_receipt_ns_ = receipt_ns;
  is_charging_ = msg->charger_enabled;
  charger_enabled_ = msg->charger_enabled;
  charger_status_ = msg->charger_status;
  readiness_.traction(receipt_ns, std::isfinite(msg->v_battery));
}

void MavrosHardwareBridgeNode::on_esc_telemetry(
    const mavros_esc_wheel_odometry::msg::EscObservation::SharedPtr msg)
{
  const auto receipt_ns = now().nanoseconds();
  std::lock_guard<std::mutex> lock(mutex_);
  if (mavros_state_.connected) {esc_tracker_.observe(*msg, receipt_ns);}
}

void MavrosHardwareBridgeNode::on_gnss_status(const mowgli_interfaces::msg::GnssStatus::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  readiness_.gnss(now().nanoseconds(), msg->position_observation_sequence,
                  msg->backend, msg->fix_valid);
}

void MavrosHardwareBridgeNode::on_wheel_odom(const nav_msgs::msg::Odometry::SharedPtr /*msg*/)
{
  std::lock_guard<std::mutex> lock(mutex_);
  readiness_.wheel(now().nanoseconds());
}

void MavrosHardwareBridgeNode::on_mower_control(
    const std::shared_ptr<mowgli_interfaces::srv::MowerControl::Request> request,
    std::shared_ptr<mowgli_interfaces::srv::MowerControl::Response> response)
{
  if (!blade_control_enabled_)
  {
    RCLCPP_WARN(
        get_logger(),
        "Mower control requested, but blade_control_enabled=false because the Pixhawk blade path is still provisional.");
    response->success = false;
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    mow_enabled_ = request->mow_enabled;
    mow_direction_ = request->mow_direction;
  }

  RCLCPP_WARN(
      get_logger(),
      "Mower control requested (enabled=%d, direction=%d) but cutting motor mapping on the Pixhawk/ArduPilot is not implemented yet.",
      request->mow_enabled,
      request->mow_direction);

  response->success = false;
}

void MavrosHardwareBridgeNode::on_emergency_stop(
    const std::shared_ptr<mowgli_interfaces::srv::EmergencyStop::Request> request,
    std::shared_ptr<mowgli_interfaces::srv::EmergencyStop::Response> response)
{
  const bool emergency_requested = (request->emergency != 0U);

  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (emergency_requested)
    {
      emergency_active_ = true;
      emergency_latched_ = true;
      emergency_reason_ = "SERVICE_EMERGENCY_STOP";
      mow_enabled_ = false;
    }
    else
    {
      emergency_active_ = false;
      emergency_reason_ = "NONE";
    }
  }

  if (!emergency_requested)
  {
    response->success = true;
    return;
  }

  // This service reports whether the emergency request was accepted locally
  // and forwarded to MAVROS. It does not imply the autopilot has already
  // confirmed or completed the requested state change.
  const auto policy = firmware_provider_->emergency_policy(emergency_mode_, emergency_disarm_);
  bool request_sent = true;

  if (!policy.mode.empty())
  {
    request_sent = send_mode_command(policy.mode) && request_sent;
  }
  if (policy.disarm)
  {
    request_sent = send_arm_command(false) && request_sent;
  }

  response->success = request_sent;

  if (request_sent)
  {
    RCLCPP_WARN(
        get_logger(),
        "Emergency stop request sent to MAVROS (mode='%s', disarm=%s). Autopilot confirmation will be logged asynchronously.",
        policy.mode.c_str(),
        policy.disarm ? "true" : "false");
  }
  else
  {
    RCLCPP_ERROR(
        get_logger(),
        "Emergency stop request could not be fully sent to MAVROS (mode='%s', disarm=%s).",
        policy.mode.c_str(),
        policy.disarm ? "true" : "false");
  }
}

void MavrosHardwareBridgeNode::publish_status()
{
  mowgli_interfaces::msg::Status msg;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto status_stamp_ns = readiness_.status_stamp_ns();
    if (status_stamp_ns > 0) {
      msg.stamp.sec = static_cast<int32_t>(status_stamp_ns / 1000000000LL);
      msg.stamp.nanosec = static_cast<uint32_t>(status_stamp_ns % 1000000000LL);
    }
    msg.raspberry_pi_power = raspberry_pi_power_;
    msg.is_charging = is_charging_;
    msg.esc_power = esc_power_;
    msg.rain_detected = rain_detected_;
    msg.sound_module_available = sound_module_available_;
    msg.sound_module_busy = sound_module_busy_;
    msg.ui_board_available = ui_board_available_;
    msg.mow_enabled = mow_enabled_;

    // ArduPilot armed state is not a mower-controller health report.
    msg.mower_status = mowgli_interfaces::msg::Status::MOWER_STATUS_INITIALIZING;

    const auto mower = esc_tracker_.project(static_cast<unsigned>(blade_esc_slot_), now().nanoseconds());
    const auto blade = blade_telemetry_from_esc(mower);
    msg.mower_esc_status = blade.status;
    msg.mower_esc_temperature = blade.temperature;
    if (blade.available)
    {
      msg.mower_esc_current = blade.current;
      msg.mower_motor_rpm = blade.rpm;
      msg.blade_status_stamp.sec = static_cast<int32_t>(blade.stamp_ns / 1000000000LL);
      msg.blade_status_stamp.nanosec =
          static_cast<uint32_t>(blade.stamp_ns % 1000000000LL);
    }
    // Configured blade role only. Wheel roles remain diagnostics-only here;
    // motor winding temperature and hardware E-stop remain unavailable.
    msg.esc_power = blade.available;
  }

  pub_status_->publish(msg);
}

void MavrosHardwareBridgeNode::publish_emergency()
{
  mowgli_interfaces::msg::Emergency msg;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    msg.stamp = now();
    msg.active_emergency = emergency_active_;
    msg.latched_emergency = emergency_latched_;
    msg.reason = emergency_reason_;
  }

  pub_emergency_->publish(msg);
}

void MavrosHardwareBridgeNode::publish_readiness()
{
  diagnostic_msgs::msg::DiagnosticArray output;
  output.header.stamp = now();
  const auto now_ns = rclcpp::Time(output.header.stamp).nanoseconds();
  Readiness readiness{};
  mowgli_interfaces::msg::Power power{};
  bool power_fresh = false;
  std::array<EscState, 3> esc{};
  std::array<int64_t, 3> esc_slots{};
  mavros_msgs::msg::State fcu{};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    readiness = readiness_.project(now_ns);
    power = last_power_;
    power_fresh = last_power_receipt_ns_ > 0 && now_ns >= last_power_receipt_ns_ &&
      now_ns - last_power_receipt_ns_ <= static_cast<int64_t>(battery_observation_timeout_s_ * 1e9);
    esc_slots = {right_esc_slot_, left_esc_slot_, blade_esc_slot_};
    for (unsigned i = 0; i < esc.size(); ++i)
    {
      esc[i] = esc_tracker_.project(static_cast<unsigned>(esc_slots[i]), now_ns);
    }
    fcu = mavros_state_;
  }
  const auto add_value = [](diagnostic_msgs::msg::DiagnosticStatus& status,
                            const char* key, const std::string& value) {
    diagnostic_msgs::msg::KeyValue item;
    item.key = key;
    item.value = value;
    status.values.push_back(std::move(item));
  };
  const auto format_value = [](double value) {
    return std::isfinite(value) ? std::to_string(value) : std::string("unavailable");
  };
  const auto add_component = [&output](const char* name, bool ok, const char* message) {
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = name;
    status.level = ok ? diagnostic_msgs::msg::DiagnosticStatus::OK :
                        diagnostic_msgs::msg::DiagnosticStatus::WARN;
    status.message = message;
    output.status.push_back(std::move(status));
  };
  add_component("mowgli_mavros_bridge/fcu_connection", readiness.connected,
                readiness.connected ? "connected" : "disconnected");
  auto& fcu_status = output.status.back();
  add_value(fcu_status, "mode", fcu.mode);
  add_value(fcu_status, "armed", fcu.armed ? "true" : "false");
  add_value(fcu_status, "firmware_compatible", "unknown");
  add_component("mowgli_mavros_bridge/imu_source", readiness.imu_fresh,
                readiness.imu_fresh ? "fresh" : "missing_or_stale");
  add_component("mowgli_mavros_bridge/gnss_source",
                readiness.gnss_fresh && readiness.gnss_valid,
                readiness.gnss_fresh && readiness.gnss_valid ? "fresh_valid_fix" :
                "missing_stale_or_invalid");
  add_value(output.status.back(), "required", gnss_required_ ? "true" : "false");
  add_component("mowgli_mavros_bridge/wheel_odometry_source", readiness.wheel_fresh,
                readiness.wheel_fresh ? "fresh" : "missing_or_stale");
  add_value(output.status.back(), "required", wheel_odometry_required_ ? "true" : "false");
  add_component("mowgli_mavros_bridge/traction_power",
                readiness.traction_fresh && readiness.traction_valid,
                readiness.traction_fresh && readiness.traction_valid ? "fresh_valid" :
                "missing_stale_or_invalid");
  diagnostic_msgs::msg::DiagnosticStatus power_status;
  power_status.name = "mowgli_mavros_bridge/power";
  power_status.level = power_fresh ? diagnostic_msgs::msg::DiagnosticStatus::OK :
      diagnostic_msgs::msg::DiagnosticStatus::STALE;
  power_status.message = power_fresh ? "fresh_canonical_power" : "missing_or_stale";
  add_value(power_status, "charge_current_a", format_value(power.charge_current));
  add_value(power_status, "battery_voltage_v", format_value(power.v_battery));
  add_value(power_status, "charger_voltage_v", format_value(power.v_charge));
  add_value(power_status, "charger_enabled", power.charger_enabled ? "true" : "false");
  add_value(power_status, "charger_status", power_fresh ? power.charger_status : "unavailable");
  output.status.push_back(std::move(power_status));

  constexpr const char* kEscNames[3] = {"right_wheel", "left_wheel", "mower"};
  for (unsigned i = 0; i < esc.size(); ++i)
  {
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = std::string("mowgli_mavros_bridge/vesc_") + kEscNames[i];
    status.level = esc[i].online ? diagnostic_msgs::msg::DiagnosticStatus::OK :
        diagnostic_msgs::msg::DiagnosticStatus::STALE;
    const bool failure = esc[i].sample.failure_flags_valid && esc[i].sample.failure_flags != 0;
    if (esc[i].online && failure) {status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;}
    status.message = esc[i].online ? (failure ? "esc_failure_flags" : "online") : (esc[i].stale ? "stale" : "absent");
    add_value(status, "esc_index", std::to_string(esc_slots[i]));
    add_value(status, "source", std::to_string(esc[i].sample.source));
    add_value(status, "online", esc[i].online ? "true" : "false");
    add_value(status, "stale", esc[i].stale ? "true" : "false");
    add_value(status, "age_ms", std::to_string(esc[i].age_ms));
    if (esc[i].observed)
    {
      add_value(status, "rpm_abs", esc[i].sample.rpm_valid ? std::to_string(
        std::abs(static_cast<int64_t>(esc[i].sample.rpm))) : "unavailable");
      add_value(status, "voltage_v", esc[i].sample.voltage_valid ? format_value(esc[i].sample.voltage) : "unavailable");
      add_value(status, "current_a", esc[i].sample.current_valid ? format_value(esc[i].sample.current) : "unavailable");
      add_value(status, "totalcurrent_ah", esc[i].sample.totalcurrent_valid ? format_value(esc[i].sample.totalcurrent) : "unavailable");
      add_value(status, "temperature_c", esc[i].sample.temperature_valid ? format_value(esc[i].sample.temperature) : "unavailable");
      add_value(status, "count", esc[i].sample.count_valid ? std::to_string(esc[i].sample.count) : "unavailable");
      add_value(status, "rpm_valid", esc[i].sample.rpm_valid ? "true" : "false");
      add_value(status, "temperature_valid", esc[i].sample.temperature_valid ? "true" : "false");
      add_value(status, "failure_flags_valid", esc[i].sample.failure_flags_valid ? "true" : "false");
      add_value(status, "failure_flags", esc[i].sample.failure_flags_valid ? std::to_string(esc[i].sample.failure_flags) : "unavailable");
      add_value(status, "error_count_valid", esc[i].sample.error_count_valid ? "true" : "false");
      add_value(status, "error_count", esc[i].sample.error_count_valid ? std::to_string(esc[i].sample.error_count) : "unavailable");
    }
    output.status.push_back(std::move(status));
  }
  add_component("mowgli_mavros_bridge/hardware_emergency_stop", false,
                "unavailable_not_installed");
  add_component("mowgli_mavros_bridge/backend_readiness", readiness.ready,
                readiness.ready ? "ready" : "not_ready");
  pub_readiness_->publish(output);
}

bool MavrosHardwareBridgeNode::send_arm_command(bool arm)
{
  const auto capabilities = firmware_provider_->capabilities();
  if (!(arm ? capabilities.arm_implemented : capabilities.disarm_implemented))
  {
    return false;
  }
  if (!cli_arm_)
  {
    RCLCPP_ERROR(get_logger(), "CommandBool client is null");
    return false;
  }

  if (!cli_arm_->wait_for_service(std::chrono::seconds(1)))
  {
    RCLCPP_ERROR(get_logger(), "Service /mavros/cmd/arming not available");
    return false;
  }

  auto request = std::make_shared<mavros_msgs::srv::CommandBool::Request>();
  request->value = arm;

  cli_arm_->async_send_request(
      request,
      [this, arm](rclcpp::Client<mavros_msgs::srv::CommandBool>::SharedFuture future)
      {
        try
        {
          const auto response = future.get();
          if (!response)
          {
            RCLCPP_ERROR(get_logger(), "Null response from /mavros/cmd/arming");
            return;
          }

          if (!response->success)
          {
            RCLCPP_ERROR(
                get_logger(),
                "Autopilot rejected arming request: %s",
                arm ? "ARM" : "DISARM");
            return;
          }

          RCLCPP_INFO(
              get_logger(),
              "Autopilot confirmed arming request: %s",
              arm ? "ARM" : "DISARM");
        }
        catch (const std::exception& e)
        {
          RCLCPP_ERROR(get_logger(), "Arm/disarm request failed asynchronously: %s", e.what());
        }
      });

  return true;
}

bool MavrosHardwareBridgeNode::send_mode_command(const std::string& mode)
{
  if (!firmware_provider_->capabilities().mode_control_implemented)
  {
    return false;
  }
  if (!cli_set_mode_)
  {
    RCLCPP_ERROR(get_logger(), "SetMode client is null");
    return false;
  }

  if (!cli_set_mode_->wait_for_service(std::chrono::seconds(1)))
  {
    RCLCPP_ERROR(get_logger(), "Service /mavros/set_mode not available");
    return false;
  }

  auto request = std::make_shared<mavros_msgs::srv::SetMode::Request>();
  request->custom_mode = mode;

  cli_set_mode_->async_send_request(
      request,
      [this, mode](rclcpp::Client<mavros_msgs::srv::SetMode>::SharedFuture future)
      {
        try
        {
          const auto response = future.get();
          if (!response)
          {
            RCLCPP_ERROR(get_logger(), "Null response from /mavros/set_mode");
            return;
          }

          if (!response->mode_sent)
          {
            RCLCPP_ERROR(
                get_logger(),
                "Autopilot rejected mode request '%s'",
                mode.c_str());
            return;
          }

          RCLCPP_INFO(get_logger(), "Autopilot confirmed mode request '%s'", mode.c_str());
        }
        catch (const std::exception& e)
        {
          RCLCPP_ERROR(get_logger(), "Set mode request failed asynchronously for '%s': %s",
                       mode.c_str(), e.what());
        }
      });

  return true;
}

}  // namespace mowgli_mavros_bridge

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<mowgli_mavros_bridge::MavrosHardwareBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
