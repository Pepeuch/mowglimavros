#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <iomanip>
#include <limits>
#include <sstream>
#include <mutex>
#include <string>
#include <stdexcept>

#include "mavros/mavros_uas.hpp"
#include "mavros/plugin.hpp"
#include "mavros/plugin_filter.hpp"
#include "diagnostic_msgs/msg/diagnostic_array.hpp"
#include "diagnostic_msgs/msg/diagnostic_status.hpp"
#include "diagnostic_msgs/msg/key_value.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "mowgli_interfaces/msg/wheel_tick.hpp"
#include "mavros_esc_wheel_odometry/msg/esc_observation.hpp"
#include "mavros_esc_wheel_odometry/observation_engine.hpp"
#include "mavros_esc_wheel_odometry/wheel_tick_projection.hpp"

namespace mavros_esc_wheel_odometry
{
class EscWheelOdometryPlugin : public mavros::plugin::Plugin
{
public:
  explicit EscWheelOdometryPlugin(mavros::plugin::UASPtr uas)
  : Plugin(uas, "esc_wheel_odometry")
  {
    ObservationConfig config;
    config.geometry = WheelGeometry{
      declare_index("left_esc_slot", 1, 63),
      declare_index("right_esc_slot", 0, 63),
      node->declare_parameter<double>("track_width_m", 0.0),
      node->declare_parameter<double>("ticks_per_meter", 0.0)};
    left_rpm_instance_ = declare_index("left_rpm_instance", 2, 2);
    right_rpm_instance_ = declare_index("right_rpm_instance", 1, 2);
    const int offset = declare_index("expected_esc_telem_mav_offset", 0, 255);
    config.legacy_enabled = offset == 0 && (left_rpm_instance_ == 1 || left_rpm_instance_ == 2) &&
      (right_rpm_instance_ == 1 || right_rpm_instance_ == 2) &&
      left_rpm_instance_ != right_rpm_instance_;
    config.esc_component_id = declare_index("esc_component_id", -1, 255);
    config.common_pair_max_skew_s = node->declare_parameter<double>("common_pair_max_skew_s", 0.25);
    config.source = node->declare_parameter<std::string>("source", "auto");
    config.left_wheel_index = declare_index("left_wheel_index", -1, 15);
    config.wheel_distance_component_id = declare_index("wheel_distance_component_id", -1, 255);
    config.right_wheel_index = declare_index("right_wheel_index", -1, 15);
    const double timeout = node->declare_parameter<double>("observation_timeout_s", 3.0);
    if (!std::isfinite(timeout) || timeout <= 0 || timeout > 3600) {
      throw std::invalid_argument("observation_timeout_s must be finite and in (0, 3600]");
    }
    config.timeout_ns = static_cast<int64_t>(timeout * 1e9);
    config.max_distance_speed_mps = node->declare_parameter<double>("max_distance_speed_mps", 10.0);
    frame_id_ = node->declare_parameter<std::string>("frame_id", "odom");
    child_frame_id_ = node->declare_parameter<std::string>("child_frame_id", "base_link");
    velocity_stddev_ = node->declare_parameter<double>("velocity_stddev_mps", 0.1);
    config_ = config; expected_offset_ = offset; connected_ = uas->is_connected();
    engine_ = std::make_unique<ObservationEngine>(config);
    engine_->connection(connected_);
    esc_pub_ = node->create_publisher<msg::EscObservation>(
      "/mavros/esc_wheel_odometry/esc_observation", rclcpp::SensorDataQoS().keep_last(64));
    diagnostics_pub_ = node->create_publisher<diagnostic_msgs::msg::DiagnosticArray>("/diagnostics",
        10);
    wheel_ticks_pub_ = node->create_publisher<mowgli_interfaces::msg::WheelTick>("/wheel_ticks", 10);
    if (engine_->wheel_configured() && !frame_id_.empty() && !child_frame_id_.empty() &&
      std::isfinite(velocity_stddev_) && velocity_stddev_ >= 0 &&
      std::isfinite(velocity_stddev_ * velocity_stddev_))
    {
      odom_pub_ = node->create_publisher<nav_msgs::msg::Odometry>("/wheel_odom", 10);
    } else {
      RCLCPP_WARN(get_logger(),
          "Wheel odometry disabled until source mapping, geometry, frames and covariance are configured; ESC normalization remains active.");
    }
    enable_connection_cb();
    parameter_validation_ = node->add_on_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter> & parameters) {
        std::lock_guard<std::mutex> lock(mutex_);
        rcl_interfaces::msg::SetParametersResult result; result.successful = false;
        pending_.reset();
        try {
          auto next = std::make_unique<Pending>(Pending{config_, frame_id_, child_frame_id_,
            velocity_stddev_, left_rpm_instance_, right_rpm_instance_, expected_offset_, {}});
          bool changed = false;
          for (const auto & p : parameters) {changed = update_parameter(*next, p) || changed;}
          if (changed) {
            next->config.legacy_enabled = next->offset == 0 &&
            (next->left_rpm == 1 || next->left_rpm == 2) &&
            (next->right_rpm == 1 || next->right_rpm == 2) && next->left_rpm != next->right_rpm;
            if (next->frame.empty() || next->child.empty() || !std::isfinite(next->stddev) ||
            next->stddev < 0 ||
            !std::isfinite(next->stddev * next->stddev))
            {
              throw std::invalid_argument("invalid odometry frames or covariance");
            }
            next->engine = std::make_unique<ObservationEngine>(next->config);
            pending_ = std::move(next);
          }
          result.successful = true;
        } catch (const std::exception & e) {result.reason = e.what();}
        return result;
      });
    parameter_apply_ = node->add_post_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter> &) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pending_) {return;}
        const bool same_legacy_mapping = left_rpm_instance_ == pending_->left_rpm &&
        right_rpm_instance_ == pending_->right_rpm && expected_offset_ == pending_->offset;
        config_ = pending_->config; frame_id_ = pending_->frame; child_frame_id_ = pending_->child;
        velocity_stddev_ = pending_->stddev; left_rpm_instance_ = pending_->left_rpm;
        right_rpm_instance_ = pending_->right_rpm; expected_offset_ = pending_->offset;
        pending_->engine->retain_esc_observations(*engine_, node->now().nanoseconds(),
          same_legacy_mapping);
        engine_ = std::move(pending_->engine); pending_.reset();
        if (engine_->wheel_configured()) {
          if (!odom_pub_) {
            odom_pub_ = node->create_publisher<nav_msgs::msg::Odometry>("/wheel_odom", 10);
          }
        } else {odom_pub_.reset();}
      });
    timer_ = node->create_wall_timer(std::chrono::milliseconds(500), [this]() {
          std::lock_guard<std::mutex> lock(mutex_);
          const auto now = node->now().nanoseconds();
          engine_->poll(now); publish_esc(now); publish_diagnostics();
    });
  }
  Subscriptions get_subscriptions() override
  {
    return {make_handler(&EscWheelOdometryPlugin::handle_rpm),
      make_handler(&EscWheelOdometryPlugin::handle_legacy_1),
      make_handler(&EscWheelOdometryPlugin::handle_legacy_5),
      make_handler(&EscWheelOdometryPlugin::handle_legacy_9),
      make_handler(&EscWheelOdometryPlugin::handle_status),
      make_handler(&EscWheelOdometryPlugin::handle_info),
      make_handler(&EscWheelOdometryPlugin::handle_distance)};
  }

