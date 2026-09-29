#include "mowgli_mavros_bridge/readiness_state.hpp"

namespace mowgli_mavros_bridge
{
ReadinessState::ReadinessState(double timeout_s, bool gnss_required,
                               bool wheel_odometry_required)
    : timeout_ns_(static_cast<int64_t>(timeout_s * 1e9)),
      gnss_required_(gnss_required),
      wheel_odometry_required_(wheel_odometry_required)
{
}

void ReadinessState::connection(bool connected)
{
  if (connected_ == connected)
  {
    return;
  }
  connected_ = connected;
  imu_stamp_.reset();
  gnss_stamp_.reset();
  wheel_stamp_.reset();
  traction_stamp_.reset();
  gnss_valid_ = false;
  traction_valid_ = false;
  gnss_sequence_ = 0;
  gnss_incarnation_.clear();
  last_status_ns_ = 0;
}

void ReadinessState::imu(int64_t receipt_ns)
{
  if (connected_ && receipt_ns > 0)
  {
    imu_stamp_ = receipt_ns;
    last_status_ns_ = receipt_ns;
  }
}

void ReadinessState::gnss(int64_t receipt_ns, uint64_t sequence,
                          const std::string& incarnation, bool valid)
{
  if (!connected_ || receipt_ns <= 0 || sequence == 0 || incarnation.empty())
  {
    return;
  }
  if (incarnation != gnss_incarnation_)
  {
    gnss_stamp_.reset();
    gnss_sequence_ = 0;
    gnss_incarnation_ = incarnation;
  }
  if (sequence <= gnss_sequence_)
  {
    // GnssStatus has no source-incarnation field in the MowgliNext IDL.
    // Accept a restarted source only after its prior observation has expired.
    if (!gnss_stamp_ || receipt_ns - *gnss_stamp_ <= timeout_ns_)
    {
      return;
    }
    gnss_sequence_ = 0;
  }
  gnss_sequence_ = sequence;
  gnss_stamp_ = receipt_ns;
  gnss_valid_ = valid;
  last_status_ns_ = receipt_ns;
}

void ReadinessState::wheel(int64_t receipt_ns)
{
  if (connected_ && receipt_ns > 0)
  {
    wheel_stamp_ = receipt_ns;
    last_status_ns_ = receipt_ns;
  }
}

void ReadinessState::traction(int64_t receipt_ns, bool valid)
{
  if (connected_ && receipt_ns > 0)
  {
    traction_stamp_ = receipt_ns;
    traction_valid_ = valid;
    last_status_ns_ = receipt_ns;
  }
}

bool ReadinessState::fresh(const std::optional<int64_t>& stamp, int64_t now_ns) const
{
  return stamp && now_ns >= *stamp && now_ns - *stamp <= timeout_ns_;
}

Readiness ReadinessState::project(int64_t now_ns) const
{
  Readiness result{connected_,
                   fresh(imu_stamp_, now_ns),
                   fresh(gnss_stamp_, now_ns),
                   gnss_valid_,
                   fresh(wheel_stamp_, now_ns),
                   fresh(traction_stamp_, now_ns),
                   traction_valid_,
                   false};
  result.ready = result.connected && result.imu_fresh && result.traction_fresh &&
                 result.traction_valid &&
                 (!gnss_required_ || (result.gnss_fresh && result.gnss_valid)) &&
                 (!wheel_odometry_required_ || result.wheel_fresh);
  return result;
}

int64_t ReadinessState::status_stamp_ns() const
{
  return last_status_ns_;
}
}  // namespace mowgli_mavros_bridge
