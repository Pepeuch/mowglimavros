#include "mavros_esc_wheel_odometry/motor_tick_integrator.hpp"
#include <algorithm>
#include <cmath>

namespace mavros_esc_wheel_odometry
{
bool MotorTickIntegrator::observe(unsigned wheel, double rpm, int64_t sample, int64_t receipt)
{
  if (wheel >= wheels_.size() || !std::isfinite(rpm) || sample <= 0 || receipt <= 0 ||
    timeout_ns_ <= 0) {return false;}
  auto & old = wheels_[wheel];
  if (old.valid && (sample <= old.sample_ns || receipt < old.receipt_ns)) {return false;}
  const double rate = rpm / 60.0;
  double ticks = old.ticks;
  const bool contiguous = old.valid && sample - old.sample_ns <= timeout_ns_ &&
    receipt - old.receipt_ns <= timeout_ns_;
  if (contiguous) {
    // Integrate each motor independently with its own measurement interval.
    const double average_rate = old.rpm / 120.0 + rpm / 120.0;
    ticks += average_rate * (static_cast<double>(sample - old.sample_ns) * 1e-9);
  }
  if (!std::isfinite(ticks) || !std::isfinite(rate)) {return false;}
  old = Wheel{ticks, rpm, rate, sample, receipt, true, old.segment + (contiguous ? 0 : 1)};
  return true;
}
void MotorTickIntegrator::reset() {wheels_ = {}; ++epoch_;}
void MotorTickIntegrator::retain_counts(const MotorTickIntegrator & previous)
{wheels_ = previous.wheels_; epoch_ = previous.epoch_;}
void MotorTickIntegrator::invalidate(unsigned wheel)
{if (wheel < wheels_.size()) {wheels_[wheel].valid = false;}}
void MotorTickIntegrator::reset_references()
{
  for (auto & wheel : wheels_) {
    wheel.valid = false; wheel.sample_ns = wheel.receipt_ns = 0;
  }
}
MotorTickState MotorTickIntegrator::state() const
{
  return {wheels_[0].ticks, wheels_[1].ticks, wheels_[0].sample_ns, wheels_[1].sample_ns,
    wheels_[0].receipt_ns, wheels_[1].receipt_ns, wheels_[0].valid, wheels_[1].valid,
    epoch_, wheels_[0].segment, wheels_[1].segment};
}
std::optional<WheelMotionObservation> MotorTickIntegrator::motion(
  double tpm,
  WheelSource source) const
{
  if (!std::isfinite(tpm) || tpm <= 0 || !wheels_[0].valid || !wheels_[1].valid) {
    return std::nullopt;
  }
  const double left = wheels_[0].ticks_per_second / tpm;
  const double right = wheels_[1].ticks_per_second / tpm;
  if (!std::isfinite(left) || !std::isfinite(right)) {return std::nullopt;}
  return WheelMotionObservation{left, right, std::max(wheels_[0].sample_ns, wheels_[1].sample_ns),
    std::max(wheels_[0].receipt_ns, wheels_[1].receipt_ns), source, true};
}
}  // namespace mavros_esc_wheel_odometry
