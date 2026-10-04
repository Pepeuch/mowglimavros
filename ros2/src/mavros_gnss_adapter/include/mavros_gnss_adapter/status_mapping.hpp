#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include "mowgli_interfaces/msg/gnss_status.hpp"
#include "universal_gnss_ros2/msg/gnss_status.hpp"

namespace mavros_gnss_adapter
{
using UniversalStatus = universal_gnss_ros2::msg::GnssStatus;
using Status = mowgli_interfaces::msg::GnssStatus;

inline uint32_t map_flags(uint32_t flags)
{
  uint32_t result = 0;
  if (flags & UniversalStatus::CAP_RTK_MODE) {result |= Status::CAP_RTK_MODE;}
  if (flags & UniversalStatus::CAP_HORIZONTAL_ACCURACY) {result |= Status::CAP_HORIZONTAL_ACCURACY;}
  if (flags & UniversalStatus::CAP_VERTICAL_ACCURACY) {result |= Status::CAP_VERTICAL_ACCURACY;}
  if (flags & UniversalStatus::CAP_HDOP) {result |= Status::CAP_HDOP;}
  if (flags & UniversalStatus::CAP_VDOP) {result |= Status::CAP_VDOP;}
  if (flags & UniversalStatus::CAP_SATELLITES_USED) {result |= Status::CAP_SATELLITES_USED;}
  if (flags & UniversalStatus::CAP_SATELLITES_VISIBLE) {result |= Status::CAP_SATELLITES_VISIBLE;}
  if (flags & UniversalStatus::CAP_SATELLITES_TRACKED) {result |= Status::CAP_SATELLITES_TRACKED;}
  if (flags & UniversalStatus::CAP_MEAN_CN0) {result |= Status::CAP_MEAN_CN0;}
  if (flags & UniversalStatus::CAP_MAX_CN0) {result |= Status::CAP_MAX_CN0;}
  if (flags & UniversalStatus::CAP_CORRECTION_AGE) {result |= Status::CAP_CORRECTION_AGE;}
  if (flags & UniversalStatus::CAP_HEADING) {result |= Status::CAP_HEADING;}
  if (flags & UniversalStatus::CAP_DUAL_ANTENNA_HEADING) {result |= Status::CAP_DUAL_ANTENNA_STATUS;}
  if (flags & UniversalStatus::CAP_INTERFERENCE_STATE) {result |= Status::CAP_INTERFERENCE_STATUS;}
  if (flags & UniversalStatus::CAP_JAMMING_STATE) {result |= Status::CAP_JAMMING_STATUS;}
  if (flags & UniversalStatus::CAP_HEADING_ACCURACY) {result |= Status::CAP_HEADING_ACCURACY;}
  if (flags & UniversalStatus::CAP_DIFFERENTIAL_CORRECTIONS) {result |= Status::CAP_DIFFERENTIAL_CORRECTIONS;}
  if (flags & UniversalStatus::CAP_CORRECTIONS_ACTIVE) {result |= Status::CAP_CORRECTIONS_ACTIVE;}
  if (flags & UniversalStatus::CAP_DUAL_ANTENNA_BASELINE) {result |= Status::CAP_DUAL_ANTENNA_BASELINE;}
  if (flags & UniversalStatus::CAP_BASELINE_AZIMUTH) {result |= Status::CAP_BASELINE_AZIMUTH;}
  if (flags & UniversalStatus::CAP_BASELINE_PITCH) {result |= Status::CAP_BASELINE_PITCH;}
  if (flags & UniversalStatus::CAP_BASELINE_LENGTH) {result |= Status::CAP_BASELINE_LENGTH;}
  if (flags & UniversalStatus::CAP_BASELINE_SOLUTION_STATUS) {result |= Status::CAP_BASELINE_SOLUTION_STATUS;}
  return result;
}

inline uint8_t map_fix(uint8_t value)
{
  switch (value)
  {
    case UniversalStatus::FIX_TYPE_NO_FIX: return Status::FIX_TYPE_NO_FIX;
    case UniversalStatus::FIX_TYPE_FIX: return Status::FIX_TYPE_GPS_FIX;
    case UniversalStatus::FIX_TYPE_RTK_FLOAT: return Status::FIX_TYPE_RTK_FLOAT;
    case UniversalStatus::FIX_TYPE_RTK_FIXED: return Status::FIX_TYPE_RTK_FIXED;
    case UniversalStatus::FIX_TYPE_DEAD_RECKONING: return Status::FIX_TYPE_DEAD_RECKONING;
    case UniversalStatus::FIX_TYPE_2D_FIX: return Status::FIX_TYPE_2D_FIX;
    case UniversalStatus::FIX_TYPE_3D_FIX: return Status::FIX_TYPE_3D_FIX;
    case UniversalStatus::FIX_TYPE_DGPS: return Status::FIX_TYPE_DGPS;
    default: return Status::FIX_TYPE_NO_FIX;
  }
}

inline uint8_t map_rtk(uint8_t value)
{
  switch (value)
  {
    case UniversalStatus::RTK_MODE_UNKNOWN: return Status::RTK_MODE_UNKNOWN;
    case UniversalStatus::RTK_MODE_NONE: return Status::RTK_MODE_NONE;
    case UniversalStatus::RTK_MODE_FLOAT: return Status::RTK_MODE_FLOAT;
    case UniversalStatus::RTK_MODE_FIXED: return Status::RTK_MODE_FIXED;
    default: return Status::RTK_MODE_UNKNOWN;
  }
}

inline uint8_t map_baseline(uint8_t value)
{
  switch (value)
  {
    case UniversalStatus::BASELINE_STATUS_UNKNOWN: return Status::BASELINE_STATUS_UNKNOWN;
    case UniversalStatus::BASELINE_STATUS_COMPUTED: return Status::BASELINE_STATUS_COMPUTED;
    case UniversalStatus::BASELINE_STATUS_NOT_SOLVED: return Status::BASELINE_STATUS_NOT_SOLVED;
    case UniversalStatus::BASELINE_STATUS_INSUFFICIENT_OBSERVATIONS: return Status::BASELINE_STATUS_INSUFFICIENT_OBSERVATIONS;
    case UniversalStatus::BASELINE_STATUS_NO_CONVERGENCE: return Status::BASELINE_STATUS_NO_CONVERGENCE;
    case UniversalStatus::BASELINE_STATUS_OUT_OF_TOLERANCE: return Status::BASELINE_STATUS_OUT_OF_TOLERANCE;
    case UniversalStatus::BASELINE_STATUS_COVARIANCE_TRACE_EXCEEDED: return Status::BASELINE_STATUS_COVARIANCE_TRACE_EXCEEDED;
    case UniversalStatus::BASELINE_STATUS_NOT_CONFIGURED: return Status::BASELINE_STATUS_NOT_CONFIGURED;
    default: return Status::BASELINE_STATUS_UNKNOWN;
  }
}

inline Status map_status(const UniversalStatus & input, const std::string & source)
{
  Status output;
  output.header.stamp = input.stamp;
  output.header.frame_id = "gps_link";
  output.backend = "mavros_" + source;
  output.fix_type = map_fix(input.fix_type);
  output.fix_valid = input.fix_valid && output.fix_type != Status::FIX_TYPE_NO_FIX;
  output.rtk_mode = map_rtk(input.rtk_mode);
  output.baseline_solution_status = map_baseline(input.baseline_solution_status);
  output.dead_reckoning = output.fix_type == Status::FIX_TYPE_DEAD_RECKONING;
  output.capability_flags = map_flags(input.capability_flags);
  output.value_flags = map_flags(input.value_flags & input.capability_flags);
  // The upstream transport supplies neither a UI quality score nor NTRIP/MSM truth.
  output.quality_percent = std::numeric_limits<float>::quiet_NaN();
  output.msm_summary_age_s = std::numeric_limits<float>::quiet_NaN();
  output.position_observation_sequence = input.position_observation_sequence;
  output.horizontal_accuracy_m = input.horizontal_accuracy_m;
  output.vertical_accuracy_m = input.vertical_accuracy_m;
  output.hdop = input.hdop;
  output.vdop = input.vdop;
  output.satellites_used = input.satellites_used;
  output.satellites_visible = input.satellites_visible;
  output.satellites_tracked = input.satellites_tracked;
  output.mean_cn0_db_hz = input.mean_cn0_db_hz;
  output.max_cn0_db_hz = input.max_cn0_db_hz;
  output.correction_age_s = input.correction_age_s;
  output.heading_deg = input.heading_deg;
  output.heading_accuracy_deg = input.heading_accuracy_deg;
  output.differential_corrections = input.differential_corrections;
  output.corrections_active = input.corrections_active;
  output.dual_antenna_heading = input.dual_antenna_heading;
  output.interference_detected = input.interference_detected;
  output.jamming_detected = input.jamming_detected;
  output.dual_antenna_baseline = input.dual_antenna_baseline;
  output.baseline_azimuth_deg = input.baseline_azimuth_deg;
  output.baseline_pitch_deg = input.baseline_pitch_deg;
  output.baseline_length_m = input.baseline_length_m;
  return output;
}
}  // namespace mavros_gnss_adapter
