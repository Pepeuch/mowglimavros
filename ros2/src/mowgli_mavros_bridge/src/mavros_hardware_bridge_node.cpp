#include "mavros_hardware_bridge_node.hpp"
#include "mowgli_mavros_bridge/rover_manual_control.hpp"
#include "mowgli_mavros_bridge/serial_gps_projection.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
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
  status_publish_rate_hz_ = declare_parameter<double>("status_publish_rate_hz", 10.0);
  manual_control_enabled_ = declare_parameter<bool>("manual_control_enabled", false);
  neutral_manual_control_enabled_ =
      declare_parameter<bool>("neutral_manual_control_enabled", false);
  manual_control_linear_scale_ = declare_parameter<double>("manual_control_linear_scale", 1000.0);
  manual_control_yaw_scale_ = declare_parameter<double>("manual_control_yaw_scale", 1000.0);
  blade_control_enabled_ = declare_parameter<bool>("blade_control_enabled", false);
  const auto dock_battery_instance = declare_parameter<int>("dock_battery_instance", -1);
  const auto traction_battery_instance = declare_parameter<int>("traction_battery_instance", -1);
  battery_observation_timeout_s_ = declare_parameter<double>("battery_observation_timeout_s", 5.0);
  power_mapping_ = PowerMapping(dock_battery_instance, traction_battery_instance, battery_observation_timeout_s_);
  const auto readiness_timeout_s = declare_parameter<double>("readiness_observation_timeout_s", 5.0);
  gnss_required_ = declare_parameter<bool>("gnss_required", true);
  wheel_odometry_required_ = declare_parameter<bool>("wheel_odometry_required", false);
  gps1_canonical_enabled_ = declare_parameter<bool>("gps1_canonical_enabled", false);
  readiness_ = ReadinessState(readiness_timeout_s, gnss_required_, wheel_odometry_required_);
  esc_observation_timeout_s_ = declare_parameter<double>("esc_observation_timeout_s", 3.0);
  esc_tracker_ = EscTelemetryTracker(esc_observation_timeout_s_);
  emergency_mode_ = declare_parameter<std::string>("emergency_mode", "HOLD");
  emergency_disarm_ = declare_parameter<bool>("emergency_disarm", true);
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
  if (!power_mapping_.valid())
  {
    RCLCPP_WARN(
        get_logger(),
        "Power mapping disabled: configure distinct dock_battery_instance and traction_battery_instance (0..255).");
  }

  RCLCPP_INFO(get_logger(), "MAVROS hardware bridge started.");
}

