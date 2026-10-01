#pragma once

#include <cmath>
#include <cstdint>

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
  float temperature{0.0F};
  float current{0.0F};
  float rpm{0.0F};
  int64_t stamp_ns{0};
  bool available{false};
};

inline BladeTelemetryProjection blade_telemetry_from_esc2(const EscState & mower)
{
  BladeTelemetryProjection out;
  if (!mower.online || !mower.observed || mower.last_update_ns <= 0)
  {
    return out;
  }

  out.available = true;
  out.status = mower.sample.rpm == 0 ? kMowerEscStatusStopped : kMowerEscStatusRunning;
  out.temperature = mower.sample.temperature;
  out.current = mower.sample.current;
  // ESC_TELEMETRY carries an unsigned RPM magnitude in ArduPilot.  Blade load
  // control only needs the physical speed magnitude; wheel direction is handled
  // separately from signed MAVLink RPM and is intentionally not implemented here.
  out.rpm = static_cast<float>(std::abs(mower.sample.rpm));
  out.stamp_ns = mower.last_update_ns;
  return out;
}

}  // namespace mowgli_mavros_bridge
