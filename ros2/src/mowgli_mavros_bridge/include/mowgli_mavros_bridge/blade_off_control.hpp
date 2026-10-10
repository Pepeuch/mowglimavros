#pragma once

#include <cstdint>
#include <stdexcept>

namespace mowgli_mavros_bridge
{

// OFF-only preparation patch. No ARM, direction inference or ON command.
class BladeOffControl
{
public:
  enum class Decision
  {
    SendNeutral,
    AlreadyAccepted,
    Failed,
    OnInhibited
  };
  struct Command
  {
    uint16_t id{183};  // MAV_CMD_DO_SET_SERVO
    float channel;
    float pwm;
  };

  explicit BladeOffControl(int64_t channel = 3, int64_t neutral_pwm = 1500)
      : channel_(channel), neutral_pwm_(neutral_pwm)
  {
    if (channel < 1 || channel > 32 || neutral_pwm < 1000 || neutral_pwm > 2000)
    {
      throw std::invalid_argument("Invalid blade servo channel or neutral PWM");
    }
  }

  Decision decide(bool enabled) const
  {
    if (enabled)
    {
      return Decision::OnInhibited;
    }
    if (state_ == State::Failed)
    {
      return Decision::Failed;
    }
    return state_ == State::Idle ? Decision::SendNeutral : Decision::AlreadyAccepted;
  }

  uint64_t begin()
  {
    state_ = State::Pending;
    return generation_;
  }

  void complete(uint64_t generation, bool accepted)
  {
    if (generation == generation_ && state_ == State::Pending)
    {
      state_ = accepted ? State::Confirmed : State::Failed;
    }
  }

  void reset_connection()
  {
    ++generation_;
    state_ = State::Idle;
  }

  Command command() const
  {
    return {183, static_cast<float>(channel_), static_cast<float>(neutral_pwm_)};
  }

private:
  enum class State
  {
    Idle,
    Pending,
    Confirmed,
    Failed
  };
  int64_t channel_, neutral_pwm_;
  uint64_t generation_{0};
  State state_{State::Idle};
};

}  // namespace mowgli_mavros_bridge
