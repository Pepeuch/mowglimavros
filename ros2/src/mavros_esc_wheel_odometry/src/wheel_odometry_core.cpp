#include "mavros_esc_wheel_odometry/wheel_odometry_core.hpp"

#include <cmath>

namespace mavros_esc_wheel_odometry
{

namespace
{
constexpr double kTwoPi = 6.28318530717958647692;
}

WheelOdometryCore::WheelOdometryCore(WheelGeometry geometry)
: geometry_(geometry)
{
}

bool WheelOdometryCore::counter_advanced(uint16_t previous, uint16_t current)
{
  return static_cast<uint16_t>(current - previous) != 0U;
}

bool WheelOdometryCore::valid() const
{
  return geometry_.left_esc_slot >= 0 && geometry_.left_esc_slot < 12 &&
         geometry_.right_esc_slot >= 0 && geometry_.right_esc_slot < 12 &&
         geometry_.left_esc_slot != geometry_.right_esc_slot &&
         geometry_.left_radius_m > 0.0 && geometry_.right_radius_m > 0.0 &&
         geometry_.track_width_m > 0.0;
}

void WheelOdometryCore::reset()
{
  left_count_.reset();
  right_count_.reset();
  left_advanced_ = false;
  right_advanced_ = false;
}

void WheelOdometryCore::receive_count_for_slot(int slot, uint16_t count)
{
  std::optional<uint16_t> * baseline = nullptr;
  bool * advanced = nullptr;
  if (slot == geometry_.left_esc_slot) {
    baseline = &left_count_;
    advanced = &left_advanced_;
  } else if (slot == geometry_.right_esc_slot) {
    baseline = &right_count_;
    advanced = &right_advanced_;
  } else {
    return;
  }

  if (!baseline->has_value()) {
    *baseline = count;
    return;
  }
  if (counter_advanced(**baseline, count)) {
    *baseline = count;
    *advanced = true;
  }
}

void WheelOdometryCore::receive_esc_counts(
  int group_offset, const std::array<uint16_t, 4> & counts)
{
  for (int index = 0; index < static_cast<int>(counts.size()); ++index) {
    receive_count_for_slot(group_offset + index, counts[static_cast<size_t>(index)]);
  }
}

std::optional<WheelObservation> WheelOdometryCore::receive_rpm(
  double left_wheel_rpm, double right_wheel_rpm, int64_t receipt_stamp_ns)
{
  if (!valid() || !left_advanced_ || !right_advanced_) {
    return std::nullopt;
  }

  const double left_mps = left_wheel_rpm * kTwoPi * geometry_.left_radius_m / 60.0;
  const double right_mps = right_wheel_rpm * kTwoPi * geometry_.right_radius_m / 60.0;
  WheelObservation observation;
  observation.receipt_stamp_ns = receipt_stamp_ns;
  observation.linear_x_mps = (left_mps + right_mps) / 2.0;
  observation.angular_z_rps = (right_mps - left_mps) / geometry_.track_width_m;

  left_advanced_ = false;
  right_advanced_ = false;
  return observation;
}

}  // namespace mavros_esc_wheel_odometry
