#include "mavros_hardware_bridge_node.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "mowgli_mavros_bridge/button_change_decoder.hpp"
#include "mowgli_mavros_bridge/rover_manual_control.hpp"
#include "mowgli_mavros_bridge/vesc_telemetry_projection.hpp"
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
namespace mowgli_mavros_bridge
{

using namespace std::chrono_literals;

static int64_t blade_steady_ms()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

MavrosHardwareBridgeNode::MavrosHardwareBridgeNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("hardware_bridge", options)
{
  // Set only by the backend launch after its single firmware resolution.
  // Standalone bridge execution preserves its existing Rover conversion.
  const char* resolved_firmware = std::getenv("MAVROS_RESOLVED_FIRMWARE");
  firmware_provider_ = make_firmware_provider(resolved_firmware ? resolved_firmware : "ardupilot");

  status_publish_rate_hz_ = declare_parameter<double>("status_publish_rate_hz", 10.0);
  manual_control_linear_scale_ = declare_parameter<double>("manual_control_linear_scale", 1000.0);
  manual_control_yaw_scale_ = declare_parameter<double>("manual_control_yaw_scale", 1000.0);
  mowing_enabled_ = declare_parameter<bool>("mowing_enabled", true);
  // Fixed during a connection: never redirect an in-flight OFF to another ESC.
  rcl_interfaces::msg::ParameterDescriptor blade_parameter;
  blade_parameter.read_only = true;
  const auto blade_channel = declare_parameter<int64_t>("blade_servo_channel", 3, blade_parameter);
  const auto blade_neutral = declare_parameter<int64_t>("blade_neutral_pwm", 1500, blade_parameter);
  const auto blade_forward = declare_parameter<int64_t>("blade_forward_pwm", 1450, blade_parameter);
  const auto blade_reverse = declare_parameter<int64_t>("blade_reverse_pwm", 1550, blade_parameter);
  if (blade_channel < 1 || blade_channel > 32 || blade_neutral < 1000 || blade_neutral > 2000 ||
      blade_forward < 1000 || blade_forward > 2000 || blade_reverse < 1000 || blade_reverse > 2000)
  {
    throw std::invalid_argument("Blade channel/PWM is out of range");
  }
  BladeControl::Config blade_config;
  blade_config.channel = static_cast<int>(blade_channel);
  blade_config.neutral = static_cast<int>(blade_neutral);
  blade_config.forward = static_cast<int>(blade_forward);
  blade_config.reverse = static_cast<int>(blade_reverse);
  blade_control_ = BladeControl(blade_config);
  battery_observation_timeout_s_ = declare_parameter<double>("battery_observation_timeout_s", 5.0);
  const auto readiness_timeout_s =
      declare_parameter<double>("readiness_observation_timeout_s", 5.0);
  gnss_required_ = declare_parameter<bool>("gnss_required", true);
  wheel_odometry_required_ = declare_parameter<bool>("wheel_odometry_required", false);
  readiness_ = ReadinessState(readiness_timeout_s, gnss_required_, wheel_odometry_required_);
  esc_observation_timeout_s_ = declare_parameter<double>("esc_observation_timeout_s", 3.0);
  esc_tracker_ = EscTelemetryTracker(esc_observation_timeout_s_);
  right_esc_slot_ = declare_parameter<int64_t>("right_esc_slot", 0);
  left_esc_slot_ = declare_parameter<int64_t>("left_esc_slot", 1);
  blade_esc_slot_ = declare_parameter<int64_t>("blade_esc_slot", 2);
  auto roles_valid = [](int64_t right, int64_t left, int64_t blade)
  {
    const int64_t roles[] = {right, left, blade};
    for (int64_t role : roles)
    {
      if (role < -1 || role >= 64)
      {
        return false;
      }
    }
    return (right < 0 || left < 0 || right != left) && (right < 0 || blade < 0 || right != blade) &&
           (left < 0 || blade < 0 || left != blade);
  };
  if (!roles_valid(right_esc_slot_, left_esc_slot_, blade_esc_slot_))
  {
    throw std::invalid_argument(
        "ESC role mappings must be distinct indexes in [0,63], or -1 disabled");
  }
  tilt_emergency_ms_ = declare_parameter<int64_t>("tilt_emergency_ms", 500);
  imu_inclination_threshold_ = declare_parameter<int64_t>("imu_inclination_threshold", 56);
  parameter_validation_callback_ = add_on_set_parameters_callback(
      [this, roles_valid](const std::vector<rclcpp::Parameter>& parameters)
      {
        std::lock_guard<std::mutex> lock(mutex_);
        int64_t right = right_esc_slot_, left = left_esc_slot_, blade = blade_esc_slot_;
        rcl_interfaces::msg::SetParametersResult result;
        result.successful = false;
        try
        {
          for (const auto& parameter : parameters)
          {
            if (parameter.get_name() == "right_esc_slot")
            {
              right = parameter.as_int();
            }
            if (parameter.get_name() == "left_esc_slot")
            {
              left = parameter.as_int();
            }
            if (parameter.get_name() == "blade_esc_slot")
            {
              blade = parameter.as_int();
            }
            if (parameter.get_name() == "manual_control_linear_scale" ||
                parameter.get_name() == "manual_control_yaw_scale")
            {
              const auto value = parameter.as_double();
              if (!std::isfinite(value) || value <= 0.0)
              {
                result.reason = "MANUAL_CONTROL scales must be finite and positive";
                return result;
              }
            }
          }
          if (!roles_valid(right, left, blade))
          {
            result.reason = "invalid or overlapping ESC role mapping";
            return result;
          }
          result.successful = true;
        }
        catch (const std::exception& error)
        {
          result.reason = error.what();
        }
        return result;
      });
  parameter_apply_callback_ = add_post_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter>& parameters)
      {
        std::lock_guard<std::mutex> lock(mutex_);
        bool changed = false;
        for (const auto& p : parameters)
        {
          if (p.get_name() == "right_esc_slot")
          {
            right_esc_slot_ = p.as_int();
            changed = true;
          }
          if (p.get_name() == "left_esc_slot")
          {
            left_esc_slot_ = p.as_int();
            changed = true;
          }
          if (p.get_name() == "blade_esc_slot")
          {
            blade_esc_slot_ = p.as_int();
            changed = true;
          }
          if (p.get_name() == "manual_control_linear_scale")
          {
            manual_control_linear_scale_ = p.as_double();
          }
          if (p.get_name() == "manual_control_yaw_scale")
          {
            manual_control_yaw_scale_ = p.as_double();
          }
        }
        if (changed)
        {
          esc_tracker_.reset();
          blade_control_.force_off(blade_steady_ms());
          drive_blade_locked();
        }
      });
  rain_detected_ = declare_parameter<bool>("rain_detected_default", false);
  esc_power_ = declare_parameter<bool>("esc_power_default", false);
  raspberry_pi_power_ = declare_parameter<bool>("raspberry_pi_power_default", true);

  create_publishers();
  create_subscriptions();
  create_services();
  create_clients();
  create_timers();

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

  sub_blade_wire_ = create_subscription<mavros_msgs::msg::Mavlink>(
      "/uas1/mavlink_sink",
      sensor_qos,
      [this](mavros_msgs::msg::Mavlink::SharedPtr message)
      {
        using Command = mavlink::common::msg::COMMAND_LONG;
        if (message->framing_status != mavros_msgs::msg::Mavlink::FRAMING_OK ||
            message->msgid != Command::MSG_ID)
          return;
        mavlink::mavlink_message_t raw{};
        if (!mavros_msgs::mavlink::convert(*message, raw))
          return;
        mavlink::MsgMap map(&raw);
        Command command;
        command.deserialize(map);
        std::lock_guard<std::mutex> lock(mutex_);
        if (command.command != 183 || command.param1 != blade_control_.channel())
          return;
        const auto steady = blade_steady_ms();
        while (!blade_wire_expectations_.empty() &&
               blade_wire_expectations_.front().until_ms < steady)
          blade_wire_expectations_.pop_front();
        const auto own = std::find_if(blade_wire_expectations_.begin(),
                                      blade_wire_expectations_.end(),
                                      [&command](const auto& expected)
                                      {
                                        return expected.pwm == command.param2;
                                      });
        if (own != blade_wire_expectations_.end())
        {
          blade_wire_expectations_.erase(own);
          return;
        }
        // An unmatched outgoing blade command invalidates local ownership/cache,
        // even if its value happens to equal the cached command.
        blade_control_.force_off(steady);
        drive_blade_locked();
      });

  sub_blade_output_ = create_subscription<mavros_msgs::msg::RCOut>(
      "/mavros/rc/out",
      sensor_qos,
      [this](mavros_msgs::msg::RCOut::SharedPtr message)
      {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto channel = static_cast<size_t>(blade_control_.channel() - 1);
        if (message->header.stamp.sec < 0 || message->header.stamp.nanosec >= 1000000000U)
          return;
        const auto stamp = rclcpp::Time(message->header.stamp).nanoseconds();
        if (mavros_state_.connected && channel < message->channels.size() &&
            stamp > last_blade_output_stamp_)
        {
          last_blade_output_stamp_ = stamp;
          blade_control_.output(message->channels[channel], blade_steady_ms());
          drive_blade_locked();
        }
      });

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

  sub_mavros_sys_status_ = create_subscription<mavros_msgs::msg::SysStatus>(
      "/mavros/sys_status",
      sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_mavros_sys_status, this, std::placeholders::_1));

  // MAVROS 2.16 exposes the FCU receive stream on its UAS endpoint, /uas1.
  // It is not a private plugin topic under /mavros.
  sub_mavlink_source_ = create_subscription<mavros_msgs::msg::Mavlink>(
      "/uas1/mavlink_source",
      sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_mavlink_source, this, std::placeholders::_1));

  sub_mavros_imu_ = create_subscription<sensor_msgs::msg::Imu>(
      "/mavros/imu/data",
      sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_mavros_imu, this, std::placeholders::_1));

  sub_power_ = create_subscription<mowgli_interfaces::msg::Power>(
      "/hardware_bridge/power",
      sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_power, this, std::placeholders::_1));
  sub_esc_telemetry_ = create_subscription<mavros_esc_wheel_odometry::msg::EscObservation>(
      "/mavros/esc_wheel_odometry/esc_observation",
      rclcpp::SensorDataQoS().keep_last(64),
      std::bind(&MavrosHardwareBridgeNode::on_esc_telemetry, this, std::placeholders::_1));
  sub_gnss_status_ = create_subscription<mowgli_interfaces::msg::GnssStatus>(
      "/gps/status",
      sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_gnss_status, this, std::placeholders::_1));
  sub_wheel_odom_ = create_subscription<nav_msgs::msg::Odometry>(
      "/wheel_odom",
      sensor_qos,
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
  srv_reboot_board_ =
      create_service<std_srvs::srv::Trigger>("~/reboot_board",
                                             std::bind(&MavrosHardwareBridgeNode::on_reboot_board,
                                                       this,
                                                       std::placeholders::_1,
                                                       std::placeholders::_2));
}

