#pragma once
#include <cstdint>
#include <optional>

namespace mowgli_mavros_bridge
{
struct BatteryInput { int instance; std::optional<double> voltage, current, percentage; uint8_t charge_state; int64_t receipt_ns; };
struct PowerProjection { double v_charge, v_battery, charge_current; bool charger_enabled; bool traction_fresh; bool dock_fresh; };
class PowerMapping {
public:
  PowerMapping(int dock_instance, int traction_instance, double stale_after_s);
  bool valid() const;
  void observe(const BatteryInput & input);
  void reset();
  PowerProjection project(int64_t now_ns) const;
  bool charging() const;
private:
  bool fresh(const std::optional<BatteryInput> & input, int64_t now_ns) const;
  int dock_instance_, traction_instance_; int64_t stale_after_ns_;
  std::optional<BatteryInput> dock_, traction_;
};
}  // namespace mowgli_mavros_bridge
