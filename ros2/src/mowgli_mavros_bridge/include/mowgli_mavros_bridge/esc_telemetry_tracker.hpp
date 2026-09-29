#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace mowgli_mavros_bridge
{

// ESC_TELEMETRY_1_TO_4 always carries four slots. Only the first three are
// wired on this robot; packet arrival alone does not refresh a slot.
struct EscSample
{
  int32_t rpm{0};
  float voltage{0.0F};
  float current{0.0F};
  float totalcurrent{0.0F};
  float temperature{0.0F};
  uint16_t count{0};
};

struct EscState
{
  EscSample sample{};
  bool observed{false};
  bool online{false};
  bool stale{false};
  int64_t age_ms{-1};
  int64_t last_update_ns{0};
};

class EscTelemetryTracker
{
public:
  explicit EscTelemetryTracker(double timeout_s = 3.0)
      : timeout_ns_(static_cast<int64_t>(timeout_s * 1e9))
  {
  }

  void reset()
  {
    states_ = {};
  }

  void observe(unsigned index, const EscSample& sample, int64_t receipt_ns)
  {
    if (index >= states_.size() || receipt_ns <= 0 || timeout_ns_ <= 0)
    {
      return;
    }
    auto& state = states_[index];
    // Count is the ESC telemetry packet counter, not the MAVLink frame count.
    // Unsigned subtraction accepts 65535 -> 0 without treating it as stale.
    const auto delta = static_cast<uint16_t>(sample.count - state.sample.count);
    if (!state.observed)
    {
      state.observed = sample.count != 0;
      if (state.observed)
      {
        state.last_update_ns = receipt_ns;
      }
    }
    else if (delta != 0)
    {
      state.last_update_ns = receipt_ns;
    }
    state.sample = sample;
  }

  EscState project(unsigned index, int64_t now_ns) const
  {
    if (index >= states_.size())
    {
      return {};
    }
    auto result = states_[index];
    if (!result.observed || result.last_update_ns <= 0)
    {
      return result;
    }
    result.age_ms = now_ns >= result.last_update_ns
                        ? (now_ns - result.last_update_ns) / 1000000LL
                        : -1;
    result.online = result.age_ms >= 0 && result.age_ms * 1000000LL <= timeout_ns_;
    result.stale = !result.online;
    return result;
  }

private:
  int64_t timeout_ns_;
  std::array<EscState, 3> states_{};
};

}  // namespace mowgli_mavros_bridge
