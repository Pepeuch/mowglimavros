#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "mavros_esc_wheel_odometry/motor_tick_integrator.hpp"

namespace mavros_esc_wheel_odometry
{
// Only RPM #226 supplies direction. Unsigned ESC_TELEMETRY RPM is not an input.
class LegacyWheelAdapter
{
public:
  explicit LegacyWheelAdapter(WheelGeometry geometry, int64_t timeout_ns = 3000000000LL);
  static bool counter_advanced(uint16_t previous, uint16_t current);
  bool valid() const;
  void reset();
  void receive_esc_counts(
    int group_offset, const std::array<uint16_t, 4> & counts,
    int64_t receipt_ns);
  std::optional<MotorRpmObservation> receive_rpm(
    double left_rpm, double right_rpm,
    int64_t receipt_ns);

private:
  WheelGeometry geometry_;
  int64_t timeout_ns_;
  std::array<std::optional<uint16_t>, 2> counts_{};
  std::array<bool, 2> advanced_{};
  std::array<int64_t, 2> stamps_{};
};
}  // namespace mavros_esc_wheel_odometry