void MavrosHardwareBridgeNode::create_publishers()
{
  pub_status_ = create_publisher<mowgli_interfaces::msg::Status>("~/status", 10);
  if (gps1_canonical_enabled_)
  {
    pub_gps_fix_ = create_publisher<sensor_msgs::msg::NavSatFix>("/gps/fix", 10);
    pub_gps_status_ = create_publisher<mowgli_interfaces::msg::GnssStatus>("/gps/status", 10);
  }
  pub_emergency_ = create_publisher<mowgli_interfaces::msg::Emergency>("~/emergency", 10);
  pub_power_ = create_publisher<mowgli_interfaces::msg::Power>("~/power", 10);
  pub_imu_ = create_publisher<sensor_msgs::msg::Imu>("~/imu/data_raw", 10);
  pub_battery_state_ =
      create_publisher<sensor_msgs::msg::BatteryState>("/battery_state", 10);

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

  sub_battery_status_ = create_subscription<mavros_battery_observer::msg::BatteryStatus>(
      "/mavros/battery_observer/status", sensor_qos,
      std::bind(&MavrosHardwareBridgeNode::on_battery_status, this, std::placeholders::_1));
  if (gps1_canonical_enabled_)
  {
    sub_serial_gps_raw_ = create_subscription<mavros_msgs::msg::GPSRAW>(
        "/mavros/gpsstatus/gps1/raw", default_qos,
        std::bind(&MavrosHardwareBridgeNode::on_serial_gps_raw, this, std::placeholders::_1));
  }
  sub_esc_telemetry_ = create_subscription<mavros_msgs::msg::ESCTelemetry>(
      "/mavros/esc_telemetry/telemetry", sensor_qos,
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
    publish_gps_stale();
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
  auto cmd = rover_manual_control_from_twist(*msg, manual_control_linear_scale_, manual_control_yaw_scale_);
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
    power_mapping_.reset();
    esc_tracker_.reset();
    is_charging_ = false;
    charger_enabled_ = false;
    charger_status_ = "unknown";
  }
  if (!mavros_state_.connected && msg->connected)
  {
    gps_observation_sequence_ = 0;
    gps_last_receipt_ns_ = 0;
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

void MavrosHardwareBridgeNode::on_battery_status(
    const mavros_battery_observer::msg::BatteryStatus::SharedPtr msg)
{
  const auto stamp_ns = now().nanoseconds();
  const BatteryInput input{static_cast<int>(msg->id),
                           msg->voltage_available ? std::optional<double>(msg->voltage) : std::nullopt,
                           msg->current_available ? std::optional<double>(msg->current) : std::nullopt,
                           msg->percentage_available ? std::optional<double>(msg->percentage) : std::nullopt,
                           msg->charge_state, stamp_ns};
  PowerProjection projection{};
  bool traction_observation = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!power_mapping_.valid()) return;
    traction_observation = input.instance == get_parameter("traction_battery_instance").as_int();
    power_mapping_.observe(input);
    if (traction_observation) readiness_.traction(stamp_ns, input.voltage.has_value());
    projection = power_mapping_.project(stamp_ns);
    is_charging_ = projection.charger_enabled;
    charger_enabled_ = projection.charger_enabled;
    charger_status_ = !projection.dock_fresh ? "unavailable" :
        (projection.charger_enabled ? "charging" : "unknown");
  }
  if (traction_observation) {
    sensor_msgs::msg::BatteryState battery;
    battery.header = msg->header;
    battery.location = "id" + std::to_string(msg->id);
    battery.voltage = projection.v_battery;
    // ArduPilot BATTERY_STATUS current is positive discharge; canonical
    // Mowgli current is positive into, negative out of the mower battery.
    battery.current = projection.traction_current;
    battery.percentage = input.percentage.value_or(NAN);
    battery.present = projection.traction_fresh;
    pub_battery_state_->publish(battery);
  }
  mowgli_interfaces::msg::Power power;
  power.stamp = msg->header.stamp;
  power.v_charge = projection.v_charge;
  power.v_battery = projection.v_battery;
  power.charge_current = projection.charge_current;
  power.charger_enabled = projection.charger_enabled;
  power.charger_status = charger_status_;
  pub_power_->publish(power);
}

void MavrosHardwareBridgeNode::on_serial_gps_raw(
    const mavros_msgs::msg::GPSRAW::SharedPtr msg)
{
  if (!gps1_canonical_enabled_)
  {
    return;
  }
  const auto receipt = now();
  const auto projected = project_serial_gps(
      SerialGpsRaw{msg->fix_type, msg->lat, msg->lon, msg->alt, msg->alt_ellipsoid,
                   msg->eph, msg->satellites_visible, msg->h_acc, msg->v_acc});
  mowgli_interfaces::msg::GnssStatus status;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!mavros_state_.connected)
    {
      return;
    }
    gps_last_receipt_ns_ = receipt.nanoseconds();
    ++gps_observation_sequence_;
    status.position_observation_sequence = gps_observation_sequence_;
  }
  status.header.stamp = receipt;
  status.header.frame_id = "gps_link";
  status.backend = "mavros_serial_gps1";
  status.fix_valid = projected.has_value();
  status.fix_type = projected ? mowgli_interfaces::msg::GnssStatus::FIX_TYPE_GPS_FIX :
      mowgli_interfaces::msg::GnssStatus::FIX_TYPE_NO_FIX;
  status.rtk_mode = mowgli_interfaces::msg::GnssStatus::RTK_MODE_NONE;
  status.quality_percent = projected ? 40.0F : 0.0F;
  status.capability_flags = mowgli_interfaces::msg::GnssStatus::CAP_HDOP |
      mowgli_interfaces::msg::GnssStatus::CAP_HORIZONTAL_ACCURACY |
      mowgli_interfaces::msg::GnssStatus::CAP_VERTICAL_ACCURACY |
      mowgli_interfaces::msg::GnssStatus::CAP_SATELLITES_VISIBLE;
  if (projected)
  {
    status.satellites_visible = projected->satellites_visible;
    status.value_flags |= mowgli_interfaces::msg::GnssStatus::CAP_SATELLITES_VISIBLE;
    if (projected->hdop)
    {
      status.hdop = static_cast<float>(*projected->hdop);
      status.value_flags |= mowgli_interfaces::msg::GnssStatus::CAP_HDOP;
    }
    if (projected->horizontal_accuracy_m)
    {
      status.horizontal_accuracy_m = static_cast<float>(*projected->horizontal_accuracy_m);
      status.value_flags |= mowgli_interfaces::msg::GnssStatus::CAP_HORIZONTAL_ACCURACY;
    }
    if (projected->vertical_accuracy_m)
    {
      status.vertical_accuracy_m = static_cast<float>(*projected->vertical_accuracy_m);
      status.value_flags |= mowgli_interfaces::msg::GnssStatus::CAP_VERTICAL_ACCURACY;
    }
    sensor_msgs::msg::NavSatFix fix;
    fix.header = status.header;
    fix.status.status = sensor_msgs::msg::NavSatStatus::STATUS_FIX;
    fix.status.service = sensor_msgs::msg::NavSatStatus::SERVICE_GPS;
    fix.latitude = projected->latitude_deg;
    fix.longitude = projected->longitude_deg;
    fix.altitude = projected->altitude_m;
    if (projected->horizontal_accuracy_m && projected->vertical_accuracy_m)
    {
      const auto h = *projected->horizontal_accuracy_m;
      const auto v = *projected->vertical_accuracy_m;
      fix.position_covariance[0] = h * h;
      fix.position_covariance[4] = h * h;
      fix.position_covariance[8] = v * v;
      fix.position_covariance_type = sensor_msgs::msg::NavSatFix::COVARIANCE_TYPE_DIAGONAL_KNOWN;
    }
    pub_gps_fix_->publish(fix);
  }
  pub_gps_status_->publish(status);
}

