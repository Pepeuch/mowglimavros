#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace mavros_esc_wheel_odometry
{

struct WheelGeometry
{
  int left_esc_slot{-1};
  int right_esc_slot{-1};
  double left_radius_m{0.0};
  double right_radius_m{0.0};
  double track_width_m{0.0};
};

struct WheelObservation
{
  int64_t receipt_stamp_ns{0};
  double linear_x_mps{0.0};
  double angular_z_rps{0.0};
};

/// Pairs signed MAVLink RPM observations with independent ESC telemetry counters.
/// All methods are deliberately ROS- and MAVROS-independent for deterministic tests.
class WheelOdometryCore
{
public:
  explicit WheelOdometryCore(WheelGeometry geometry);

  static bool counter_advanced(uint16_t previous, uint16_t current);
  bool valid() const;
  void reset();

  /// Accept one four-slot MAVLink ESC_TELEMETRY group at its zero-based wire offset.
  void receive_esc_counts(int group_offset, const std::array<uint16_t, 4> & counts);

  /// Return one fresh observation only after both selected counters progressed.
  std::optional<WheelObservation> receive_rpm(
    double left_wheel_rpm, double right_wheel_rpm, int64_t receipt_stamp_ns);

private:
  void receive_count_for_slot(int slot, uint16_t count);

  WheelGeometry geometry_;
  std::optional<uint16_t> left_count_;
  std::optional<uint16_t> right_count_;
  bool left_advanced_{false};
  bool right_advanced_{false};
};

}  // namespace mavros_esc_wheel_odometry
