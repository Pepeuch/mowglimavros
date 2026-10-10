#!/usr/bin/env python3
"""Static guardrails complement the C++ state and mocked ROS graph tests."""
from pathlib import Path
import unittest


class BladeOffContract(unittest.TestCase):
    def test_mower_service_has_no_arming_or_mode_side_effect(self):
        source = (Path(__file__).parents[1] / "src/mavros_hardware_bridge_node.cpp").read_text()
        body = source.split("void MavrosHardwareBridgeNode::on_mower_control(", 1)[1]
        body = body.split("bool MavrosHardwareBridgeNode::request_blade_neutral()", 1)[0]
        self.assertNotIn("send_arm_command", body)
        self.assertNotIn("send_mode_command", body)
        self.assertIn("BladeControl::Direction::Off", body)
        self.assertIn("BladeControl::Direction::Forward", body)
        self.assertIn("BladeControl::Direction::Reverse", body)

    def test_neutral_transport_has_no_arm_fallback(self):
        source = (Path(__file__).parents[1] / "src/mavros_hardware_bridge_node.cpp").read_text()
        body = source.split("bool MavrosHardwareBridgeNode::request_blade_neutral()", 1)[1]
        body = body.split("void MavrosHardwareBridgeNode::on_emergency_stop(", 1)[0]
        self.assertNotIn("send_arm_command", body)
        self.assertNotIn("cli_arm_", body)
        self.assertIn("request->command = 183", body)
        self.assertIn("command->channel", body)
        self.assertIn("command->pwm", body)
        self.assertIn("response->result == 0", body)


if __name__ == "__main__":
    unittest.main()
