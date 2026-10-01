#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace mavros_battery_observer
{

struct BatterySample
{
  std::optional<double> voltage;
  std::optional<double> current;
  int64_t receipt_ns{0};
};

struct PowerProjection
{
  double v_charge{std::numeric_limits<double>::quiet_NaN()};
  double v_battery{std::numeric_limits<double>::quiet_NaN()};
  double charge_current{std::numeric_limits<double>::quiet_NaN()};
  double battery_current{std::numeric_limits<double>::quiet_NaN()};
  bool charger_enabled{false};
  bool dock_fresh{false};
  bool battery_fresh{false};
};

class PowerState
{
public:
  PowerState(double stale_after_s, double charger_on_voltage, double charger_off_voltage)
  : stale_after_ns_(static_cast<int64_t>(stale_after_s * 1e9)),
    charger_on_voltage_(charger_on_voltage),
    charger_off_voltage_(charger_off_voltage)
  {
  }

  bool valid() const
  {
    return stale_after_ns_ > 0 && std::isfinite(charger_on_voltage_) &&
           std::isfinite(charger_off_voltage_) && charger_off_voltage_ >= 0.0 &&
           charger_on_voltage_ > charger_off_voltage_;
  }

  void observe_dock(const BatterySample & sample) { dock_ = sample; }
  void observe_battery(const BatterySample & sample) { battery_ = sample; }

  void reset()
  {
    dock_.reset();
    battery_.reset();
    charger_present_ = false;
  }

  PowerProjection project(int64_t now_ns) const
  {
    PowerProjection out;
    out.dock_fresh = fresh(dock_, now_ns);
    out.battery_fresh = fresh(battery_, now_ns);

    if (!out.dock_fresh)
    {
      // Never carry charger presence across stale/disconnected telemetry.
      // A new generation must cross the ON threshold again.
      charger_present_ = false;
    }
    else if (dock_->voltage)
    {
      out.v_charge = *dock_->voltage;
      if (std::isfinite(out.v_charge))
      {
        // Hysteresis prevents residual/backfeed voltages from looking like a
        // charger and avoids state chatter around the detection threshold.
        if (charger_present_)
        {
          if (out.v_charge < charger_off_voltage_)
          {
            charger_present_ = false;
          }
        }
        else if (out.v_charge > charger_on_voltage_)
        {
          charger_present_ = true;
        }
      }
      else
      {
        charger_present_ = false;
      }
      out.charger_enabled = charger_present_;
    }
    else
    {
      charger_present_ = false;
    }

    if (out.battery_fresh)
    {
      if (battery_->voltage)
      {
        out.v_battery = *battery_->voltage;
      }
      // MAVLink BATTERY_STATUS reports discharge as positive.  Mowgli's
      // battery current convention is positive into, negative out of battery.
      if (battery_->current)
      {
        out.battery_current = -*battery_->current;
      }
    }

    // POWER1 is charger current (positive into robot), POWER2 is battery
    // discharge current (positive on MAVLink, negative in Mowgli convention).
    // Net charge current therefore is POWER1 + signed POWER2.
    if (out.dock_fresh && out.battery_fresh && dock_->current && battery_->current)
    {
      out.charge_current = *dock_->current - *battery_->current;
    }

    return out;
  }

private:
  bool fresh(const std::optional<BatterySample> & sample, int64_t now_ns) const
  {
    return sample && now_ns >= sample->receipt_ns &&
           now_ns - sample->receipt_ns <= stale_after_ns_;
  }

  int64_t stale_after_ns_;
  double charger_on_voltage_;
  double charger_off_voltage_;
  mutable bool charger_present_{false};
  std::optional<BatterySample> dock_;
  std::optional<BatterySample> battery_;
};

}  // namespace mavros_battery_observer
