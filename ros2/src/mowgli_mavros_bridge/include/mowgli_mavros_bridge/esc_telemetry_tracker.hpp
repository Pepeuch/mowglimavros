#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include "mavros_esc_wheel_odometry/msg/esc_observation.hpp"

namespace mowgli_mavros_bridge
{
struct EscSample
{
  int32_t rpm{0};
  float voltage{0}, current{0}, totalcurrent{0}, temperature{0};
  uint16_t count{0}, failure_flags{0};
  uint32_t error_count{0};
  uint8_t source{0};
  bool rpm_valid{false}, voltage_valid{false}, current_valid{false}, temperature_valid{false};
  bool totalcurrent_valid{false}, count_valid{false}, failure_flags_valid{false},
  error_count_valid{false};
};
struct EscState
{
  EscSample sample{};
  bool observed{false}, valid{false}, online{false}, stale{false};
  int64_t age_ms{-1}, last_update_ns{0}, metadata_ns{0};
};
// Only canonical validity/time semantics. No MAVLink decoder or counter logic.
class EscTelemetryTracker
{
public:
  explicit EscTelemetryTracker(double timeout_s = 3.0)
  : timeout_ns_(static_cast<int64_t>(timeout_s * 1e9)) {}
  void reset() {states_ = {};}
  void observe(const mavros_esc_wheel_odometry::msg::EscObservation & observation, int64_t now_ns)
  {
    if (observation.esc_index >= states_.size() || now_ns <= 0 || timeout_ns_ <= 0 ||
      (observation.source != observation.SOURCE_COMMON &&
      observation.source != observation.SOURCE_ARDUPILOT_LEGACY)) {return;}
    const int64_t stamp = ns(observation.header.stamp);
    const int64_t meta = ns(observation.metadata_stamp);
    auto & state = states_[observation.esc_index];
    if (stamp > now_ns || (state.sample.source == observation.source &&
      stamp > 0 && stamp < state.last_update_ns)) {return;}
    state.observed = state.observed || observation.valid || observation.temperature_valid ||
      observation.failure_flags_valid || observation.error_count_valid;
    state.valid = observation.valid && stamp > 0;
    state.last_update_ns = stamp; state.metadata_ns = meta;
    auto & s = state.sample;
    s.source = observation.source; s.rpm = observation.rpm;
    s.rpm_valid = observation.rpm_valid;
    s.voltage = observation.voltage;
    s.voltage_valid = observation.voltage_valid && std::isfinite(s.voltage);
    s.current = observation.current;
    s.current_valid = observation.current_valid && std::isfinite(s.current);
    s.temperature = observation.temperature;
    s.temperature_valid = observation.temperature_valid && std::isfinite(s.temperature);
    s.totalcurrent = observation.totalcurrent;
    s.totalcurrent_valid = observation.totalcurrent_valid && std::isfinite(s.totalcurrent);
    s.count = observation.count; s.count_valid = observation.count_valid;
    s.failure_flags = observation.failure_flags;
    s.failure_flags_valid = observation.failure_flags_valid;
    s.error_count = observation.error_count; s.error_count_valid = observation.error_count_valid;
  }
  EscState project(unsigned index, int64_t now_ns) const
  {
    if (index >= states_.size()) {return {};}
    auto out = states_[index];
    out.age_ms = out.last_update_ns > 0 && now_ns >= out.last_update_ns ?
      (now_ns - out.last_update_ns) / 1000000LL : -1;
    out.online = out.valid && fresh(out.last_update_ns, now_ns);
    out.stale = out.observed && !out.online;
    out.sample.rpm_valid = out.sample.rpm_valid && out.online;
    out.sample.voltage_valid = out.sample.voltage_valid && out.online;
    out.sample.current_valid = out.sample.current_valid && out.online;
    out.sample.totalcurrent_valid = out.sample.totalcurrent_valid && out.online;
    out.sample.count_valid = out.sample.count_valid && out.online;
    const bool metadata_fresh = fresh(out.metadata_ns, now_ns);
    out.sample.temperature_valid = out.sample.temperature_valid && metadata_fresh;
    out.sample.failure_flags_valid = out.sample.failure_flags_valid && metadata_fresh;
    out.sample.error_count_valid = out.sample.error_count_valid && metadata_fresh;
    return out;
  }

private:
  static int64_t ns(const builtin_interfaces::msg::Time & stamp)
  {
    return stamp.sec >= 0 && stamp.nanosec < 1000000000U ?
           static_cast<int64_t>(stamp.sec) * 1000000000LL + stamp.nanosec : 0;
  }
  bool fresh(int64_t stamp, int64_t now) const
  {return stamp > 0 && now >= stamp && now - stamp <= timeout_ns_;}
  int64_t timeout_ns_;
  std::array<EscState, 64> states_{};
};
}  // namespace mowgli_mavros_bridge
