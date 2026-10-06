#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

#include "mavros_esc_wheel_odometry/motor_tick_integrator.hpp"

namespace mavros_esc_wheel_odometry {
// Encodes fractional motor revolutions into the unsigned magnitude + direction
// convention of mowgli_interfaces/WheelTick. This is an integer transport
// scale, not an installation calibration: metric conversion remains entirely
// controlled by ticks_per_meter.
struct WheelTickProjection {
  static constexpr double kCountsPerMotorRevolution = 1000.0;
  bool left_valid{false}, right_valid{false};
  uint32_t left_count{0}, right_count{0};
  uint8_t left_direction{1}, right_direction{1};
  static float factor(double ticks_per_meter) {
    const double scaled = ticks_per_meter * kCountsPerMotorRevolution;
    return std::isfinite(scaled) && scaled > 0 &&
                   scaled <= std::numeric_limits<float>::max()
               ? static_cast<float>(scaled)
               : 0.0F;
  }
};

class WheelTickProjector {
public:
  std::optional<WheelTickProjection> project(WheelSource source,
                                             const MotorTickState &ticks) {
    if ((source != WheelSource::EscStatus &&
         source != WheelSource::ArduPilotLegacy) ||
        (!ticks.left_valid && !ticks.right_valid)) {
      left_.valid = right_.valid = false;
      return std::nullopt;
    }
    if (source_ != source || epoch_ != ticks.epoch) {
      source_ = source;
      epoch_ = ticks.epoch;
      reset_references();
    }
    const bool left_new =
        project_wheel(ticks.left_valid, ticks.left_segment, ticks.left_ticks,
                      ticks.left_sample_ns, left_);
    const bool right_new =
        project_wheel(ticks.right_valid, ticks.right_segment, ticks.right_ticks,
                      ticks.right_sample_ns, right_);
    if (!left_new && !right_new) {
      return std::nullopt;
    }
    return WheelTickProjection{left_.valid,  right_.valid,    left_.count,
                               right_.count, left_.direction, right_.direction};
  }

private:
  struct Wheel {
    bool valid{false};
    uint64_t segment{0};
    int64_t sample_ns{0};
    double ticks{0}, fractional_count{0};
    uint32_t count{0};
    uint8_t direction{1};
  };

  void reset_references() {
    left_.valid = right_.valid = false;
    left_.fractional_count = right_.fractional_count = 0;
  }

  static bool project_wheel(bool valid, uint64_t segment, double ticks,
                            int64_t sample_ns, Wheel &wheel) {
    if (!valid || !std::isfinite(ticks)) {
      wheel.valid = false;
      return false;
    }
    if (!wheel.valid || wheel.segment != segment) {
      wheel.valid = true;
      wheel.segment = segment;
      wheel.ticks = ticks;
      wheel.sample_ns = sample_ns;
      wheel.fractional_count = 0;
      return true;
    }
    if (sample_ns <= wheel.sample_ns) {
      return false;
    }
    wheel.sample_ns = sample_ns;
    const double delta = ticks - wheel.ticks;
    wheel.ticks = ticks;
    if (!std::isfinite(delta) ||
        std::abs(delta) * WheelTickProjection::kCountsPerMotorRevolution >
            std::numeric_limits<int32_t>::max()) {
      wheel.valid = false;
      return false;
    }
    if ((delta > 0 && wheel.direction == 0) ||
        (delta < 0 && wheel.direction == 1)) {
      wheel.fractional_count = 0;
    }
    if (delta > 0) {
      wheel.direction = 1;
    } else if (delta < 0) {
      wheel.direction = 0;
    }
    wheel.fractional_count +=
        std::abs(delta) * WheelTickProjection::kCountsPerMotorRevolution;
    const auto whole_count =
        static_cast<uint32_t>(std::floor(wheel.fractional_count));
    wheel.count += whole_count;
    wheel.fractional_count -= whole_count;
    return true;
  }

  WheelSource source_{WheelSource::None};
  uint64_t epoch_{0};
  Wheel left_;
  Wheel right_;
};
} // namespace mavros_esc_wheel_odometry
