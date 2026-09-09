#include "mowgli_mavros_bridge/readiness_state.hpp"
namespace mowgli_mavros_bridge {
ReadinessState::ReadinessState(double timeout_s) : timeout_ns_(static_cast<int64_t>(timeout_s * 1e9)) {}
void ReadinessState::connection(bool connected) { if (connected_ != connected) { connected_=connected; gnss_stamp_.reset(); wheel_stamp_.reset(); traction_stamp_.reset(); gnss_valid_=traction_valid_=false; gnss_sequence_=0; gnss_incarnation_.clear(); last_status_ns_=0; } }
void ReadinessState::gnss(int64_t s,uint64_t seq,const std::string & inc,bool valid) { if(!connected_ || s <= 0 || seq==0 || inc.empty()) return; if(inc != gnss_incarnation_) { gnss_stamp_.reset(); gnss_sequence_=0; gnss_incarnation_=inc; } if(seq <= gnss_sequence_) return; gnss_sequence_=seq; gnss_stamp_=s; gnss_valid_=valid; last_status_ns_=s; }
void ReadinessState::wheel(int64_t s) { if(connected_ && s > 0) { wheel_stamp_=s; last_status_ns_=s; } }
void ReadinessState::traction(int64_t s,bool valid) { if(connected_ && s > 0) { traction_stamp_=s; traction_valid_=valid; last_status_ns_=s; } }
bool ReadinessState::fresh(const std::optional<int64_t> & s,int64_t now) const { return s && now>=*s && now-*s<=timeout_ns_; }
Readiness ReadinessState::project(int64_t now) const { Readiness r{connected_,fresh(gnss_stamp_,now),gnss_valid_,fresh(wheel_stamp_,now),fresh(traction_stamp_,now),traction_valid_,false}; r.ready=r.connected&&r.gnss_fresh&&r.gnss_valid&&r.wheel_fresh&&r.traction_fresh&&r.traction_valid; return r; }
int64_t ReadinessState::status_stamp_ns() const { return last_status_ns_; }
}  // namespace mowgli_mavros_bridge
