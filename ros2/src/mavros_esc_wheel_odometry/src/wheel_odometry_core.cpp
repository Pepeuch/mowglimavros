#include "mavros_esc_wheel_odometry/wheel_odometry_core.hpp"
#include <cmath>

namespace mavros_esc_wheel_odometry
{
double rpm_to_mps(double rpm, double radius_m)
{
  constexpr double kTwoPi = 6.28318530717958647692;
  return rpm * kTwoPi * radius_m / 60.0;
}
WheelOdometryCore::WheelOdometryCore(WheelGeometry geometry)
: geometry_(geometry) {}
bool WheelOdometryCore::valid() const
{
  return std::isfinite(geometry_.track_width_m) && geometry_.track_width_m > 0.0;
}
void WheelOdometryCore::reset() {last_sample_ns_ = last_receipt_ns_ = 0;}
std::optional<WheelObservation> WheelOdometryCore::receive_motion(
  const WheelMotionObservation & motion)
{
  if (!valid() || !motion.valid || motion.source == WheelSource::None ||
    !std::isfinite(motion.left_mps) || !std::isfinite(motion.right_mps) ||
    motion.sample_stamp_ns <= last_sample_ns_ || motion.receipt_stamp_ns <= 0 ||
    motion.receipt_stamp_ns < last_receipt_ns_)
  {
    return std::nullopt;
  }
  const double linear = (motion.left_mps + motion.right_mps) / 2.0;
  const double angular = (motion.right_mps - motion.left_mps) / geometry_.track_width_m;
  if (!std::isfinite(linear) || !std::isfinite(angular)) {return std::nullopt;}
  last_sample_ns_ = motion.sample_stamp_ns; last_receipt_ns_ = motion.receipt_stamp_ns;
  return WheelObservation{motion.sample_stamp_ns, motion.receipt_stamp_ns, linear, angular};
}
}  // namespace mavros_esc_wheel_odometry
