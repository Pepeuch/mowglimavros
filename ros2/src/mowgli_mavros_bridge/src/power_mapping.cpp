#include "mowgli_mavros_bridge/power_mapping.hpp"
#include <cmath>
#include <limits>
namespace mowgli_mavros_bridge {
namespace { constexpr uint8_t kMavBatteryChargeStateCharging = 6; }
PowerMapping::PowerMapping(int dock, int traction, double stale) : dock_instance_(dock), traction_instance_(traction), stale_after_ns_(static_cast<int64_t>(stale * 1e9)) {}
bool PowerMapping::valid() const { return dock_instance_ >= 0 && dock_instance_ <= 255 && traction_instance_ >= 0 && traction_instance_ <= 255 && dock_instance_ != traction_instance_ && stale_after_ns_ > 0; }
void PowerMapping::observe(const BatteryInput & input) { if (input.instance == dock_instance_) dock_ = input; else if (input.instance == traction_instance_) traction_ = input; }
void PowerMapping::reset() { dock_.reset(); traction_.reset(); }
bool PowerMapping::fresh(const std::optional<BatteryInput> & input, int64_t now) const { return input && now >= input->receipt_ns && now - input->receipt_ns <= stale_after_ns_; }
PowerProjection PowerMapping::project(int64_t now) const { const double nan = std::numeric_limits<double>::quiet_NaN(); PowerProjection out{nan,nan,nan,false,fresh(traction_,now),fresh(dock_,now)}; if(out.traction_fresh) out.v_battery=traction_->voltage.value_or(nan); if(out.dock_fresh) { out.v_charge=dock_->voltage.value_or(nan); out.charge_current=dock_->current ? -*dock_->current : nan; out.charger_enabled=dock_->charge_state == kMavBatteryChargeStateCharging; } return out; }
bool PowerMapping::charging() const { return dock_ && dock_->charge_state == kMavBatteryChargeStateCharging; }
}  // namespace mowgli_mavros_bridge
