#include <chrono>
#include <thread>

#include "mavros_hardware_bridge_node.hpp"
#include <gtest/gtest.h>

namespace mowgli_mavros_bridge
{
// Deterministic callback boundary: OFF arrives with an ON queued, FCU already
// confirmed and real BladeControl stop acquisition complete. No executor timer
// can race the callback; the outbound servo requests are captured by a ROS mock.
class BridgeRegressionPeer
{
public:
  static int64_t steady()
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }
  static void wait_transport(MavrosHardwareBridgeNode& n)
  {
    ASSERT_TRUE(n.cli_command_long_->wait_for_service(std::chrono::seconds(3)));
  }
  static void setup(MavrosHardwareBridgeNode& n)
  {
    const auto t = steady();
    n.mavros_state_.connected = n.mavros_state_.armed = true;
    n.mavros_state_.mode = "HOLD";
    n.last_fcu_receipt_ms_ = n.last_hardware_safety_ms_ = t;
    n.safety_state_.observe_motor_outputs(true, true);
    n.blade_test_active_ = true;
    n.session_control_.demand("HOLD", true);
    n.session_control_.observe(true, true, "HOLD");
    n.blade_control_.connection(true, t - 1600);
    n.blade_control_.permission(true, t - 1600);
    auto neutral = n.blade_control_.next(t - 1600);
    ASSERT_TRUE(neutral);
    n.blade_control_.complete(neutral->token, true, t - 1600);
    for (int i = 0; i != 6; ++i)
    {
      const auto sample = t - 1250 + i * 250;
      n.blade_control_.sample(2, sample * 1000000, i + 1, true, true, 0, sample);
    }
    ASSERT_TRUE(n.blade_control_.stopped(t));
    n.pending_blade_starts_.push_back(
        {std::make_shared<rmw_request_id_t>(), BladeControl::Direction::Forward, t});
  }
  static void wait_safety_transport(MavrosHardwareBridgeNode& n)
  {
    ASSERT_TRUE(n.cli_arm_->wait_for_service(std::chrono::seconds(3)));
    ASSERT_TRUE(n.cli_set_mode_->wait_for_service(std::chrono::seconds(3)));
  }
  static void prepare_safety(MavrosHardwareBridgeNode& n,
                             const std::string& mode,
                             const std::string& reason)
  {
    setup(n);
    n.pending_blade_starts_.clear();
    n.blade_test_active_ = false;
    n.mavros_state_.mode = mode;
    n.last_high_level_status_.state = mode == "MANUAL" ? 4 : 2;
    n.session_control_.demand(mode, true);
    n.session_control_.observe(true, true, mode);
    if (reason == "lift")
      n.safety_state_.observe_button_change(3, n.now().nanoseconds());
    else if (reason == "tilt")
      n.safety_state_.observe_tilt(true);
    else if (reason == "switch")
      n.safety_state_.observe_motor_outputs(true, false);
    else
      n.safety_state_.set_service_emergency(true);
  }
  static bool disarm(MavrosHardwareBridgeNode& n, bool hold)
  {
    return hold ? n.request_hold_and_blade_disarm() : n.request_blade_disarm();
  }
  static void confirm_disarm(MavrosHardwareBridgeNode& n)
  {
    n.mavros_state_.armed = false;
    n.session_control_.observe(true, false, n.mavros_state_.mode);
  }
  static void off(MavrosHardwareBridgeNode& n)
  {
    auto request = std::make_shared<mowgli_interfaces::srv::MowerControl::Request>();
    n.on_mower_control(std::make_shared<rmw_request_id_t>(), request);
    EXPECT_FALSE(n.blade_test_active_);
    EXPECT_TRUE(n.pending_blade_starts_.empty());
    EXPECT_EQ(n.blade_control_.requested(), BladeControl::Direction::Off);
    EXPECT_FALSE(n.session_control_.requested_arm());
  }
};

