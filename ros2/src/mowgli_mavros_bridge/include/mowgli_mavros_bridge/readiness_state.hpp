#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace mowgli_mavros_bridge
{
struct Readiness
{
  bool connected;
  bool imu_fresh;
  bool gnss_fresh;
  bool gnss_valid;
  bool wheel_fresh;
  bool traction_fresh;
  bool traction_valid;
  bool ready;
};

class ReadinessState
{
public:
  explicit ReadinessState(double timeout_s, bool gnss_required = true,
                          bool wheel_odometry_required = false);
  void connection(bool connected);
  void imu(int64_t receipt_ns);
  void gnss(int64_t receipt_ns, uint64_t sequence, const std::string& incarnation, bool valid);
  void wheel(int64_t receipt_ns);
  void traction(int64_t receipt_ns, bool valid);
  Readiness project(int64_t now_ns) const;
  int64_t status_stamp_ns() const;

private:
  bool fresh(const std::optional<int64_t>& stamp, int64_t now_ns) const;
  int64_t timeout_ns_;
  bool gnss_required_;
  bool wheel_odometry_required_;
  int64_t last_status_ns_{0};
  bool connected_{false};
  std::optional<int64_t> imu_stamp_, gnss_stamp_, wheel_stamp_, traction_stamp_;
  bool gnss_valid_{false};
  bool traction_valid_{false};
  uint64_t gnss_sequence_{0};
  std::string gnss_incarnation_;
};
}  // namespace mowgli_mavros_bridge
