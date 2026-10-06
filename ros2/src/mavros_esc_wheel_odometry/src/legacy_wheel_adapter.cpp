#include "mavros_esc_wheel_odometry/legacy_wheel_adapter.hpp"
#include <cmath>
namespace mavros_esc_wheel_odometry
{
LegacyWheelAdapter::LegacyWheelAdapter(WheelGeometry geometry, int64_t timeout_ns)
: geometry_(geometry), timeout_ns_(timeout_ns) {}
bool LegacyWheelAdapter::counter_advanced(uint16_t previous, uint16_t current)
{return static_cast<uint16_t>(current - previous) != 0U;}
bool LegacyWheelAdapter::valid() const
{
  return geometry_.left_esc_slot >= 0 && geometry_.left_esc_slot < 12 &&
         geometry_.right_esc_slot >= 0 && geometry_.right_esc_slot < 12 &&
         geometry_.left_esc_slot != geometry_.right_esc_slot &&
         timeout_ns_ > 0;
}
void LegacyWheelAdapter::reset() {counts_ = {}; advanced_ = {}; stamps_ = {};}
void LegacyWheelAdapter::receive_esc_counts(
  int offset,
  const std::array<uint16_t, 4> & counts, int64_t receipt_ns)
{
  if ((offset != 0 && offset != 4 && offset != 8) || receipt_ns <= 0) {return;}
  const int slots[2] = {geometry_.left_esc_slot, geometry_.right_esc_slot};
  for (unsigned wheel = 0; wheel < 2; ++wheel) {
    const int index = slots[wheel] - offset;
    if (index < 0 || index >= 4) {continue;}
    const auto count = counts[static_cast<size_t>(index)];
    if (!counts_[wheel]) {
      counts_[wheel] = count;
    } else if (counter_advanced(*counts_[wheel], count) && receipt_ns >= stamps_[wheel]) {
      counts_[wheel] = count; advanced_[wheel] = true; stamps_[wheel] = receipt_ns;
    }
  }
}
std::optional<MotorRpmObservation> LegacyWheelAdapter::receive_rpm(
  double left, double right, int64_t receipt_ns)
{
  if (!valid() || !advanced_[0] || !advanced_[1] || receipt_ns <= 0 ||
    !std::isfinite(left) || !std::isfinite(right)) {return std::nullopt;}
  for (auto stamp : stamps_) {
    if (stamp <= 0 || receipt_ns < stamp || receipt_ns - stamp > timeout_ns_) {
      advanced_ = {}; return std::nullopt;
    }
  }
  advanced_ = {};
  return MotorRpmObservation{left, right, receipt_ns, receipt_ns};
}
}  // namespace mavros_esc_wheel_odometry