TEST(BridgeRegression, OffCancelsReadyQueuedOnBeforeDispatch)
{
  rclcpp::init(0, nullptr);
  auto mock = std::make_shared<rclcpp::Node>("off_order_mock");
  std::vector<float> pwms;
  auto service = mock->create_service<mavros_msgs::srv::CommandLong>(
      "/mavros/cmd/command",
      [&pwms](const std::shared_ptr<mavros_msgs::srv::CommandLong::Request> req,
              std::shared_ptr<mavros_msgs::srv::CommandLong::Response> res)
      {
        pwms.push_back(req->param2);
        res->success = true;
        res->result = 0;
      });
  auto bridge = std::make_shared<MavrosHardwareBridgeNode>();
  // Discovery may finish without spinning the bridge (and its periodic timer).
  BridgeRegressionPeer::wait_transport(*bridge);
  BridgeRegressionPeer::setup(*bridge);
  BridgeRegressionPeer::off(*bridge);
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(mock);
  for (int i = 0; i != 20; ++i)
  {
    executor.spin_some();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_FALSE(pwms.empty());
  for (const auto pwm : pwms)
    EXPECT_EQ(pwm, 1500);
  executor.remove_node(mock);
  bridge.reset();
  mock.reset();
  rclcpp::shutdown();
}

class SafetyPathRegression : public testing::TestWithParam<std::pair<std::string, std::string>>
{
};
TEST_P(SafetyPathRegression, PreservesOriginalModePolicyAcrossDisarmAndTicks)
{
  rclcpp::init(0, nullptr);
  const auto [mode, reason] = GetParam();
  const bool hold = reason == "switch" || reason == "service";
  auto mock = std::make_shared<rclcpp::Node>("safety_path_mock");
  auto bridge = std::make_shared<MavrosHardwareBridgeNode>();
  std::vector<std::string> modes;
  std::vector<bool> arms;
  std::vector<float> pwms;
  auto arm = mock->create_service<mavros_msgs::srv::CommandBool>(
      "/mavros/cmd/arming",
      [&](std::shared_ptr<mavros_msgs::srv::CommandBool::Request> request,
          std::shared_ptr<mavros_msgs::srv::CommandBool::Response> response)
      {
        arms.push_back(request->value);
        response->success = true;
        response->result = 0;
        BridgeRegressionPeer::confirm_disarm(*bridge);
      });
  auto set_mode = mock->create_service<mavros_msgs::srv::SetMode>(
      "/mavros/set_mode",
      [&](std::shared_ptr<mavros_msgs::srv::SetMode::Request> request,
          std::shared_ptr<mavros_msgs::srv::SetMode::Response> response)
      {
        modes.push_back(request->custom_mode);
        response->mode_sent = true;
      });
  auto servo = mock->create_service<mavros_msgs::srv::CommandLong>(
      "/mavros/cmd/command",
      [&](std::shared_ptr<mavros_msgs::srv::CommandLong::Request> request,
          std::shared_ptr<mavros_msgs::srv::CommandLong::Response> response)
      {
        pwms.push_back(request->param2);
        response->success = true;
        response->result = 0;
      });
  auto arm_client = mock->create_client<mavros_msgs::srv::CommandBool>("/mavros/cmd/arming");
  ASSERT_TRUE(arm_client->wait_for_service(std::chrono::seconds(3)));
  BridgeRegressionPeer::wait_transport(*bridge);
  BridgeRegressionPeer::wait_safety_transport(*bridge);
  BridgeRegressionPeer::prepare_safety(*bridge, mode, reason);
  ASSERT_TRUE(BridgeRegressionPeer::disarm(*bridge, hold));
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(mock);
  executor.add_node(bridge);
  for (int i = 0; i != 40; ++i)
  {
    executor.spin_some();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_EQ(arms.size(), 1u);
  EXPECT_FALSE(arms.front());
  ASSERT_FALSE(pwms.empty());
  for (const auto pwm : pwms)
    EXPECT_EQ(pwm, 1500);
  if (hold)
  {
    ASSERT_EQ(modes.size(), 1u);
    EXPECT_EQ(modes.front(), "HOLD");
  }
  else
    EXPECT_TRUE(modes.empty());  // also after DISARM ACK, observed state and repeated ticks
  executor.remove_node(bridge);
  executor.remove_node(mock);
  bridge.reset();
  mock.reset();
  rclcpp::shutdown();
}
INSTANTIATE_TEST_SUITE_P(ExistingSafetyPaths,
                         SafetyPathRegression,
                         testing::Values(std::make_pair("MANUAL", "lift"),
                                         std::make_pair("GUIDED", "lift"),
                                         std::make_pair("MANUAL", "tilt"),
                                         std::make_pair("GUIDED", "tilt"),
                                         std::make_pair("MANUAL", "switch"),
                                         std::make_pair("GUIDED", "service")));
}  // namespace mowgli_mavros_bridge
