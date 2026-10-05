#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

#include "mowgli_mavros_bridge/esc_telemetry_tracker.hpp"

namespace mowgli_mavros_bridge
{

// Preserve the mower ESC status values already exposed by the MAVROS backend.
// MowgliNext treats this field as a legacy backend status; do not reinterpret
// wheel telemetry through it.  The blade freshness timestamp is the authority
// for whether the report is current.
constexpr uint8_t kMowerEscStatusUnavailable = 99U;
constexpr uint8_t kMowerEscStatusStopped = 200U;
constexpr uint8_t kMowerEscStatusRunning = 201U;

struct BladeTelemetryProjection
{
  uint8_t status{kMowerEscStatusUnavailable};
  float temperature{std::numeric_limits<float>::quiet_NaN()};
  float current{0.0F};
  float rpm{0.0F};
  int64_t stamp_ns{0};
  bool available{false};
};

inline BladeTelemetryProjection blade_telemetry_from_esc(const EscState & mower)
{
  BladeTelemetryProjection out;
  if (!mower.online || !mower.observed || mower.last_update_ns <= 0 ||
    !mower.sample.rpm_valid || !mower.sample.current_valid)
  {
    return out;
  }

  out.available = true;
  out.status = mower.sample.rpm == 0 ? kMowerEscStatusStopped : kMowerEscStatusRunning;
  // Public Status has no per-field validity flag; user-approved unknown marker.
  out.temperature = mower.sample.temperature_valid ? mower.sample.temperature :
    std::numeric_limits<float>::quiet_NaN();
  out.current = mower.sample.current;
  // Blade projection uses magnitude; wheel motion is not transported here.
  out.rpm = static_cast<float>(std::abs(static_cast<double>(mower.sample.rpm)));
  out.stamp_ns = mower.last_update_ns;
  return out;
}

}  // namespace mowgli_mavros_bridge