void MavrosHardwareBridgeNode::create_clients()
{
  cli_arm_ = create_client<mavros_msgs::srv::CommandBool>("/mavros/cmd/arming");
  cli_set_mode_ = create_client<mavros_msgs::srv::SetMode>("/mavros/set_mode");
  cli_command_long_ = create_client<mavros_msgs::srv::CommandLong>("/mavros/cmd/command");
}

void MavrosHardwareBridgeNode::create_timers()
{
  timer_blade_ = create_wall_timer(50ms,
                                   [this]()
                                   {
                                     std::lock_guard<std::mutex> lock(mutex_);
                                     drive_blade_locked();
                                   });
  const auto period = std::chrono::duration<double>(1.0 / std::max(1.0, status_publish_rate_hz_));

  timer_status_ = create_wall_timer(std::chrono::duration_cast<std::chrono::milliseconds>(period),
                                    [this]()
                                    {
                                      publish_status();
                                      publish_emergency();
                                    });
  timer_diagnostics_ = create_wall_timer(1s,
                                         [this]()
                                         {
                                           publish_readiness();
                                         });
}

void MavrosHardwareBridgeNode::on_cmd_vel(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (!std::isfinite(msg->twist.linear.x) || !std::isfinite(msg->twist.angular.z))
  {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "Ignoring non-finite /cmd_vel command.");
    return;
  }

  if (safety_state_.project(now().nanoseconds()).active_emergency)
  {
    publish_neutral_manual_control();
    return;
  }

  auto cmd = firmware_provider_->manual_control_from_twist(*msg,
                                                           manual_control_linear_scale_,
                                                           manual_control_yaw_scale_);
  cmd.header.stamp = now();
  pub_manual_control_->publish(cmd);
}

