#pragma once

#include <memory>
#include <string>

#include <geometry_msgs/msg/twist_stamped.hpp>
#include <mavros_msgs/msg/manual_control.hpp>

namespace mowgli_mavros_bridge
{
enum class ManualControlMapping { RoverSteeringYThrottleZ };

struct FirmwareCapabilities
{
  ManualControlMapping manual_control_mapping;
  bool manual_control_mapping_implemented;
  bool arm_implemented;
  bool disarm_implemented;
  bool mode_control_implemented;
  bool blade_control_implemented;
  bool actuation_validated;
};

struct EmergencyPolicy
{
  std::string mode;
  bool disarm;
};

// Firmware mapping, emergency policy and capabilities belong here. Safety gates,
// ROS request transport, timestamps, telemetry and status remain in the bridge.
class FirmwareProvider
{
public:
  virtual ~FirmwareProvider() = default;
  virtual const char * name() const noexcept = 0;
  virtual FirmwareCapabilities capabilities() const noexcept = 0;
  virtual EmergencyPolicy default_emergency_policy() const = 0;
  virtual EmergencyPolicy emergency_policy(
    const std::string & configured_mode, bool configured_disarm) const = 0;
  virtual mavros_msgs::msg::ManualControl manual_control_from_twist(
    const geometry_msgs::msg::TwistStamped & twist,
    double linear_scale, double yaw_scale) const = 0;
};

std::unique_ptr<FirmwareProvider> make_firmware_provider(const std::string & firmware);
}  // namespace mowgli_mavros_bridge
