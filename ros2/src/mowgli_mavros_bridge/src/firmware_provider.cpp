#include "mowgli_mavros_bridge/firmware_provider.hpp"
#include "mowgli_mavros_bridge/rover_manual_control.hpp"

#include <stdexcept>

namespace mowgli_mavros_bridge
{
namespace
{
// Preserve the existing conversion for both profiles. This is implemented
// software behavior, not proof of PX4/ArduPilot actuator compatibility.
class CurrentRuntimeProvider : public FirmwareProvider
{
public:
  FirmwareCapabilities capabilities() const noexcept override
  {
    return {ManualControlMapping::RoverSteeringYThrottleZ, true, true, true, true, false, false};
  }

  EmergencyPolicy default_emergency_policy() const override
  {
    return {"HOLD", true};
  }

  EmergencyPolicy emergency_policy(
    const std::string & configured_mode, bool configured_disarm) const override
  {
    // Retain all existing ROS parameter overrides, including an empty mode.
    return {configured_mode, configured_disarm};
  }

  mavros_msgs::msg::ManualControl manual_control_from_twist(
    const geometry_msgs::msg::TwistStamped & twist,
    double linear_scale, double yaw_scale) const override
  {
    return rover_manual_control_from_twist(twist, linear_scale, yaw_scale);
  }
};

class ArduPilotProvider final : public CurrentRuntimeProvider
{
public:
  const char * name() const noexcept override {return "ardupilot";}
};

class PX4Provider final : public CurrentRuntimeProvider
{
public:
  const char * name() const noexcept override {return "px4";}
};
}  // namespace

std::unique_ptr<FirmwareProvider> make_firmware_provider(const std::string & firmware)
{
  if (firmware == "ardupilot") {
    return std::make_unique<ArduPilotProvider>();
  }
  if (firmware == "px4") {
    return std::make_unique<PX4Provider>();
  }
  if (firmware == "betaflight" || firmware == "inav" || firmware == "mowgli") {
    throw std::invalid_argument("MAVROS_FIRMWARE=" + firmware + ": not implemented");
  }
  throw std::invalid_argument("C++ provider requires a resolved ardupilot or px4 firmware");
}
}  // namespace mowgli_mavros_bridge
