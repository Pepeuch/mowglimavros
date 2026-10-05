#pragma once

#include <cstdint>
#include <optional>

namespace mavros_esc_wheel_odometry
{
enum class WheelSource {None, WheelDistance, EscStatus, ArduPilotLegacy};

struct WheelGeometry
{
  int left_esc_slot{-1};
  int right_esc_slot{-1};
  double left_radius_m{0.0};
  double right_radius_m{0.0};
  double track_width_m{0.0};
};

struct WheelMotionObservation
{
  double left_mps{0.0};
  double right_mps{0.0};
  int64_t sample_stamp_ns{0};
  int64_t receipt_stamp_ns{0};
  WheelSource source{WheelSource::None};
  bool valid{false};
};

struct WheelObservation
{
  int64_t sample_stamp_ns{0};
  int64_t receipt_stamp_ns{0};
  double linear_x_mps{0.0};
  double angular_z_rps{0.0};
};

// One protocol-independent differential-drive equation for all source adapters.
class WheelOdometryCore
{
public:
  explicit WheelOdometryCore(WheelGeometry geometry);
  bool valid() const;
  void reset();
  std::optional<WheelObservation> receive_motion(const WheelMotionObservation & motion);

private:
  WheelGeometry geometry_;
  int64_t last_sample_ns_{0}, last_receipt_ns_{0};
};

// Shared unit conversion, not a second odometry equation.
double rpm_to_mps(double rpm, double radius_m);
}  // namespace mavros_esc_wheel_odometry
