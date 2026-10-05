#include "mavros_esc_wheel_odometry/observation_engine.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace mavros_esc_wheel_odometry
{
namespace
{
bool sample_time_valid(uint64_t us)
{return us > 0 && us <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max() / 1000);}
}
const char * source_name(WheelSource source)
{
  switch (source) {
    case WheelSource::WheelDistance: return "wheel_distance";
    case WheelSource::EscStatus: return "esc_status";
    case WheelSource::ArduPilotLegacy: return "ardupilot_legacy";
    default: return "none";
  }
}
ObservationEngine::ObservationEngine(ObservationConfig config)
: config_(config), core_(config.geometry), legacy_(config.geometry, config.timeout_ns),
  common_motor_ticks_(config.timeout_ns), legacy_motor_ticks_(config.timeout_ns)
{
  if (config.source != "auto" && config.source != "wheel_distance" &&
    config.source != "esc_status" && config.source != "ardupilot_legacy")
  {
    throw std::invalid_argument(
        "source must be auto, wheel_distance, esc_status or ardupilot_legacy");
  }
  const auto & g = config.geometry;
  if (g.left_esc_slot < -1 || g.left_esc_slot >= 64 || g.right_esc_slot < -1 ||
    g.right_esc_slot >= 64 ||
    (g.left_esc_slot >= 0 && g.left_esc_slot == g.right_esc_slot) ||
    !std::isfinite(g.ticks_per_meter) || g.ticks_per_meter < 0 ||
    !std::isfinite(g.track_width_m) || g.track_width_m < 0 ||
    config.left_wheel_index < -1 || config.left_wheel_index >= 16 ||
    config.right_wheel_index < -1 || config.right_wheel_index >= 16 ||
    (config.left_wheel_index >= 0 && config.left_wheel_index == config.right_wheel_index))
  {throw std::invalid_argument("invalid wheel mapping or geometry");}
  if (config.wheel_distance_component_id < -1 || config.wheel_distance_component_id > 255)
  {throw std::invalid_argument("wheel_distance_component_id must be -1 or a MAVLink component id");}
  if (config.esc_component_id < -1 || config.esc_component_id > 255) {
    throw std::invalid_argument("esc_component_id must be -1 or a MAVLink component id");
  }
  if (!std::isfinite(config.common_pair_max_skew_s) || config.common_pair_max_skew_s <= 0 ||
    config.common_pair_max_skew_s > static_cast<double>(config.timeout_ns) * 1e-9)
  {
    throw std::invalid_argument(
        "COMMON pair skew must be finite, positive and <= freshness timeout");
  }
  if (config.timeout_ns <= 0 || !std::isfinite(config.max_distance_speed_mps) ||
    config.max_distance_speed_mps <= 0)
  {
    throw std::invalid_argument(
        "source timeout and distance speed limit must be finite and positive");
  }
}
bool ObservationEngine::fresh(int64_t stamp, int64_t now) const
{return stamp > 0 && now >= stamp && now - stamp <= config_.timeout_ns;}
bool ObservationEngine::common_mapping() const
{
  const auto & g = config_.geometry;
  return g.left_esc_slot >= 0 && g.left_esc_slot < 64 && g.right_esc_slot >= 0 &&
         g.right_esc_slot < 64 && g.left_esc_slot != g.right_esc_slot;
}
bool ObservationEngine::distance_mapping(unsigned count) const
{
  return count >= 2 && count <= 16 && config_.left_wheel_index >= 0 &&
         config_.right_wheel_index >= 0 && config_.left_wheel_index != config_.right_wheel_index &&
         static_cast<unsigned>(config_.left_wheel_index) < count &&
         static_cast<unsigned>(config_.right_wheel_index) < count;
}
bool ObservationEngine::wheel_configured() const
{
  if (!core_.valid()) {return false;}
  const bool auto_mode = config_.source == "auto";
  return ((auto_mode || config_.source == "wheel_distance") && distance_mapping(16)) ||
         ((auto_mode || config_.source == "esc_status") && common_mapping() &&
         config_.geometry.ticks_per_meter > 0) ||
         ((auto_mode || config_.source == "ardupilot_legacy") && config_.legacy_enabled &&
         legacy_.valid() && config_.geometry.ticks_per_meter > 0);
}
void ObservationEngine::reset_common()
{
  status_ = {}; info_ = {}; common_count_.reset(); common_component_.reset();
  common_motor_ticks_.reset();
  common_owner_stamp_ns_ = 0; common_status_owner_ = false;
  published_left_ = published_right_ = 0;
  common_epoch_baseline_needed_ = false; common_configuration_ns_ = 0;
  if (active_ == WheelSource::EscStatus) {core_.reset(); active_ = WheelSource::None;}
}
void ObservationEngine::reset_distance()
{
  distance_ = {}; distance_component_.reset(); distance_baseline_.reset();
  if (active_ == WheelSource::WheelDistance) {core_.reset(); active_ = WheelSource::None;}
}
void ObservationEngine::reset()
{
  reset_common(); reset_distance(); core_.reset(); legacy_.reset(); legacy_esc_ = {};
  legacy_motion_.reset(); legacy_motor_ticks_.reset(); active_ = WheelSource::None;
  last_receipt_ns_ = 0;
}
void ObservationEngine::receive_clock(int64_t receipt)
{
  if (receipt < last_receipt_ns_) {reset();}
  last_receipt_ns_ = receipt;
}
bool ObservationEngine::accept_common_status(uint8_t component, int64_t receipt)
{
  if (config_.esc_component_id >= 0 && component != config_.esc_component_id) {return false;}
  if (common_component_ && *common_component_ != component) {
    if (common_status_owner_ && fresh(common_owner_stamp_ns_, receipt)) {return false;}
    reset_common();
  }
  common_component_ = component;
  common_status_owner_ = true;
  return true;
}
bool ObservationEngine::accept_common_info(uint8_t component)
{
  if (config_.esc_component_id >= 0 && component != config_.esc_component_id) {return false;}
  if (common_component_ && *common_component_ != component) {
    // INFO can neither acquire nor take over a STATUS lease, even after expiry.
    if (common_status_owner_) {return false;}
    reset_common();
  }
  // Retain at most one INFO candidate. First STATUS remains authoritative.
  common_component_ = component;
  return true;
}
void ObservationEngine::connection(bool connected)
{reset(); connected_ = connected;}
void ObservationEngine::retain_esc_observations(
  const ObservationEngine & previous,
  int64_t configuration_ns, bool retain_legacy_ticks)
{
  connected_ = previous.connected_;
  status_ = previous.status_; info_ = previous.info_;
  legacy_esc_ = previous.legacy_esc_;
  last_receipt_ns_ = previous.last_receipt_ns_;
  if (config_.esc_component_id < 0 ||
    (previous.common_component_ && *previous.common_component_ == config_.esc_component_id))
  {
    common_count_ = previous.common_count_; common_component_ = previous.common_component_;
    common_owner_stamp_ns_ = previous.common_owner_stamp_ns_;
    common_status_owner_ = previous.common_status_owner_;
  } else {status_ = {}; info_ = {};}

  const bool same_slots = config_.geometry.left_esc_slot ==
    previous.config_.geometry.left_esc_slot &&
    config_.geometry.right_esc_slot == previous.config_.geometry.right_esc_slot;
  if (same_slots && common_component_ == previous.common_component_) {
    common_motor_ticks_.retain_counts(previous.common_motor_ticks_);
  }
  if (same_slots && retain_legacy_ticks) {
    legacy_motor_ticks_.retain_counts(previous.legacy_motor_ticks_);
  }
  common_configuration_ns_ = configuration_ns;
}
bool ObservationEngine::common_available(int64_t now) const
{
  if (!common_mapping()) {return false;}
  const auto & l = status_[config_.geometry.left_esc_slot];
  const auto & r = status_[config_.geometry.right_esc_slot];
  if (!l.seen || !r.seen || !l.data.valid || !r.data.valid ||
    !fresh(l.data.stamp_ns, now) || !fresh(r.data.stamp_ns, now)) {return false;}
  if (common_count_ && (static_cast<unsigned>(config_.geometry.left_esc_slot) >= *common_count_ ||
    static_cast<unsigned>(config_.geometry.right_esc_slot) >= *common_count_)) {return false;}
  const uint64_t delta = std::max(l.time_us, r.time_us) - std::min(l.time_us, r.time_us);
  return static_cast<double>(delta) * 1e-6 <= config_.common_pair_max_skew_s;
}
void ObservationEngine::select(
  int64_t now, bool common_update,
  uint64_t previous_left, uint64_t previous_right)
{
  WheelSource next = WheelSource::None;
  if (connected_ && wheel_configured()) {
    const bool automatic = config_.source == "auto";
    if ((automatic || config_.source == "wheel_distance") && distance_.valid &&
      fresh(distance_.receipt_ns, now))
    {
      next = WheelSource::WheelDistance;
    } else if ((automatic || config_.source == "esc_status") && common_available(now)) {
      next = WheelSource::EscStatus;
    } else if ((automatic || config_.source == "ardupilot_legacy") && config_.legacy_enabled &&
      legacy_motion_ && fresh(legacy_motion_->receipt_stamp_ns, now))
    {
      next = WheelSource::ArduPilotLegacy;
    }
  }
  if (next != active_) {
    core_.reset();
    // These per-wheel floors fence cached data when returning from another source.
    // A temporary COMMON skew gap retains its pending new wheel, so normal
    // separate-group pairing is not mistaken for a cross-source transition.
    if (common_mapping()) {
      const auto left = status_[config_.geometry.left_esc_slot].time_us;
      const auto right = status_[config_.geometry.right_esc_slot].time_us;
      if ((next != WheelSource::EscStatus &&
        (next != WheelSource::None || active_ != WheelSource::EscStatus)) ||
        (next == WheelSource::EscStatus && !common_update))
      {
        published_left_ = left; published_right_ = right;
      } else if (next == WheelSource::EscStatus && active_ != WheelSource::None) {
        published_left_ = previous_left; published_right_ = previous_right;
      }
    }
    distance_baseline_.reset();
    if (next == WheelSource::WheelDistance) {distance_baseline_ = distance_;}
    active_ = next;
  }
}
void ObservationEngine::poll(int64_t now) {receive_clock(now); select(now);}
std::optional<WheelObservation> ObservationEngine::common_status(
  const CommonStatusPacket & packet, int64_t receipt)
{
  if (!connected_ || receipt <= 0 || !sample_time_valid(packet.time_us) || packet.index > 60 ||
    packet.index % 4)
  {
    return std::nullopt;
  }
  receive_clock(receipt);
  if (!accept_common_status(packet.component_id, receipt)) {return std::nullopt;}
  const uint64_t previous_left =
    common_mapping() ? status_[config_.geometry.left_esc_slot].time_us : 0;
  const uint64_t previous_right =
    common_mapping() ? status_[config_.geometry.right_esc_slot].time_us : 0;
  // Only this COMMON producer's epoch changed; independent wheel/legacy data survives.
  for (unsigned i = 0; i < 4; ++i) {
    const auto & old = status_[packet.index + i];
    if (old.seen && packet.time_us < old.time_us) {
      reset_common(); common_component_ = packet.component_id; common_status_owner_ = true;
      common_epoch_baseline_needed_ = true; break;
    }
  }
  for (unsigned i = 0; i < 4; ++i) {
    const unsigned slot = packet.index + i;
    if (common_count_ && slot >= *common_count_) {continue;}
    auto & state = status_[slot];
    if (state.seen && packet.time_us == state.time_us) {
      const bool voltage_valid = std::isfinite(packet.voltage[i]);
      const bool current_valid = std::isfinite(packet.current[i]);
      if (state.data.rpm != packet.rpm[i] || state.data.voltage_valid != voltage_valid ||
        state.data.current_valid != current_valid ||
        (voltage_valid && state.data.voltage != packet.voltage[i]) ||
        (current_valid && state.data.current != packet.current[i]))
      {
        state.data.valid = false;
        if (static_cast<int>(slot) == config_.geometry.left_esc_slot) {
          common_motor_ticks_.invalidate(0);
        }
        if (static_cast<int>(slot) == config_.geometry.right_esc_slot) {
          common_motor_ticks_.invalidate(1);
        }
      }
      continue;
    }
    state.seen = true; state.time_us = packet.time_us;
    common_owner_stamp_ns_ = receipt;
    auto & d = state.data;
    d.index = slot; d.source = EscSource::Common; d.stamp_ns = receipt; d.valid = true;
    d.rpm = packet.rpm[i]; d.rpm_valid = d.rpm_direction_valid = true;
    d.voltage_valid = std::isfinite(packet.voltage[i]);
    d.current_valid = std::isfinite(packet.current[i]);
    d.voltage = d.voltage_valid ? packet.voltage[i] : 0.0F;
    d.current = d.current_valid ? packet.current[i] : 0.0F;
    if (static_cast<int>(slot) == config_.geometry.left_esc_slot) {
      common_motor_ticks_.observe(0, d.rpm, static_cast<int64_t>(packet.time_us * 1000), receipt);
    }
    if (static_cast<int>(slot) == config_.geometry.right_esc_slot) {
      common_motor_ticks_.observe(1, d.rpm, static_cast<int64_t>(packet.time_us * 1000), receipt);
    }
  }
  select(receipt, true, previous_left, previous_right);
  if (active_ != WheelSource::EscStatus || !common_available(receipt)) {return std::nullopt;}
  const auto & left_state = status_[config_.geometry.left_esc_slot];
  const auto & right_state = status_[config_.geometry.right_esc_slot];
  const auto & l = left_state.data;
  const auto & r = right_state.data;
  if (l.stamp_ns <= common_configuration_ns_ || r.stamp_ns <= common_configuration_ns_) {
    return std::nullopt;
  }
  if (common_epoch_baseline_needed_) {
    common_epoch_baseline_needed_ = false; published_left_ = left_state.time_us;
    published_right_ = right_state.time_us;
    return std::nullopt;
  }
  if (left_state.time_us <= published_left_ || right_state.time_us <= published_right_) {
    return std::nullopt;
  }
  auto motion = common_motor_ticks_.motion(config_.geometry.ticks_per_meter,
      WheelSource::EscStatus);
  auto observation = motion ? core_.receive_motion(*motion) : std::nullopt;
  if (observation) {published_left_ = left_state.time_us; published_right_ = right_state.time_us;}
  return observation;
}
void ObservationEngine::common_info(const CommonInfoPacket & packet, int64_t receipt)
{
  if (!connected_ || receipt <= 0 || !sample_time_valid(packet.time_us) || packet.index > 60 ||
    packet.index % 4 || packet.count > 64 ||
    (packet.count == 0 ? packet.index != 0 : packet.index >= packet.count)) {return;}
  receive_clock(receipt);
  if (!accept_common_info(packet.component_id)) {return;}
  bool metadata_epoch_changed = false;
  for (unsigned i = 0; i < 4 && packet.index + i < packet.count; ++i) {
    const auto & old = info_[packet.index + i];
    if (old.seen && packet.time_us < old.time_us) {metadata_epoch_changed = true;}
  }
  if (metadata_epoch_changed) {
    for (unsigned i = 0; i < 4 && packet.index + i < packet.count; ++i) {
      info_[packet.index + i] = Info{packet.time_us, 0, packet.temperature[i],
        packet.failure_flags[i], packet.error_count[i], true, false};
    }
    // Reject the reset sample's metadata; a subsequent measure establishes freshness.
    select(receipt);
    return;
  }
  common_count_ = packet.count;
  for (unsigned slot = packet.count; slot < status_.size(); ++slot) {
    status_[slot].data.valid = false; info_[slot].seen = false;
  }
  for (unsigned i = 0; i < 4 && packet.index + i < packet.count; ++i) {
    auto & state = info_[packet.index + i];
    if (state.seen && packet.time_us == state.time_us) {
      if (state.temperature != packet.temperature[i] ||
        state.failure_flags != packet.failure_flags[i] ||
        state.error_count != packet.error_count[i]) {state.valid = false;}
      continue;
    }
    state = Info{packet.time_us, receipt, packet.temperature[i], packet.failure_flags[i],
      packet.error_count[i], true, true};
  }
  select(receipt);
}
void ObservationEngine::legacy_esc(const LegacyEscPacket & packet, int64_t receipt)
{
  if (!connected_ || receipt <= 0 ||
    (packet.index != 0 && packet.index != 4 && packet.index != 8))
  {
    return;
  }
  receive_clock(receipt);
  legacy_.receive_esc_counts(static_cast<int>(packet.index), packet.counts, receipt);
  for (unsigned i = 0; i < 4; ++i) {
    const auto slot = packet.index + i;
    auto & state = legacy_esc_[slot];
    auto & d = state.data;
    if (receipt < d.stamp_ns) {continue;}
    if ((!state.count && packet.counts[i] != 0) ||
      (state.count && LegacyWheelAdapter::counter_advanced(*state.count, packet.counts[i])))
    {d.stamp_ns = receipt; d.valid = true;}
    state.count = packet.counts[i];
    d.index = slot; d.source = EscSource::ArduPilotLegacy;
    d.rpm = packet.rpm[i]; d.rpm_valid = true; d.rpm_direction_valid = false;
    d.voltage_valid = std::isfinite(packet.voltage[i]);
    d.current_valid = std::isfinite(packet.current[i]);
    d.temperature_valid = std::isfinite(packet.temperature[i]);
    d.voltage = d.voltage_valid ? packet.voltage[i] : 0.0F;
    d.current = d.current_valid ? packet.current[i] : 0.0F;
    d.temperature = d.temperature_valid ? packet.temperature[i] : 0.0F;
    d.metadata_stamp_ns = d.stamp_ns;
    d.totalcurrent_valid = std::isfinite(packet.totalcurrent[i]);
    d.totalcurrent = d.totalcurrent_valid ? packet.totalcurrent[i] : 0.0F;
    d.count = packet.counts[i]; d.count_valid = true;
  }
}
std::optional<WheelObservation> ObservationEngine::legacy_rpm(
  double left, double right,
  int64_t receipt)
{
  if (!connected_ || receipt <= 0) {return std::nullopt;}
  receive_clock(receipt);
  if (!std::isfinite(left) || !std::isfinite(right)) {
    legacy_motion_.reset(); legacy_motor_ticks_.reset_references();
    select(receipt); return std::nullopt;
  }
  const auto raw = legacy_.receive_rpm(left, right, receipt);
  std::optional<WheelMotionObservation> motion;
  if (raw && config_.legacy_enabled) {
    const bool left_ok = legacy_motor_ticks_.observe(0, raw->left_rpm, raw->sample_stamp_ns,
        raw->receipt_stamp_ns);
    const bool right_ok = legacy_motor_ticks_.observe(1, raw->right_rpm, raw->sample_stamp_ns,
        raw->receipt_stamp_ns);
    if (left_ok && right_ok) {
      motion = legacy_motor_ticks_.motion(config_.geometry.ticks_per_meter,
          WheelSource::ArduPilotLegacy);
    }
  }
  if (motion) {legacy_motion_ = motion;}
  select(receipt);
  if (active_ == WheelSource::ArduPilotLegacy && motion) {return core_.receive_motion(*motion);}
  return std::nullopt;
}
std::optional<WheelObservation> ObservationEngine::wheel_distance(
  const DistancePacket & packet,
  int64_t receipt)
{
  if (!connected_ || receipt <= 0) {return std::nullopt;}
  receive_clock(receipt);
  if (config_.wheel_distance_component_id >= 0 &&
    packet.component_id != config_.wheel_distance_component_id) {return std::nullopt;}
  // One encoder component owns a live source; another may take over after stale.
  if (distance_component_ && *distance_component_ != packet.component_id) {
    if (distance_.valid && fresh(distance_.receipt_ns, receipt)) {return std::nullopt;}
    reset_distance();
  }
  bool good = receipt > 0 && sample_time_valid(packet.time_us) && distance_mapping(packet.count);
  for (unsigned i = 0; i < std::min<unsigned>(packet.count, 16); ++i) {
    good = good && std::isfinite(packet.distance[i]);
  }
  if (!good) {
    distance_.valid = false; distance_baseline_.reset(); select(receipt); return std::nullopt;
  }
  const Distance next{packet.distance[config_.left_wheel_index],
    packet.distance[config_.right_wheel_index],
    packet.time_us, receipt, true};
  if (distance_.valid && packet.time_us == distance_.time_us) {
    if (next.left != distance_.left || next.right != distance_.right) {
      distance_.valid = false; distance_baseline_.reset(); select(receipt);
    }
    return std::nullopt;
  }
  if (distance_.valid && packet.time_us < distance_.time_us) {
    reset_distance();
  }
  if (!fresh(distance_.receipt_ns, receipt)) {distance_baseline_.reset();}
  distance_ = next; distance_component_ = packet.component_id;
  select(receipt);
  if (active_ != WheelSource::WheelDistance) {return std::nullopt;}
  if (!distance_baseline_) {distance_baseline_ = next; return std::nullopt;}
  const auto baseline = *distance_baseline_;
  distance_baseline_ = next;
  if (next.time_us <= baseline.time_us || next.receipt_ns < baseline.receipt_ns ||
    next.time_us - baseline.time_us > static_cast<uint64_t>(config_.timeout_ns / 1000))
  {
    return std::nullopt;
  }
  const double dt = static_cast<double>(next.time_us - baseline.time_us) * 1e-6;
  const double left = (next.left - baseline.left) / dt;
  const double right = (next.right - baseline.right) / dt;
  if (!std::isfinite(left) || !std::isfinite(right) ||
    std::abs(left) > config_.max_distance_speed_mps ||
    std::abs(right) > config_.max_distance_speed_mps)
  {
    distance_.valid = false; distance_baseline_.reset(); select(receipt); return std::nullopt;
  }
  return core_.receive_motion(WheelMotionObservation{left, right,
             static_cast<int64_t>(next.time_us * 1000), receipt, WheelSource::WheelDistance, true});
}
MotorTickState ObservationEngine::motor_ticks(WheelSource source, int64_t now) const
{
  MotorTickState out;
  if (source == WheelSource::EscStatus) {
    out = common_motor_ticks_.state();
    const auto & g = config_.geometry;
    if (!common_mapping()) {out.left_valid = out.right_valid = false;} else {
      out.left_valid = out.left_valid && status_[g.left_esc_slot].data.valid &&
        (!common_count_ || static_cast<unsigned>(g.left_esc_slot) < *common_count_);
      out.right_valid = out.right_valid && status_[g.right_esc_slot].data.valid &&
        (!common_count_ || static_cast<unsigned>(g.right_esc_slot) < *common_count_);
    }
  } else if (source == WheelSource::ArduPilotLegacy) {out = legacy_motor_ticks_.state();}
  out.left_valid = out.left_valid && connected_ && fresh(out.left_receipt_ns, now);
  out.right_valid = out.right_valid && connected_ && fresh(out.right_receipt_ns, now);
  return out;
}
bool ObservationEngine::touched(unsigned index) const
{
  return index < 64 && (status_[index].seen || info_[index].seen ||
         (index < 12 && legacy_esc_[index].count.has_value()));
}
EscObservationData ObservationEngine::esc(unsigned index, int64_t now) const
{
  EscObservationData out;
  out.index = index;
  if (!connected_ || index >= 64) {return out;}
  const auto & common = status_[index];
  const bool in_count = !common_count_ || index < *common_count_;
  if (common.seen && common.data.valid && in_count && fresh(common.data.stamp_ns, now)) {
    out = common.data;
    const auto & information = info_[index];
    const uint64_t sample_delta = std::max(common.time_us, information.time_us) -
      std::min(common.time_us, information.time_us);
    if (information.seen && information.valid && fresh(information.stamp_ns, now) &&
      sample_delta <= static_cast<uint64_t>(config_.timeout_ns / 1000))
    {
      out.metadata_stamp_ns = information.stamp_ns;
      out.temperature_valid = information.temperature != std::numeric_limits<int16_t>::max();
      if (out.temperature_valid) {
        out.temperature = static_cast<float>(information.temperature) * 0.01F;
      }
      out.failure_flags = information.failure_flags; out.error_count = information.error_count;
      out.failure_flags_valid = out.error_count_valid = true;
    }
    return out;
  }
  if (index < 12 && legacy_esc_[index].count) {
    out = legacy_esc_[index].data;
    out.valid = out.valid && fresh(out.stamp_ns, now);
  } else if (common.seen) {out = common.data; out.valid = false;}
  if (!common.seen && info_[index].seen && info_[index].valid && in_count &&
    fresh(info_[index].stamp_ns, now) &&
    !(index < 12 && legacy_esc_[index].count))
  {
    const auto & information = info_[index];
    out.source = EscSource::Common;
    out.metadata_stamp_ns = information.stamp_ns;
    out.temperature_valid = information.temperature != std::numeric_limits<int16_t>::max();
    if (out.temperature_valid) {
      out.temperature = static_cast<float>(information.temperature) * 0.01F;
    }
    out.failure_flags = information.failure_flags; out.error_count = information.error_count;
    out.failure_flags_valid = out.error_count_valid = true;
    return out;  // valid/rpm/current remain false: this is INFO only.
  }
  if (!out.valid) {
    out.rpm_valid = out.voltage_valid = out.current_valid = out.temperature_valid = false;
    out.failure_flags_valid = out.error_count_valid = false;
    out.totalcurrent_valid = out.count_valid = false;
  }
  return out;
}
}  // namespace mavros_esc_wheel_odometry
