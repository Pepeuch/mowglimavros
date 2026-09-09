#pragma once
#include <cstdint>
#include <optional>
#include <string>
namespace mowgli_mavros_bridge {
struct Readiness { bool connected, gnss_fresh, gnss_valid, wheel_fresh, traction_fresh, traction_valid, ready; };
class ReadinessState {
public:
  explicit ReadinessState(double timeout_s);
  void connection(bool connected);
  void gnss(int64_t stamp_ns, uint64_t sequence, const std::string & incarnation, bool valid);
  void wheel(int64_t stamp_ns);
  void traction(int64_t stamp_ns, bool valid);
  Readiness project(int64_t now_ns) const;
  int64_t status_stamp_ns() const;
private:
  bool fresh(const std::optional<int64_t> & stamp, int64_t now_ns) const;
  int64_t timeout_ns_, last_status_ns_{0}; bool connected_{false};
  std::optional<int64_t> gnss_stamp_, wheel_stamp_, traction_stamp_;
  bool gnss_valid_{false}, traction_valid_{false}; uint64_t gnss_sequence_{0}; std::string gnss_incarnation_;
};
}  // namespace mowgli_mavros_bridge