private:
  struct Pending
  {
    ObservationConfig config;
    std::string frame, child;
    double stddev;
    int left_rpm, right_rpm, offset;
    std::unique_ptr<ObservationEngine> engine;
  };
  static int checked_index(int64_t value, int maximum)
  {
    if (value < -1 || value > maximum) {
      throw std::invalid_argument("index parameter out of range");
    }
    return static_cast<int>(value);
  }
  int declare_index(const std::string & name, int value, int maximum)
  {return checked_index(node->declare_parameter<int64_t>(name, value), maximum);}
  static bool update_parameter(Pending & next, const rclcpp::Parameter & p)
  {
    const auto & name = p.get_name(); auto & g = next.config.geometry;
    if (name == "esc_component_id") {
      next.config.esc_component_id = checked_index(p.as_int(), 255);
    } else if (name == "common_pair_max_skew_s") {
      next.config.common_pair_max_skew_s = p.as_double();
    } else if (name == "source") {
      next.config.source = p.as_string();
    } else if (name == "left_esc_slot") {
      g.left_esc_slot = checked_index(p.as_int(), 63);
    } else if (name == "right_esc_slot") {
      g.right_esc_slot = checked_index(p.as_int(), 63);
    } else if (name == "ticks_per_meter") {
      g.ticks_per_meter = p.as_double();
    } else if (name == "track_width_m") {
      g.track_width_m = p.as_double();
    } else if (name == "left_wheel_index") {
      next.config.left_wheel_index = checked_index(p.as_int(), 15);
    } else if (name == "right_wheel_index") {
      next.config.right_wheel_index = checked_index(p.as_int(), 15);
    } else if (name == "wheel_distance_component_id") {
      next.config.wheel_distance_component_id = checked_index(p.as_int(), 255);
    } else if (name == "left_rpm_instance") {
      next.left_rpm = checked_index(p.as_int(), 2);
    } else if (name == "right_rpm_instance") {
      next.right_rpm = checked_index(p.as_int(), 2);
    } else if (name == "expected_esc_telem_mav_offset") {
      next.offset = checked_index(p.as_int(), 255);
    } else if (name == "observation_timeout_s") {
      const double value = p.as_double();
      if (!std::isfinite(value) || value <= 0 || value > 3600) {
        throw std::invalid_argument("invalid observation timeout");
      }
      next.config.timeout_ns = static_cast<int64_t>(value * 1e9);
    } else if (name == "max_distance_speed_mps") {
      next.config.max_distance_speed_mps = p.as_double();
    } else if (name == "frame_id") {
      next.frame = p.as_string();
    } else if (name == "child_frame_id") {
      next.child = p.as_string();
    } else if (name == "velocity_stddev_mps") {next.stddev = p.as_double();} else {return false;}
    return true;
  }
  static builtin_interfaces::msg::Time stamp(int64_t ns)
  {
    builtin_interfaces::msg::Time out;
    if (ns > 0) {
      out.sec = static_cast<int32_t>(ns / 1000000000LL); out.nanosec = ns % 1000000000LL;
    }
    return out;
  }
  void publish_esc(int64_t now)
  {
    for (unsigned index = 0; index < 64; ++index) {
      if (!engine_->touched(index)) {continue;}
      const auto d = engine_->esc(index, now);
      msg::EscObservation out;
      out.header.stamp = stamp(d.stamp_ns); out.metadata_stamp = stamp(d.metadata_stamp_ns);
      out.esc_index = index; out.source = static_cast<uint8_t>(d.source); out.valid = d.valid;
      out.rpm = d.rpm; out.rpm_valid = d.rpm_valid; out.rpm_direction_valid = d.rpm_direction_valid;
      out.voltage = d.voltage; out.voltage_valid = d.voltage_valid;
      out.current = d.current; out.current_valid = d.current_valid;
      out.temperature = d.temperature; out.temperature_valid = d.temperature_valid;
      out.failure_flags = d.failure_flags; out.failure_flags_valid = d.failure_flags_valid;
      out.error_count = d.error_count; out.error_count_valid = d.error_count_valid;
      out.totalcurrent = d.totalcurrent; out.totalcurrent_valid = d.totalcurrent_valid;
      out.count = d.count; out.count_valid = d.count_valid;
      esc_pub_->publish(out);
    }
  }
  static std::string precise(double value)
  {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return out.str();
  }
  void publish_diagnostics()
  {
    diagnostic_msgs::msg::DiagnosticArray out;
    out.header.stamp = node->now();
    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = "mavros_esc_wheel_odometry/source";
    status.level = engine_->active_source() == WheelSource::None ? status.STALE : status.OK;
    status.message = source_name(engine_->active_source());
    diagnostic_msgs::msg::KeyValue key;
    key.key = "active_source"; key.value = status.message; status.values.push_back(key);
    auto add = [&status](const std::string & name, const std::string & value) {
        diagnostic_msgs::msg::KeyValue item; item.key = name; item.value = value;
        status.values.push_back(item);
      };
    add("ticks_unit", "motor_revolution");
    add("wheel_tick_transport_scale", precise(WheelTickProjection::kCountsPerMotorRevolution));
    add("wheel_tick_source", source_name(wheel_tick_source_));
    add("ticks_per_meter", precise(config_.geometry.ticks_per_meter));
    add("rpm_metric_calibrated", config_.geometry.ticks_per_meter > 0 ? "true" : "false");
    const auto now = node->now().nanoseconds();
    for (auto source : {WheelSource::EscStatus, WheelSource::ArduPilotLegacy}) {
      const auto ticks = engine_->motor_ticks(source, now);
      const std::string prefix = std::string(source_name(source)) + "/";
      add(prefix + "epoch", std::to_string(ticks.epoch));
      add(prefix + "left_segment", std::to_string(ticks.left_segment));
      add(prefix + "right_segment", std::to_string(ticks.right_segment));
      add(prefix + "left_raw_ticks", precise(ticks.left_ticks));
      add(prefix + "right_raw_ticks", precise(ticks.right_ticks));
      add(prefix + "left_valid", ticks.left_valid ? "true" : "false");
      add(prefix + "right_valid", ticks.right_valid ? "true" : "false");
      add(prefix + "left_sample_stamp_ns", std::to_string(ticks.left_sample_ns));
      add(prefix + "right_sample_stamp_ns", std::to_string(ticks.right_sample_ns));
      add(prefix + "left_receipt_stamp_ns", std::to_string(ticks.left_receipt_ns));
      add(prefix + "right_receipt_stamp_ns", std::to_string(ticks.right_receipt_ns));
      if (config_.geometry.ticks_per_meter > 0) {
        const double left = ticks.left_ticks / config_.geometry.ticks_per_meter;
        const double right = ticks.right_ticks / config_.geometry.ticks_per_meter;
        add(prefix + "left_distance_valid",
            ticks.left_valid && std::isfinite(left) ? "true" : "false");
        add(prefix + "right_distance_valid",
            ticks.right_valid && std::isfinite(right) ? "true" : "false");
        if (std::isfinite(left)) {add(prefix + "left_distance_m", precise(left));}
        if (std::isfinite(right)) {add(prefix + "right_distance_m", precise(right));}
      }
    }
    out.status.push_back(status); diagnostics_pub_->publish(out);
  }
  void publish_wheel_ticks()
  {
    const auto now = node->now().nanoseconds();
    // Raw ticks remain available before metric calibration. Explicit source
    // selection is preserved; WHEEL_DISTANCE has no motor tick contract.
    WheelSource source = WheelSource::None;
    auto ticks = engine_->motor_ticks(WheelSource::EscStatus, now);
    if ((config_.source == "auto" || config_.source == "esc_status") &&
      (ticks.left_valid || ticks.right_valid)) {
      source = WheelSource::EscStatus;
    } else if (config_.source == "auto" || config_.source == "ardupilot_legacy") {
      ticks = engine_->motor_ticks(WheelSource::ArduPilotLegacy, now);
      source = WheelSource::ArduPilotLegacy;
    }
    if (!ticks.left_valid && !ticks.right_valid) {source = WheelSource::None;}
    wheel_tick_source_ = source;
    const auto projected = wheel_tick_projector_.project(source, ticks);
    if (!projected) {return;}
    mowgli_interfaces::msg::WheelTick out;
    out.stamp = stamp(std::max(ticks.left_receipt_ns, ticks.right_receipt_ns));
    out.wheel_tick_factor = WheelTickProjection::factor(config_.geometry.ticks_per_meter);
    // Keep magnitude counts continuous even while one wheel is invalid.
    out.wheel_direction_rl = projected->left_direction;
    out.wheel_ticks_rl = projected->left_count;
    out.wheel_direction_rr = projected->right_direction;
    out.wheel_ticks_rr = projected->right_count;
    if (projected->left_valid) {
      out.valid_wheels |= mowgli_interfaces::msg::WheelTick::WHEEL_VALID_RL;
    }
    if (projected->right_valid) {
      out.valid_wheels |= mowgli_interfaces::msg::WheelTick::WHEEL_VALID_RR;
    }
    wheel_ticks_pub_->publish(out);
  }
  void publish_wheel(const std::optional<WheelObservation> & observation)
  {
    if (!observation || !odom_pub_) {return;}
    nav_msgs::msg::Odometry odom;
    odom.header.stamp = stamp(observation->receipt_stamp_ns);
    odom.header.frame_id = frame_id_; odom.child_frame_id = child_frame_id_;
    odom.twist.twist.linear.x = observation->linear_x_mps;
    odom.twist.twist.angular.z = observation->angular_z_rps;
    const double variance = velocity_stddev_ * velocity_stddev_;
    odom.twist.covariance[0] = odom.twist.covariance[7] = odom.twist.covariance[35] = variance;
    for (unsigned index : {0U, 7U, 14U, 21U, 28U, 35U}) {odom.pose.covariance[index] = 1.0e6;}
    odom_pub_->publish(odom);
  }
  void handle_status(
    const mavlink::mavlink_message_t * message, mavlink::common::msg::ESC_STATUS & m,
    mavros::plugin::filter::SystemAndOk)
  {
    std::lock_guard<std::mutex> lock(mutex_); const auto now = node->now().nanoseconds();
    publish_wheel(engine_->common_status(CommonStatusPacket{m.index, m.time_usec, m.rpm, m.voltage,
        m.current, message->compid}, now));
    publish_esc(now); publish_wheel_ticks();
  }
  void handle_info(
    const mavlink::mavlink_message_t * message, mavlink::common::msg::ESC_INFO & m,
    mavros::plugin::filter::SystemAndOk)
  {
    std::lock_guard<std::mutex> lock(mutex_); const auto now = node->now().nanoseconds();
    engine_->common_info(CommonInfoPacket{m.index, m.count, m.time_usec, m.temperature,
        m.failure_flags, m.error_count, message->compid}, now);
    publish_esc(now);
  }
  void handle_distance(
    const mavlink::mavlink_message_t * message, mavlink::common::msg::WHEEL_DISTANCE & m,
    mavros::plugin::filter::SystemAndOk)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    publish_wheel(engine_->wheel_distance(DistancePacket{m.time_usec, m.count, m.distance,
        message->compid}, node->now().nanoseconds()));
  }
  void handle_rpm(
    const mavlink::mavlink_message_t *, mavlink::ardupilotmega::msg::RPM & m,
    mavros::plugin::filter::SystemAndOk)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    publish_wheel(engine_->legacy_rpm(left_rpm_instance_ == 1 ? m.rpm1 : m.rpm2,
      right_rpm_instance_ == 1 ? m.rpm1 : m.rpm2, node->now().nanoseconds()));
    publish_wheel_ticks();
  }
  template<typename T> void legacy_packet(const T & m, unsigned index)
  {
    std::lock_guard<std::mutex> lock(mutex_); const auto now = node->now().nanoseconds();
    LegacyEscPacket packet; packet.index = index; packet.counts = m.count; packet.rpm = m.rpm;
    for (unsigned i = 0; i < 4; ++i) {
      packet.voltage[i] = m.voltage[i] / 100.0F; packet.current[i] = m.current[i] / 100.0F;
      packet.temperature[i] = m.temperature[i];
      packet.totalcurrent[i] = m.totalcurrent[i] / 1000.0F;
    }
    engine_->legacy_esc(packet, now); publish_esc(now);
  }
  void handle_legacy_1(
    const mavlink::mavlink_message_t *, mavlink::ardupilotmega::msg::ESC_TELEMETRY_1_TO_4 & m,
    mavros::plugin::filter::SystemAndOk) {legacy_packet(m, 0);}
  void handle_legacy_5(
    const mavlink::mavlink_message_t *, mavlink::ardupilotmega::msg::ESC_TELEMETRY_5_TO_8 & m,
    mavros::plugin::filter::SystemAndOk) {legacy_packet(m, 4);}
  void handle_legacy_9(
    const mavlink::mavlink_message_t *, mavlink::ardupilotmega::msg::ESC_TELEMETRY_9_TO_12 & m,
    mavros::plugin::filter::SystemAndOk) {legacy_packet(m, 8);}
  void connection_cb(bool connected) override
  {
    std::lock_guard<std::mutex> lock(mutex_); connected_ = connected;
    engine_->connection(connected);
    if (!connected) {
      wheel_tick_projector_.project(WheelSource::None, MotorTickState{});
      wheel_tick_source_ = WheelSource::None;
    }
  }
  ObservationConfig config_;
  int expected_offset_{-1};
  bool connected_{false};
  std::unique_ptr<Pending> pending_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_validation_;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr parameter_apply_;
  std::unique_ptr<ObservationEngine> engine_;
  std::mutex mutex_;
  std::string frame_id_, child_frame_id_;
  int left_rpm_instance_{-1}, right_rpm_instance_{-1};
  double velocity_stddev_{0.1};
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<mowgli_interfaces::msg::WheelTick>::SharedPtr wheel_ticks_pub_;
  rclcpp::Publisher<msg::EscObservation>::SharedPtr esc_pub_;
  rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr diagnostics_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  WheelTickProjector wheel_tick_projector_;
  WheelSource wheel_tick_source_{WheelSource::None};
};
}  // namespace mavros_esc_wheel_odometry
#include <mavros/mavros_plugin_register_macro.hpp>
MAVROS_PLUGIN_REGISTER(mavros_esc_wheel_odometry::EscWheelOdometryPlugin)