void MavrosHardwareBridgeNode::on_high_level_status(
    const mowgli_interfaces::msg::HighLevelStatus::SharedPtr msg)
{
  bool enter_manual = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    enter_manual = high_level_motion_active(msg->state) &&
                   !high_level_motion_active(last_high_level_status_.state);
    last_high_level_status_ = *msg;
  }
  if (enter_manual)
  {
    (void)send_mode_command("MANUAL");
  }
}

void MavrosHardwareBridgeNode::on_mavros_state(const mavros_msgs::msg::State::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (mavros_state_.connected != msg->connected)
  {
    last_blade_output_stamp_ = 0;
    blade_wire_expectations_.clear();
  }
  blade_control_.connection(msg->connected, blade_steady_ms());
  last_fcu_receipt_ms_ = blade_steady_ms();
  if (mavros_state_.connected && !msg->connected)
  {
    last_power_receipt_ns_ = 0;
    last_power_ = mowgli_interfaces::msg::Power{};
    esc_tracker_.reset();
    is_charging_ = false;
    charger_enabled_ = false;
    charger_status_ = "unknown";
  }
  if (!msg->connected)
  {
    safety_state_.disconnect();
  }
  readiness_.connection(msg->connected);
  mavros_state_ = *msg;
  drive_blade_locked();
}