void MavrosHardwareBridgeNode::publish_gps_stale()
{
  if (!gps1_canonical_enabled_)
  {
    return;
  }
  mowgli_interfaces::msg::GnssStatus status;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gps_last_receipt_ns_ == 0 ||
        now().nanoseconds() - gps_last_receipt_ns_ <= 3000000000LL)
    {
      return;
    }
    status.position_observation_sequence = gps_observation_sequence_;
  }
  // A repeated no-fix status informs the GUI; the unchanged observation
  // sequence cannot refresh backend readiness or create a fake position.
  status.header.stamp = now();
  status.header.frame_id = "gps_link";
  status.backend = "mavros_serial_gps1";
  status.fix_valid = false;
  status.fix_type = mowgli_interfaces::msg::GnssStatus::FIX_TYPE_NO_FIX;
  status.rtk_mode = mowgli_interfaces::msg::GnssStatus::RTK_MODE_NONE;
  pub_gps_status_->publish(status);
}

void MavrosHardwareBridgeNode::on_esc_telemetry(
    const mavros_msgs::msg::ESCTelemetry::SharedPtr msg)
{
  const auto receipt_ns = now().nanoseconds();
  std::lock_guard<std::mutex> lock(mutex_);
  if (!mavros_state_.connected)
  {
    return;
  }
  for (size_t index = 0; index < std::min<size_t>(3, msg->esc_telemetry.size()); ++index)
  {
    const auto& item = msg->esc_telemetry[index];
    esc_tracker_.observe(index,
                         EscSample{item.rpm, item.voltage, item.current,
                                   item.totalcurrent, item.temperature, item.count},
                         receipt_ns);
  }
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
  bool request_sent = true;

  if (!emergency_mode_.empty())
  {
    request_sent = send_mode_command(emergency_mode_) && request_sent;
  }
  if (emergency_disarm_)
  {
    request_sent = send_arm_command(false) && request_sent;
  }

  response->success = request_sent;

  if (request_sent)
  {
    RCLCPP_WARN(
        get_logger(),
        "Emergency stop request sent to MAVROS (mode='%s', disarm=%s). Autopilot confirmation will be logged asynchronously.",
        emergency_mode_.c_str(),
        emergency_disarm_ ? "true" : "false");
  }
  else
  {
    RCLCPP_ERROR(
        get_logger(),
        "Emergency stop request could not be fully sent to MAVROS (mode='%s', disarm=%s).",
        emergency_mode_.c_str(),
        emergency_disarm_ ? "true" : "false");
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

    const auto mower = esc_tracker_.project(2, now().nanoseconds());
    msg.mower_esc_status = !mower.online ? 99U : (mower.sample.rpm == 0 ? 200U : 201U);
    if (mower.online)
    {
      msg.mower_esc_temperature = mower.sample.temperature;
      msg.mower_esc_current = mower.sample.current;
      msg.mower_motor_rpm = std::abs(mower.sample.rpm);
      msg.blade_status_stamp.sec = static_cast<int32_t>(mower.last_update_ns / 1000000000LL);
      msg.blade_status_stamp.nanosec =
          static_cast<uint32_t>(mower.last_update_ns % 1000000000LL);
    }
    // Motor winding temperature and hardware E-stop are not available through
    // ESC_TELEMETRY; leave those fields unset. No automatic blade control.
    msg.esc_power = mower.online;
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

void MavrosHardwareBridgeNode::publish_power()
{
  // Power is published only by genuine BATTERY_STATUS observations.  A timer
  // must not make cached battery state appear fresh.
}

void MavrosHardwareBridgeNode::publish_readiness()
{
  diagnostic_msgs::msg::DiagnosticArray output;
  output.header.stamp = now();
  const auto now_ns = rclcpp::Time(output.header.stamp).nanoseconds();
  Readiness readiness{};
  PowerProjection power{};
  std::array<EscState, 3> esc{};
  mavros_msgs::msg::State fcu{};
  {
    std::lock_guard<std::mutex> lock(mutex_);
    readiness = readiness_.project(now_ns);
    power = power_mapping_.project(now_ns);
    for (unsigned i = 0; i < esc.size(); ++i)
    {
      esc[i] = esc_tracker_.project(i, now_ns);
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
  diagnostic_msgs::msg::DiagnosticStatus dock_status;
  dock_status.name = "mowgli_mavros_bridge/dock_power";
  dock_status.level = power.dock_fresh ? diagnostic_msgs::msg::DiagnosticStatus::OK :
      diagnostic_msgs::msg::DiagnosticStatus::STALE;
  dock_status.message = power.dock_fresh ? "fresh" : "absent_or_stale_non_blocking";
  output.status.push_back(std::move(dock_status));

  diagnostic_msgs::msg::DiagnosticStatus current_status;
  current_status.name = "mowgli_mavros_bridge/power_currents";
  current_status.level = power.traction_fresh ? diagnostic_msgs::msg::DiagnosticStatus::OK :
      diagnostic_msgs::msg::DiagnosticStatus::STALE;
  current_status.message = power.traction_fresh ? "traction_fresh" : "traction_missing_or_stale";
  add_value(current_status, "dock_charge_state_raw", std::to_string(power.dock_charge_state_raw));
  add_value(current_status, "dock_current_raw_a", format_value(power.dock_current_raw));
  add_value(current_status, "traction_current_raw_a", format_value(power.traction_current_raw));
  add_value(current_status, "traction_current_a", format_value(power.traction_current));
  add_value(current_status, "charge_current_a", format_value(power.charge_current));
  add_value(current_status, "battery_net_current_a", format_value(power.battery_net_current));
  add_value(current_status, "traction_voltage_v", format_value(power.v_battery));
  add_value(current_status, "charger_voltage_v", format_value(power.v_charge));
  add_value(current_status, "traction_percentage", format_value(power.traction_percentage));
  add_value(current_status, "charger_state", !power.dock_fresh ? "unavailable" :
            (power.charger_enabled ? "charging" : "unknown"));
  output.status.push_back(std::move(current_status));

  constexpr const char* kEscNames[3] = {"right_wheel", "left_wheel", "mower"};
  for (unsigned i = 0; i < esc.size(); ++i)
  {
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = std::string("mowgli_mavros_bridge/vesc_") + kEscNames[i];
    status.level = esc[i].online ? diagnostic_msgs::msg::DiagnosticStatus::OK :
        diagnostic_msgs::msg::DiagnosticStatus::STALE;
    status.message = esc[i].online ? "online" : (esc[i].stale ? "stale" : "absent");
    add_value(status, "esc_index", std::to_string(i));
    add_value(status, "online", esc[i].online ? "true" : "false");
    add_value(status, "stale", esc[i].stale ? "true" : "false");
    add_value(status, "age_ms", std::to_string(esc[i].age_ms));
    if (esc[i].observed)
    {
      add_value(status, "rpm_abs", std::to_string(std::abs(esc[i].sample.rpm)));
      add_value(status, "voltage_v", format_value(esc[i].sample.voltage));
      add_value(status, "current_a", format_value(esc[i].sample.current));
      add_value(status, "totalcurrent_ah", format_value(esc[i].sample.totalcurrent));
      add_value(status, "temperature_c", format_value(esc[i].sample.temperature));
      add_value(status, "count", std::to_string(esc[i].sample.count));
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
