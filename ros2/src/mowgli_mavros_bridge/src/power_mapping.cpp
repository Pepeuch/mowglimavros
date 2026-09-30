#include "mowgli_mavros_bridge/power_mapping.hpp"

#include <cmath>
#include <limits>

namespace mowgli_mavros_bridge
{
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
                      fresh(traction_, now_ns), fresh(dock_, now_ns), nan, nan, -1};
  if (out.traction_fresh)
  {
    out.v_battery = traction_->voltage.value_or(nan);
    out.traction_percentage = traction_->percentage.value_or(nan);
    // MAVLink positive current is discharge; Mowgli battery current is
    // negative while the robot consumes power.
    if (traction_->current)
    {
      out.traction_current_raw = *traction_->current;
      out.traction_current = -out.traction_current_raw;
    }
  }
  if (out.dock_fresh)
  {
    out.v_charge = dock_->voltage.value_or(nan);
    if (dock_->current)
    {
      out.dock_current_raw = *dock_->current;
    }
    out.dock_charge_state_raw = dock_->charge_state;
    // On this robot POWER1 voltage is present only when the dock is connected.
    // The FCU charge-state enum remains a raw diagnostic observation.
    out.charger_enabled = std::isfinite(out.v_charge) && out.v_charge > 0.0;
  }
  // Requested MowgliNext convention: POWER1 (dock/charger) minus POWER2
  // (robot consumption). An absent or stale path leaves the result unknown.
  if (std::isfinite(out.dock_current_raw) && std::isfinite(out.traction_current_raw))
  {
    out.charge_current = out.dock_current_raw - out.traction_current_raw;
  }
  // The extra diagnostic net-current field remains unvalidated until the
  // POWER1 monitor has been checked in all four physical dock states.
  return out;
}
}  // namespace mowgli_mavros_bridge