void MavrosHardwareBridgeNode::on_mavros_sys_status(
    const mavros_msgs::msg::SysStatus::SharedPtr msg)
{
  constexpr uint32_t kMotorOutputs =
      static_cast<uint32_t>(mavlink::common::MAV_SYS_STATUS_SENSOR::MOTOR_OUTPUTS);
  bool safety_engaged = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto previous = safety_state_.hardware_safety_state();
    if (mavros_state_.connected)
    {
      last_hardware_safety_ms_ = blade_steady_ms();
      safety_state_.observe_motor_outputs((msg->sensors_present & kMotorOutputs) != 0U,
                                          (msg->sensors_enabled & kMotorOutputs) != 0U);
      safety_engaged = previous != HardwareSafetyState::Engaged &&
                       safety_state_.hardware_safety_state() == HardwareSafetyState::Engaged;
    }
  }
  if (safety_engaged)
  {
    (void)request_hold_and_blade_disarm();
  }
}

void MavrosHardwareBridgeNode::on_mavlink_source(const mavros_msgs::msg::Mavlink::SharedPtr msg)
{
  const auto button_state = decode_button_change(*msg);
  if (!button_state)
  {
    return;
  }

  const auto receipt_ns = now().nanoseconds();
  bool disarm_blade = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mavros_state_.connected)
    {
      safety_state_.observe_button_change(*button_state, receipt_ns);
      const bool double_lift =
          safety_state_.left_wheel_lifted() && safety_state_.right_wheel_lifted();
      disarm_blade = double_lift && !double_lift_disarm_sent_;
      double_lift_disarm_sent_ = double_lift;
    }
  }
  if (disarm_blade)
  {
    (void)request_blade_disarm();
  }
}

void MavrosHardwareBridgeNode::on_mavros_imu(const sensor_msgs::msg::Imu::SharedPtr msg)
{
  bool disarm_blade = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    last_imu_ = *msg;
    readiness_.imu(now().nanoseconds());
    const auto& q = msg->orientation;
    const double norm = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    bool tilted = false;
    if (std::isfinite(norm) && norm > 1.0e-9)
    {
      const double x = q.x / norm;
      const double y = q.y / norm;
      const double z_axis_up = std::clamp(1.0 - 2.0 * (x * x + y * y), -1.0, 1.0);
      const double inclination = std::acos(z_axis_up);
      const double threshold_g =
          std::clamp(static_cast<double>(imu_inclination_threshold_) * 0.016, 0.0, 1.0);
      tilted = inclination >= std::asin(threshold_g);
    }
    const int64_t receipt_ns = now().nanoseconds();
    if (tilted)
    {
      if (tilt_started_ns_ == 0)
      {
        tilt_started_ns_ = receipt_ns;
      }
      const bool active = receipt_ns - tilt_started_ns_ >= tilt_emergency_ms_ * 1000000LL;
      safety_state_.observe_tilt(active);
      disarm_blade = active && !tilt_disarm_sent_;
      tilt_disarm_sent_ = active;
    }
    else
    {
      tilt_started_ns_ = 0;
      tilt_disarm_sent_ = false;
      safety_state_.observe_tilt(false);
    }
  }
  if (disarm_blade)
  {
    (void)request_blade_disarm();
  }
  pub_imu_->publish(*msg);
}

void MavrosHardwareBridgeNode::on_power(const mowgli_interfaces::msg::Power::SharedPtr msg)
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
  if (mavros_state_.connected)
  {
    esc_tracker_.observe(*msg, receipt_ns);
    if (blade_esc_slot_ >= 0 && msg->esc_index == blade_esc_slot_)
    {
      const auto state = esc_tracker_.project(msg->esc_index, receipt_ns);
      blade_control_.sample(state.sample.source,
                            state.last_update_ns,
                            state.sample.count,
                            state.sample.count_valid,
                            state.online && state.sample.rpm_valid && state.age_ms >= 0 &&
                                state.age_ms <= 1000,
                            state.sample.rpm,
                            blade_steady_ms());
      drive_blade_locked();
    }
  }
}

void MavrosHardwareBridgeNode::on_gnss_status(
    const mowgli_interfaces::msg::GnssStatus::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(mutex_);
  readiness_.gnss(now().nanoseconds(),
                  msg->position_observation_sequence,
                  msg->backend,
                  msg->fix_valid);
}

void MavrosHardwareBridgeNode::on_wheel_odom(const nav_msgs::msg::Odometry::SharedPtr /*msg*/)
{
  std::lock_guard<std::mutex> lock(mutex_);
  readiness_.wheel(now().nanoseconds());
}

