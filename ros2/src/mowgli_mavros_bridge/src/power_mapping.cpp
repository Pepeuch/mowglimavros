#include "mowgli_mavros_bridge/power_mapping.hpp"

#include <cmath>
#include <limits>

namespace mowgli_mavros_bridge
{
namespace
{
constexpr uint8_t kMavBatteryChargeStateCharging = 6;
}

PowerMapping::PowerMapping(int dock, int traction, double stale)
    : dock_instance_(dock), traction_instance_(traction),
      stale_after_ns_(static_cast<int64_t>(stale * 1e9))
{
}

bool PowerMapping::valid() const
{
  return dock_instance_ >= 0 && dock_instance_ <= 255 && traction_instance_ >= 0 &&
         traction_instance_ <= 255 && dock_instance_ != traction_instance_ &&
         stale_after_ns_ > 0;
}

void PowerMapping::observe(const BatteryInput& input)
{
  if (input.instance == dock_instance_)
  {
    dock_ = input;
  }
  else if (input.instance == traction_instance_)
  {
    traction_ = input;
  }
}

void PowerMapping::reset()
{
  dock_.reset();
  traction_.reset();
}

bool PowerMapping::fresh(const std::optional<BatteryInput>& input, int64_t now_ns) const
{
  return input && now_ns >= input->receipt_ns &&
         now_ns - input->receipt_ns <= stale_after_ns_;
}

PowerProjection PowerMapping::project(int64_t now_ns) const
{
  const double nan = std::numeric_limits<double>::quiet_NaN();
  PowerProjection out{nan, nan, nan, nan, nan, nan, false,
                      fresh(traction_, now_ns), fresh(dock_, now_ns)};
  if (out.traction_fresh)
  {
    out.v_battery = traction_->voltage.value_or(nan);
    out.traction_percentage = traction_->percentage.value_or(nan);
    // MAVLink positive current is discharge; Mowgli battery current is
    // negative while the robot consumes power.
    if (traction_->current)
    {
      out.traction_current = -*traction_->current;
    }
  }
  if (out.dock_fresh)
  {
    out.v_charge = dock_->voltage.value_or(nan);
    if (dock_->current)
    {
      out.charge_current = -*dock_->current;
    }
    out.charger_enabled = dock_->charge_state == kMavBatteryChargeStateCharging;
  }
  // Net current is only known when both physical paths have valid samples.
  if (std::isfinite(out.charge_current) && std::isfinite(out.traction_current))
  {
    out.battery_net_current = out.charge_current + out.traction_current;
  }
  return out;
}
}  // namespace mowgli_mavros_bridge
