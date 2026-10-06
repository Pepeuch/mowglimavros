#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include "mavros_esc_wheel_odometry/legacy_wheel_adapter.hpp"

namespace mavros_esc_wheel_odometry
{
enum class EscSource : uint8_t {Common = 1, ArduPilotLegacy = 2};
struct EscObservationData
{
  unsigned index{0};
  EscSource source{EscSource::Common};
  int64_t stamp_ns{0}, metadata_stamp_ns{0};
  bool valid{false};
  int32_t rpm{0};
  bool rpm_valid{false}, rpm_direction_valid{false};
  float voltage{0}, current{0}, temperature{0};
  bool voltage_valid{false}, current_valid{false}, temperature_valid{false};
  uint16_t failure_flags{0};
  uint32_t error_count{0};
  bool failure_flags_valid{false}, error_count_valid{false};
  float totalcurrent{0};
  uint16_t count{0};
  bool totalcurrent_valid{false}, count_valid{false};
};
struct CommonStatusPacket
{
  unsigned index{0};
  uint64_t time_us{0};
  std::array<int32_t, 4> rpm{};
  std::array<float, 4> voltage{}, current{};
  uint8_t component_id{1};
};
struct CommonInfoPacket
{
  unsigned index{0}, count{0};
  uint64_t time_us{0};
  std::array<int16_t, 4> temperature{};
  std::array<uint16_t, 4> failure_flags{};
  std::array<uint32_t, 4> error_count{};
  uint8_t component_id{1};
};
struct LegacyEscPacket
{
  unsigned index{0};
  std::array<uint16_t, 4> counts{}, rpm{};
  std::array<float, 4> voltage{}, current{}, temperature{}, totalcurrent{};
};
struct DistancePacket
{
  uint64_t time_us{0};
  unsigned count{0};
  std::array<double, 16> distance{};
  uint8_t component_id{0};
};
struct ObservationConfig
{
  WheelGeometry geometry{};
  std::string source{"auto"};
  int left_wheel_index{-1}, right_wheel_index{-1};
  int64_t timeout_ns{3000000000LL};
  bool legacy_enabled{false};
  int wheel_distance_component_id{-1};
  int esc_component_id{-1};
  double common_pair_max_skew_s{0.25};
  // New distance-source plausibility guard; legacy RPM behavior is unaffected.
  double max_distance_speed_mps{10.0};
};
const char * source_name(WheelSource source);

// Single normalizer/selector in the plugin's library, with no ROS or firmware branches.
class ObservationEngine
{
public:
  explicit ObservationEngine(ObservationConfig config);
  void connection(bool connected);
  // Wheel reconfiguration resets motion only; do not disrupt raw ESC reporting.
  void retain_esc_observations(
    const ObservationEngine & previous, int64_t configuration_ns,
    bool retain_legacy_ticks = true);
  void poll(int64_t now_ns);
  bool wheel_configured() const;
  MotorTickState motor_ticks(WheelSource source, int64_t now_ns) const;
  WheelSource active_source() const {return active_;}
  std::optional<WheelObservation> common_status(const CommonStatusPacket &, int64_t receipt_ns);
  void common_info(const CommonInfoPacket &, int64_t receipt_ns);
  void legacy_esc(const LegacyEscPacket &, int64_t receipt_ns);
  std::optional<WheelObservation> legacy_rpm(double left, double right, int64_t receipt_ns);
  std::optional<WheelObservation> wheel_distance(const DistancePacket &, int64_t receipt_ns);
  bool touched(unsigned index) const;
  EscObservationData esc(unsigned index, int64_t now_ns) const;

private:
  struct Status
  {
    EscObservationData data{};
    uint64_t time_us{0};
    bool seen{false};
  };
  struct Info
  {
    uint64_t time_us{0};
    int64_t stamp_ns{0};
    int16_t temperature{0};
    uint16_t failure_flags{0};
    uint32_t error_count{0};
    bool seen{false}, valid{false};
  };
  struct Legacy
  {
    EscObservationData data{};
    std::optional<uint16_t> count;
  };
  struct Distance
  {
    double left{0}, right{0};
    uint64_t time_us{0};
    int64_t receipt_ns{0};
    bool valid{false};
  };
  bool fresh(int64_t stamp, int64_t now) const;
  bool common_mapping() const;
  bool distance_mapping(unsigned count) const;
  bool common_available(int64_t now) const;
  void reset();
  void reset_common();
  void reset_distance();
  void receive_clock(int64_t receipt);
  bool accept_common_status(uint8_t component, int64_t receipt);
  bool accept_common_info(uint8_t component);
  void select(
    int64_t now, bool common_update = false,
    uint64_t previous_left = 0, uint64_t previous_right = 0);
  ObservationConfig config_;
  WheelOdometryCore core_;
  LegacyWheelAdapter legacy_;
  MotorTickIntegrator common_motor_ticks_, legacy_motor_ticks_;
  bool connected_{false};
  std::array<Status, 64> status_{};
  std::array<Info, 64> info_{};
  std::array<Legacy, 12> legacy_esc_{};
  std::optional<unsigned> common_count_;
  // Before STATUS acquisition this identifies candidate INFO only, not a lease.
  std::optional<uint8_t> common_component_;
  bool common_status_owner_{false};
  int64_t common_owner_stamp_ns_{0}, last_receipt_ns_{0};
  Distance distance_{};
  std::optional<uint8_t> distance_component_;
  std::optional<Distance> distance_baseline_;
  std::optional<WheelMotionObservation> legacy_motion_;
  WheelSource active_{WheelSource::None};
  uint64_t published_left_{0}, published_right_{0};
  bool common_epoch_baseline_needed_{false};
  int64_t common_configuration_ns_{0};
};
}  // namespace mavros_esc_wheel_odometry