void MavrosHardwareBridgeNode::on_mower_control(
    const std::shared_ptr<rmw_request_id_t> header,
    const std::shared_ptr<mowgli_interfaces::srv::MowerControl::Request> request)
{
  std::lock_guard<std::mutex> lock(mutex_);
  drive_blade_locked();
  if (request->mow_enabled > 1 || (request->mow_enabled && request->mow_direction > 1))
  {
    reply_blade(header, false);
    return;
  }
  const auto direction = !request->mow_enabled         ? BladeControl::Direction::Off
                         : request->mow_direction == 0 ? BladeControl::Direction::Forward
                                                       : BladeControl::Direction::Reverse;
  const bool transport_available = std::string(firmware_provider_->name()) == "ardupilot" &&
                                   cli_command_long_->service_is_ready();
  if (!transport_available && direction != BladeControl::Direction::Off)
  {
    reply_blade(header, false);
    return;
  }
  // OFF cancels a deferred ON even while the transport is absent. A queued
  // neutral is not an ACK: fail this caller, but retain the OFF intent so that
  // service recovery can never launch the superseded ON.
  const auto result = blade_control_.request(direction, blade_steady_ms());
  if (!transport_available)
  {
    drive_blade_locked();
    reply_blade(header, false);
    return;
  }
  if (result != BladeControl::Result::Pending)
  {
    reply_blade(header, result == BladeControl::Result::Confirmed);
    return;
  }
  if (blade_replies_.size() >= 32)
  {
    // Never prevent OFF from preempting ON because the response queue is full.
    drive_blade_locked();
    reply_blade(header, false);
    return;
  }
  blade_replies_.push_back({header, blade_control_.revision(), blade_steady_ms()});
  drive_blade_locked();
}

bool MavrosHardwareBridgeNode::request_blade_neutral()
{
  std::lock_guard<std::mutex> lock(mutex_);
  blade_control_.force_off(blade_steady_ms());
  drive_blade_locked();
  return std::string(firmware_provider_->name()) == "ardupilot" &&
         cli_command_long_->service_is_ready();
}

void MavrosHardwareBridgeNode::reply_blade(const std::shared_ptr<rmw_request_id_t>& header,
                                           bool success)
{
  mowgli_interfaces::srv::MowerControl::Response response;
  response.success = success;
  try
  {
    srv_mower_control_->send_response(*header, response);
  }
  catch (const std::exception& error)
  {
    RCLCPP_WARN(get_logger(), "Blade reply failed: %s", error.what());
  }
}

void MavrosHardwareBridgeNode::drive_blade_locked()
{
  const auto steady = blade_steady_ms();
  while (!blade_wire_expectations_.empty() && blade_wire_expectations_.front().until_ms < steady)
    blade_wire_expectations_.pop_front();
  const auto safety = safety_state_.project(now().nanoseconds());
  const bool allowed = mowing_enabled_ && mavros_state_.connected && mavros_state_.armed &&
                       last_fcu_receipt_ms_ >= 0 && steady - last_fcu_receipt_ms_ <= 2500 &&
                       last_hardware_safety_ms_ >= 0 && steady - last_hardware_safety_ms_ <= 3000 &&
                       safety_state_.hardware_safety_state() == HardwareSafetyState::Released &&
                       !safety.active_emergency && !safety.latched_emergency &&
                       blade_esc_slot_ >= 0;
  blade_control_.permission(allowed, steady);
  blade_control_.tick(steady);
  if (!blade_control_.satisfied() && std::any_of(blade_replies_.begin(),
                                                 blade_replies_.end(),
                                                 [this, steady](const auto& reply)
                                                 {
                                                   return reply.revision ==
                                                              blade_control_.revision() &&
                                                          steady - reply.started_ms > 20000;
                                                 }))
  {
    // A failed service deadline must not leave a delayed ON intent alive.
    blade_control_.service_timeout(steady);
  }
  for (auto it = blade_rpc_ids_.begin(); it != blade_rpc_ids_.end();)
  {
    if (!blade_control_.current(it->first))
    {
      cli_command_long_->remove_pending_request(it->second);
      it = blade_rpc_ids_.erase(it);
    }
    else
      ++it;
  }
  for (auto it = blade_replies_.begin(); it != blade_replies_.end();)
  {
    const bool invalid = it->revision != blade_control_.revision() || blade_control_.failed() ||
                         steady - it->started_ms > 20000;
    if (invalid || blade_control_.satisfied())
    {
      reply_blade(it->header, !invalid);
      it = blade_replies_.erase(it);
    }
    else
      ++it;
  }
  if (std::string(firmware_provider_->name()) != "ardupilot" ||
      !cli_command_long_->service_is_ready())
  {
    return;
  }
  const auto command = blade_control_.next(steady);
  if (!command)
    return;
  auto request = std::make_shared<mavros_msgs::srv::CommandLong::Request>();
  request->broadcast = false;
  request->command = 183;
  request->param1 = static_cast<float>(command->channel);
  request->param2 = static_cast<float>(command->pwm);
  try
  {
    blade_wire_expectations_.push_back({command->pwm, steady + 2000});
    const auto future_request = cli_command_long_->async_send_request(
        request,
        [this,
         token = command->token](rclcpp::Client<mavros_msgs::srv::CommandLong>::SharedFuture future)
        {
          bool accepted = false;
          try
          {
            const auto response = future.get();
            accepted = response && response->success && response->result == 0;
          }
          catch (const std::exception& error)
          {
            RCLCPP_ERROR(get_logger(), "Blade ACK failed: %s", error.what());
          }
          std::lock_guard<std::mutex> callback_lock(mutex_);
          blade_rpc_ids_.erase(token);
          blade_control_.complete(token, accepted, blade_steady_ms());
          drive_blade_locked();
        });
    blade_rpc_ids_[command->token] = future_request.request_id;
  }
  catch (const std::exception& error)
  {
    if (!blade_wire_expectations_.empty())
      blade_wire_expectations_.pop_back();
    blade_control_.complete(command->token, false, blade_steady_ms());
    RCLCPP_ERROR(get_logger(), "Blade send failed: %s", error.what());
  }
}

