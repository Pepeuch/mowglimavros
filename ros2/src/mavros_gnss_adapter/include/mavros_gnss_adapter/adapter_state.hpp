#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "sensor_msgs/msg/nav_sat_fix.hpp"
#include "mavros_gnss_adapter/status_mapping.hpp"

namespace mavros_gnss_adapter
{
inline bool canonical_enabled(const std::string & mode, const std::string & source)
{
  if ((mode != "mavros" && mode != "direct") || (source != "gps1" && source != "gps2")) {
    throw std::invalid_argument("GNSS_SOURCE must be direct|mavros; GNSS_MAVROS_SOURCE gps1|gps2");
  }
  return mode == "mavros";
}

inline int64_t stamp_ns(const builtin_interfaces::msg::Time & stamp)
{
  return int64_t(stamp.sec) * 1000000000LL + stamp.nanosec;
}

// No fix/RTK interpretation here: the raw side channel supplies only ellipsoid
// altitude and the position tuple needed to correlate it to the private fix.
struct AltitudeSample
{
  int64_t receipt_ros_ns;
  int32_t latitude_e7;
  int32_t longitude_e7;
  int32_t altitude_msl_mm;
  std::optional<double> ellipsoid_m;
};

class AdapterState
{
public:
  explicit AdapterState(std::string source, int64_t timeout_ns = 3000000000LL)
  : source_(std::move(source)), timeout_ns_(timeout_ns)
  {
    if (timeout_ns <= 0) {throw std::invalid_argument("GNSS timeout must be positive");}
  }

  Status observe_status(const UniversalStatus & input, int64_t steady_ns)
  {
    const auto identity = input.source_id + "/" + input.source_incarnation;
    if (identity != identity_) {
      // Preserve already queued current packets at startup; on replacement no
      // pending fix/altitude from the previous incarnation may survive.
      if (!identity_.empty()) {pending_.clear(); altitudes_.clear(); accepted_stamps_.clear();}
      identity_ = identity;
      sequence_ = 0;
      position_receipt_.reset();
      position_stamp_ = builtin_interfaces::msg::Time{};
      last_fix_stamp_ = 0;
    }
    if (input.position_observation_sequence < sequence_) {
      invalidate();
      return status_;
    }
    if (input.position_observation_sequence != sequence_) {
      sequence_ = input.position_observation_sequence;
      position_stamp_ = input.stamp;
      position_receipt_ = steady_ns;
      accepted_stamps_.push_back(stamp_ns(input.stamp));
      if (accepted_stamps_.size() > 32) {accepted_stamps_.pop_front();}
    }
    status_ = map_status(input, source_);
    status_.header.stamp = position_stamp_;  // RTK enrichment is not a new position.
    if (!fresh(steady_ns)) {invalidate();}
    if (!status_.fix_valid) {accepted_stamps_.clear(); pending_.clear();}
    return status_;
  }

  std::optional<Status> expire(int64_t steady_ns)
  {
    if (!fresh(steady_ns) && status_.fix_valid) {
      invalidate();
      pending_.clear();
      altitudes_.clear();
      return status_;
    }
    return std::nullopt;
  }

  void observe_altitude(const AltitudeSample & sample)
  {
    altitudes_.push_back(sample);
    if (altitudes_.size() > 32) {altitudes_.pop_front();}
  }

  void observe_fix(const sensor_msgs::msg::NavSatFix & fix, int64_t steady_ns)
  {
    pending_.push_back({fix, steady_ns});
    if (pending_.size() > 32) {pending_.pop_front();}
  }

  std::vector<sensor_msgs::msg::NavSatFix> take_fixes(int64_t steady_ns)
  {
    std::vector<sensor_msgs::msg::NavSatFix> output;
    while (!pending_.empty() && steady_ns - pending_.front().receipt_ns >= kDeliveryWaitNs) {
      auto fix = pending_.front().fix;
      pending_.pop_front();
      const auto stamp = stamp_ns(fix.header.stamp);
      const bool accepted = std::find(
        accepted_stamps_.begin(), accepted_stamps_.end(), stamp) != accepted_stamps_.end();
      if (!fresh(steady_ns) || !status_.fix_valid || !accepted || stamp <= last_fix_stamp_ ||
        fix.status.status == sensor_msgs::msg::NavSatStatus::STATUS_NO_FIX)
      {
        continue;
      }
      // UG and this raw callback timestamp the same MAVROS dispatch locally.
      // Require a unique tuple/time match, never reuse the latest cached height.
      auto match = altitudes_.end();
      size_t matches = 0;
      for (auto it = altitudes_.begin(); it != altitudes_.end(); ++it) {
        if (std::abs(it->receipt_ros_ns - stamp) <= kPairToleranceNs &&
          std::abs(fix.latitude - it->latitude_e7 / 1e7) < 0.5e-7 &&
          std::abs(fix.longitude - it->longitude_e7 / 1e7) < 0.5e-7 &&
          std::abs(fix.altitude - it->altitude_msl_mm / 1000.0) < 0.0005)
        {
          match = it;
          ++matches;
        }
      }
      fix.altitude = std::numeric_limits<double>::quiet_NaN();
      if (matches == 1) {
        fix.altitude = match->ellipsoid_m.value_or(std::numeric_limits<double>::quiet_NaN());
        altitudes_.erase(match);
      }
      fix.header.frame_id = "gps_link";
      last_fix_stamp_ = stamp;
      output.push_back(fix);
    }
    return output;
  }

private:
  bool fresh(int64_t now) const
  {
    return sequence_ != 0 && position_receipt_ && now >= *position_receipt_ &&
           now - *position_receipt_ <= timeout_ns_;
  }

  void invalidate()
  {
    status_.fix_valid = false;
    status_.fix_type = Status::FIX_TYPE_NO_FIX;
    status_.rtk_mode = Status::RTK_MODE_UNKNOWN;
    status_.value_flags = 0;
    status_.differential_corrections = false;
    status_.corrections_active = false;
    status_.dead_reckoning = false;
  }

  struct PendingFix {sensor_msgs::msg::NavSatFix fix; int64_t receipt_ns;};
  static constexpr int64_t kPairToleranceNs = 20000000LL;
  static constexpr int64_t kDeliveryWaitNs = 100000000LL;
  std::string source_;
  int64_t timeout_ns_;
  std::string identity_;
  uint64_t sequence_{0};
  builtin_interfaces::msg::Time position_stamp_{};
  std::optional<int64_t> position_receipt_;
  int64_t last_fix_stamp_{0};
  Status status_{};
  std::deque<int64_t> accepted_stamps_;
  std::deque<AltitudeSample> altitudes_;
  std::deque<PendingFix> pending_;
};
}  // namespace mavros_gnss_adapter
