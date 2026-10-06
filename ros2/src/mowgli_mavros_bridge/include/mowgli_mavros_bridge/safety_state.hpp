#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace mowgli_mavros_bridge
{

enum class HardwareSafetyState
{
  Unknown,
  Released,
  Engaged,
};

struct EmergencyState
{
  bool active_emergency{false};
  bool latched_emergency{false};
  bool lift_warning{false};
  float lift_duration_sec{0.0F};
  std::string reason{"NONE"};
};

class SafetyState
{
public:
  void observe_motor_outputs(bool enabled)
  {
    hardware_safety_state_ = enabled ? HardwareSafetyState::Released :
      HardwareSafetyState::Engaged;
    if (!enabled) {
      hardware_emergency_latched_ = true;
    }
  }

  void observe_button_change(uint8_t state, int64_t receipt_ns)
  {
    const bool was_lifted = wheel_lift_state_valid_ &&
      (left_wheel_lifted_ || right_wheel_lifted_);
    raw_button_state_ = state;
    left_wheel_lifted_ = (state & 0x01U) != 0U;
    right_wheel_lifted_ = (state & 0x02U) != 0U;
    wheel_lift_state_valid_ = true;
    last_button_change_ns_ = receipt_ns;

    const bool lifted = left_wheel_lifted_ || right_wheel_lifted_;
    if (lifted && !was_lifted) {
      lift_started_ns_ = receipt_ns;
    } else if (!lifted) {
      lift_started_ns_ = 0;
    }
  }

  void set_service_emergency(bool active)
  {
    service_emergency_active_ = active;
    if (active) {
      service_emergency_latched_ = true;
    }
  }

  void disconnect()
  {
    hardware_safety_state_ = HardwareSafetyState::Unknown;
    wheel_lift_state_valid_ = false;
    lift_started_ns_ = 0;
  }

  EmergencyState project(int64_t now_ns, bool wheel_lift_safety_enabled) const
  {
    EmergencyState result;
    result.latched_emergency =
      hardware_emergency_latched_ || service_emergency_latched_;
    unsigned lifted_count = 0U;
    if (wheel_lift_safety_enabled && wheel_lift_state_valid_) {
      lifted_count = static_cast<unsigned>(left_wheel_lifted_) +
        static_cast<unsigned>(right_wheel_lifted_);
      result.lift_warning = lifted_count == 1U;
    }

    if (hardware_safety_state_ == HardwareSafetyState::Engaged) {
      result.active_emergency = true;
      result.latched_emergency = true;
      result.reason = "HARDWARE_SAFETY_SWITCH";
    } else if (service_emergency_active_) {
      result.active_emergency = true;
      result.reason = "SERVICE_EMERGENCY_STOP";
    } else if (lifted_count > 0U) {
      result.active_emergency = lifted_count == 2U;
      result.reason = "WHEEL_LIFT";
    }

    if (wheel_lift_safety_enabled && wheel_lift_state_valid_ && lift_started_ns_ > 0 &&
      now_ns >= lift_started_ns_)
    {
      result.lift_duration_sec = static_cast<float>(
        static_cast<double>(now_ns - lift_started_ns_) / 1.0e9);
    }
    return result;
  }

  HardwareSafetyState hardware_safety_state() const {return hardware_safety_state_;}
  bool wheel_lift_state_valid() const {return wheel_lift_state_valid_;}
  bool left_wheel_lifted() const {return left_wheel_lifted_;}
  bool right_wheel_lifted() const {return right_wheel_lifted_;}
  uint8_t raw_button_state() const {return raw_button_state_;}
  int64_t last_button_change_ns() const {return last_button_change_ns_;}

private:
  HardwareSafetyState hardware_safety_state_{HardwareSafetyState::Unknown};
  bool hardware_emergency_latched_{false};
  bool service_emergency_active_{false};
  bool service_emergency_latched_{false};

  bool wheel_lift_state_valid_{false};
  bool left_wheel_lifted_{false};
  bool right_wheel_lifted_{false};
  uint8_t raw_button_state_{0};
  int64_t last_button_change_ns_{0};
  int64_t lift_started_ns_{0};
};

}  // namespace mowgli_mavros_bridge