void MavrosHardwareBridgeNode::on_emergency_stop(
    const std::shared_ptr<mowgli_interfaces::srv::EmergencyStop::Request> request,
    std::shared_ptr<mowgli_interfaces::srv::EmergencyStop::Response> response)
{
  const bool emergency_requested = (request->emergency != 0U);

  {
    std::lock_guard<std::mutex> lock(mutex_);

    safety_state_.set_service_emergency(emergency_requested);
    if (emergency_requested)
    {
      mow_enabled_ = false;
    }
  }

  if (!emergency_requested)
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      safety_state_.clear_released_latches();
    }
    if (high_level_motion_active(last_high_level_status_.state))
    {
      (void)send_mode_command("MANUAL");
    }
    response->success = true;
    return;
  }

  // This service reports whether the emergency request was accepted locally
  // and forwarded to MAVROS. It does not imply the autopilot has already
  // confirmed or completed the requested state change.
  response->success = request_hold_and_blade_disarm();
}

void MavrosHardwareBridgeNode::on_reboot_board(
    const std::shared_ptr<std_srvs::srv::Trigger::Request>,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  if (!cli_command_long_->wait_for_service(std::chrono::seconds(1)))
  {
    response->success = false;
    response->message = "Service /mavros/cmd/command not available";
    return;
  }
  auto request = std::make_shared<mavros_msgs::srv::CommandLong::Request>();
  request->broadcast = false;
  request->command = 246U;  // MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN
  request->confirmation = 0U;
  request->param1 = 1.0F;  // reboot autopilot
  cli_command_long_->async_send_request(request);
  response->success = true;
  response->message = "Pixhawk reboot request queued";
}

