#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "mavros_esc_wheel_odometry/wheel_odometry_core.hpp"

namespace mavros_esc_wheel_odometry
{
// Unit convention, not a physical calibration: one fractional tick = one motor revolution.
struct MotorRpmObservation
{
  double left_rpm{0}, right_rpm{0};
  int64_t sample_stamp_ns{0}, receipt_stamp_ns{0};
};
struct MotorTickState
{
  double left_ticks{0}, right_ticks{0};
  int64_t left_sample_ns{0}, right_sample_ns{0};
  int64_t left_receipt_ns{0}, right_receipt_ns{0};
  bool left_valid{false}, right_valid{false};
  uint64_t epoch{0}, left_segment{0}, right_segment{0};
};
class MotorTickIntegrator
{
public:
  explicit MotorTickIntegrator(int64_t timeout_ns = 3000000000LL)
  : timeout_ns_(timeout_ns) {}
  bool observe(unsigned wheel, double raw_rpm, int64_t sample_ns, int64_t receipt_ns);
  void reset();
  void reset_references();
  void invalidate(unsigned wheel);
  void retain_counts(const MotorTickIntegrator & previous);
  MotorTickState state() const;
  std::optional<WheelMotionObservation> motion(double ticks_per_meter, WheelSource source) const;

private:
  struct Wheel
  {
    double ticks{0}, rpm{0}, ticks_per_second{0};
    int64_t sample_ns{0}, receipt_ns{0};
    bool valid{false};
    uint64_t segment{0};
  };
  int64_t timeout_ns_;
  uint64_t epoch_{0};
  std::array<Wheel, 2> wheels_{};
};
}  // namespace mavros_esc_wheel_odometry