void MavrosHardwareBridgeNode::publish_status()
{
  mowgli_interfaces::msg::Status msg;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto status_stamp_ns = readiness_.status_stamp_ns();
    if (status_stamp_ns > 0)
    {
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
    msg.mow_enabled = blade_control_.enabled();
    msg.blade_requested_direction = BladeControl::label(blade_control_.requested());

    // ArduPilot armed state is not a mower-controller health report.
    msg.mower_status = mowgli_interfaces::msg::Status::MOWER_STATUS_INITIALIZING;

    const auto mower =
        esc_tracker_.project(static_cast<unsigned>(blade_esc_slot_), now().nanoseconds());
    const auto blade = blade_telemetry_from_esc(mower);
    msg.mower_esc_status = blade.status;
    msg.mower_esc_temperature = blade.temperature;
    if (blade.available)
    {
      msg.mower_esc_current = blade.current;
      msg.mower_motor_rpm = blade.rpm;
      msg.blade_status_stamp.sec = static_cast<int32_t>(blade.stamp_ns / 1000000000LL);
      msg.blade_status_stamp.nanosec = static_cast<uint32_t>(blade.stamp_ns % 1000000000LL);
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
    const auto emergency = safety_state_.project(rclcpp::Time(msg.stamp).nanoseconds());
    msg.active_emergency = emergency.active_emergency;
    msg.latched_emergency = emergency.latched_emergency;
    msg.lift_warning = emergency.lift_warning;
    msg.lift_duration_sec = emergency.lift_duration_sec;
    msg.reason = emergency.reason;
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
  SafetyState safety{};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    readiness = readiness_.project(now_ns);
    power = last_power_;
    power_fresh = last_power_receipt_ns_ > 0 && now_ns >= last_power_receipt_ns_ &&
                  now_ns - last_power_receipt_ns_ <=
                      static_cast<int64_t>(battery_observation_timeout_s_ * 1e9);
    esc_slots = {right_esc_slot_, left_esc_slot_, blade_esc_slot_};
    for (unsigned i = 0; i < esc.size(); ++i)
    {
      esc[i] = esc_tracker_.project(static_cast<unsigned>(esc_slots[i]), now_ns);
    }
    fcu = mavros_state_;
    safety = safety_state_;
  }
  const auto add_value =
      [](diagnostic_msgs::msg::DiagnosticStatus& status, const char* key, const std::string& value)
  {
    diagnostic_msgs::msg::KeyValue item;
    item.key = key;
    item.value = value;
    status.values.push_back(std::move(item));
  };
  const auto format_value = [](double value)
  {
    return std::isfinite(value) ? std::to_string(value) : std::string("unavailable");
  };
  const auto add_component = [&output](const char* name, bool ok, const char* message)
  {
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = name;
    status.level = ok ? diagnostic_msgs::msg::DiagnosticStatus::OK
                      : diagnostic_msgs::msg::DiagnosticStatus::WARN;
    status.message = message;
    output.status.push_back(std::move(status));
  };
  add_component("mowgli_mavros_bridge/fcu_connection",
                readiness.connected,
                readiness.connected ? "connected" : "disconnected");
  auto& fcu_status = output.status.back();
  add_value(fcu_status, "mode", fcu.mode);
  add_value(fcu_status, "armed", fcu.armed ? "true" : "false");
  add_value(fcu_status, "firmware_compatible", "unknown");
  add_component("mowgli_mavros_bridge/imu_source",
                readiness.imu_fresh,
                readiness.imu_fresh ? "fresh" : "missing_or_stale");
  add_component("mowgli_mavros_bridge/gnss_source",
                readiness.gnss_fresh && readiness.gnss_valid,
                readiness.gnss_fresh && readiness.gnss_valid ? "fresh_valid_fix"
                                                             : "missing_stale_or_invalid");
  add_value(output.status.back(), "required", gnss_required_ ? "true" : "false");
  add_component("mowgli_mavros_bridge/wheel_odometry_source",
                readiness.wheel_fresh,
                readiness.wheel_fresh ? "fresh" : "missing_or_stale");
  add_value(output.status.back(), "required", wheel_odometry_required_ ? "true" : "false");
  add_component("mowgli_mavros_bridge/traction_power",
                readiness.traction_fresh && readiness.traction_valid,
                readiness.traction_fresh && readiness.traction_valid ? "fresh_valid"
                                                                     : "missing_stale_or_invalid");
  diagnostic_msgs::msg::DiagnosticStatus power_status;
  power_status.name = "mowgli_mavros_bridge/power";
  power_status.level = power_fresh ? diagnostic_msgs::msg::DiagnosticStatus::OK
                                   : diagnostic_msgs::msg::DiagnosticStatus::STALE;
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
    status.level = esc[i].online ? diagnostic_msgs::msg::DiagnosticStatus::OK
                                 : diagnostic_msgs::msg::DiagnosticStatus::STALE;
    const bool failure = esc[i].sample.failure_flags_valid && esc[i].sample.failure_flags != 0;
    if (esc[i].online && failure)
    {
      status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    }
    status.message = esc[i].online ? (failure ? "esc_failure_flags" : "online")
                                   : (esc[i].stale ? "stale" : "absent");
    add_value(status, "esc_index", std::to_string(esc_slots[i]));
    add_value(status, "source", std::to_string(esc[i].sample.source));
    add_value(status, "online", esc[i].online ? "true" : "false");
    add_value(status, "stale", esc[i].stale ? "true" : "false");
    add_value(status, "age_ms", std::to_string(esc[i].age_ms));
    if (esc[i].observed)
    {
      add_value(status,
                "rpm_abs",
                esc[i].sample.rpm_valid
                    ? std::to_string(std::abs(static_cast<int64_t>(esc[i].sample.rpm)))
                    : "unavailable");
      add_value(status,
                "voltage_v",
                esc[i].sample.voltage_valid ? format_value(esc[i].sample.voltage) : "unavailable");
      add_value(status,
                "current_a",
                esc[i].sample.current_valid ? format_value(esc[i].sample.current) : "unavailable");
      add_value(status,
                "totalcurrent_ah",
                esc[i].sample.totalcurrent_valid ? format_value(esc[i].sample.totalcurrent)
                                                 : "unavailable");
      add_value(status,
                "temperature_c",
                esc[i].sample.temperature_valid ? format_value(esc[i].sample.temperature)
                                                : "unavailable");
      add_value(status,
                "count",
                esc[i].sample.count_valid ? std::to_string(esc[i].sample.count) : "unavailable");
      add_value(status, "rpm_valid", esc[i].sample.rpm_valid ? "true" : "false");
      add_value(status, "temperature_valid", esc[i].sample.temperature_valid ? "true" : "false");
      add_value(status,
                "failure_flags_valid",
                esc[i].sample.failure_flags_valid ? "true" : "false");
      add_value(status,
                "failure_flags",
                esc[i].sample.failure_flags_valid ? std::to_string(esc[i].sample.failure_flags)
                                                  : "unavailable");
      add_value(status, "error_count_valid", esc[i].sample.error_count_valid ? "true" : "false");
      add_value(status,
                "error_count",
                esc[i].sample.error_count_valid ? std::to_string(esc[i].sample.error_count)
                                                : "unavailable");
    }
    output.status.push_back(std::move(status));
  }
  diagnostic_msgs::msg::DiagnosticStatus hardware_safety;
  hardware_safety.name = "mowgli_mavros_bridge/hardware_emergency_stop";
  switch (safety.hardware_safety_state())
  {
    case HardwareSafetyState::Engaged:
      hardware_safety.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
      hardware_safety.message = "engaged";
      break;
    case HardwareSafetyState::Released:
      hardware_safety.level = diagnostic_msgs::msg::DiagnosticStatus::OK;
      hardware_safety.message = "released";
      break;
    case HardwareSafetyState::Unknown:
      hardware_safety.level = diagnostic_msgs::msg::DiagnosticStatus::STALE;
      hardware_safety.message = "unknown";
      break;
  }
  output.status.push_back(std::move(hardware_safety));

  diagnostic_msgs::msg::DiagnosticStatus wheel_lift;
  wheel_lift.name = "mowgli_mavros_bridge/wheel_lift";
  if (!safety.wheel_lift_state_valid())
  {
    wheel_lift.level = diagnostic_msgs::msg::DiagnosticStatus::STALE;
    wheel_lift.message = "unknown";
  }
  else if (safety.left_wheel_lifted() && safety.right_wheel_lifted())
  {
    wheel_lift.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    wheel_lift.message = "both_wheels_lifted";
  }
  else if (safety.left_wheel_lifted() || safety.right_wheel_lifted())
  {
    wheel_lift.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    wheel_lift.message = "one_wheel_lifted";
  }
  else
  {
    wheel_lift.level = diagnostic_msgs::msg::DiagnosticStatus::OK;
    wheel_lift.message = "no_wheel_lifted";
  }
  add_value(wheel_lift, "state_valid", safety.wheel_lift_state_valid() ? "true" : "false");
  add_value(wheel_lift,
            "left_lifted",
            safety.wheel_lift_state_valid() ? (safety.left_wheel_lifted() ? "true" : "false")
                                            : "unknown");
  add_value(wheel_lift,
            "right_lifted",
            safety.wheel_lift_state_valid() ? (safety.right_wheel_lifted() ? "true" : "false")
                                            : "unknown");
  add_value(wheel_lift, "safety_enabled", "true");
  add_value(wheel_lift,
            "raw_button_state",
            safety.wheel_lift_state_valid() ? std::to_string(safety.raw_button_state())
                                            : "unknown");
  output.status.push_back(std::move(wheel_lift));
  add_component("mowgli_mavros_bridge/backend_readiness",
                readiness.ready,
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
            RCLCPP_ERROR(get_logger(),
                         "Autopilot rejected arming request: %s",
                         arm ? "ARM" : "DISARM");
            return;
          }

          RCLCPP_INFO(get_logger(),
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
            RCLCPP_ERROR(get_logger(), "Autopilot rejected mode request '%s'", mode.c_str());
            return;
          }

          RCLCPP_INFO(get_logger(), "Autopilot confirmed mode request '%s'", mode.c_str());
        }
        catch (const std::exception& e)
        {
          RCLCPP_ERROR(get_logger(),
                       "Set mode request failed asynchronously for '%s': %s",
                       mode.c_str(),
                       e.what());
        }
      });

  return true;
}

void MavrosHardwareBridgeNode::publish_neutral_manual_control()
{
  mavros_msgs::msg::ManualControl command;
  command.header.stamp = now();
  pub_manual_control_->publish(command);
}

bool MavrosHardwareBridgeNode::request_blade_disarm()
{
  (void)request_blade_neutral();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    mow_enabled_ = false;
  }
  return send_arm_command(false);
}

bool MavrosHardwareBridgeNode::request_hold_and_blade_disarm()
{
  (void)request_blade_neutral();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    mow_enabled_ = false;
    publish_neutral_manual_control();
  }
  const bool mode_forwarded = send_mode_command("HOLD");
  const bool blade_forwarded = send_arm_command(false);
  return mode_forwarded && blade_forwarded;
}

bool MavrosHardwareBridgeNode::high_level_motion_active(uint8_t state) const
{
  using Status = mowgli_interfaces::msg::HighLevelStatus;
  return state == Status::HIGH_LEVEL_STATE_AUTONOMOUS ||
         state == Status::HIGH_LEVEL_STATE_RECORDING ||
         state == Status::HIGH_LEVEL_STATE_MANUAL_MOWING;
}

}  // namespace mowgli_mavros_bridge

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<mowgli_mavros_bridge::MavrosHardwareBridgeNode>());
  rclcpp::shutdown();
  return 0;
}
