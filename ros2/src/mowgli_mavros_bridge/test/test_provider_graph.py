#!/usr/bin/env python3
"""Real bridge with mock MAVROS services in isolated domains, without hardware."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


CASES = (
    (None, None, False, False),  # Provider defaults, drive disabled.
    ("CUSTOM", False, False, True),  # Mode-only emergency, neutral-only drive.
    ("", True, True, False),  # Disarm-only emergency, existing mapping enabled.
    ("", False, False, False),  # Explicitly no FCU emergency request.
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bridge-executable", required=True)
    parser.add_argument("--provider", choices=("ardupilot", "px4"))
    parser.add_argument("--case", type=int)
    args = parser.parse_args()
    if args.provider is None:
        for index, provider in enumerate(("ardupilot", "px4")):
            for case in range(len(CASES)):
                env = dict(os.environ, ROS_DOMAIN_ID=str(221 + index * len(CASES) + case),
                           ROS_AUTOMATIC_DISCOVERY_RANGE="LOCALHOST",
                           MAVROS_RESOLVED_FIRMWARE=provider)
                subprocess.run(
                    [sys.executable, __file__, "--bridge-executable", args.bridge_executable,
                     "--provider", provider, "--case", str(case)],
                    env=env, check=True, timeout=25)
        return

    import rclpy
    from geometry_msgs.msg import TwistStamped
    from mavros_msgs.msg import ManualControl, State, SysStatus
    from mavros_msgs.srv import CommandBool, SetMode
    from mowgli_interfaces.srv import EmergencyStop

    rclpy.init()
    node = rclpy.create_node("provider_contract_mock")
    requests = []
    commands = []
    mode, disarm, drive, neutral = CASES[args.case]

    def mode_request(request, response):
        requests.append(("mode", request.custom_mode))
        response.mode_sent = False  # Preserve local-forwarded vs FCU-confirmed semantics.
        return response

    def arm_request(request, response):
        requests.append(("arm", request.value))
        response.success = False
        return response

    services = [node.create_service(SetMode, "/mavros/set_mode", mode_request),
                node.create_service(CommandBool, "/mavros/cmd/arming", arm_request)]
    subscription = node.create_subscription(ManualControl, "/mavros/manual_control/send", commands.append, 10)
    publisher = node.create_publisher(TwistStamped, "/cmd_vel", 10)
    state_publisher = node.create_publisher(State, "/mavros/state", 10)
    safety_publisher = node.create_publisher(SysStatus, "/mavros/sys_status", 10)
    fcu_state = State(connected=True, armed=True)
    safety_status = SysStatus(sensors_present=1 << 15, sensors_enabled=1 << 15)
    def publish_safety():
        state_publisher.publish(fcu_state)
        safety_publisher.publish(safety_status)
    safety_timer = node.create_timer(0.05, publish_safety)
    client = node.create_client(EmergencyStop, "/hardware_bridge/emergency_stop")

    def wait_for(predicate, timeout=6):
        end = time.monotonic() + timeout
        while not predicate() and time.monotonic() < end:
            rclpy.spin_once(node, timeout_sec=0.05)
        assert predicate(), "timed out waiting for bridge/mock ROS exchange"

    def send_twist(linear, yaw):
        before = len(commands)
        message = TwistStamped()
        message.twist.linear.x = linear
        message.twist.angular.z = yaw
        for _ in range(5):
            publisher.publish(message)
            rclpy.spin_once(node, timeout_sec=0.05)
        end = time.monotonic() + 0.25
        while time.monotonic() < end:
            rclpy.spin_once(node, timeout_sec=0.05)
        return commands[before:]

    parameters = {"manual_control_enabled": drive, "neutral_manual_control_enabled": neutral}
    if mode is not None:
        parameters["emergency_mode"] = mode
        parameters["emergency_disarm"] = disarm
    with tempfile.TemporaryDirectory(prefix="provider-graph-") as directory:
        config = Path(directory) / "parameters.yaml"
        config.write_text(json.dumps({"hardware_bridge": {"ros__parameters": parameters}}))
        with open(Path(directory) / "bridge.log", "w+") as log:
            process = subprocess.Popen(
                [args.bridge_executable, "--ros-args", "--params-file", str(config)],
                stdout=log, stderr=subprocess.STDOUT)
            try:
                wait_for(lambda: client.service_is_ready() and publisher.get_subscription_count() == 1)
                wait_for(lambda: subscription.get_publisher_count() == 1)
                wait_for(lambda: state_publisher.get_subscription_count() == 1 and safety_publisher.get_subscription_count() == 1)
                end = time.monotonic() + 0.3
                while time.monotonic() < end:
                    rclpy.spin_once(node, timeout_sec=0.05)
                output = send_twist(0.2, -0.35)
                if drive:
                    assert output
                    assert all((msg.x, msg.y, msg.z, msg.r, msg.buttons) == (0, -350, 200, 0, 0) for msg in output)
                else:
                    assert not output, "drive gate changed"
                output = send_twist(0.0, 0.0)
                assert bool(output) == (drive or neutral)
                assert all((msg.x, msg.y, msg.z, msg.r, msg.buttons) == (0, 0, 0, 0, 0) for msg in output)
                assert not send_twist(float("nan"), 0.0), "invalid command gate changed"
                assert not requests, "traction opt-in must never auto-arm or change mode"
                if drive:
                    safety_status.sensors_enabled = 0
                    for _ in range(5):
                        publish_safety()
                        rclpy.spin_once(node, timeout_sec=0.05)
                    output = send_twist(0.2, 0.0)
                    assert output and all(msg.x == msg.y == msg.z == msg.r == 0 for msg in output), "physical safety must block traction"
                    safety_status.sensors_enabled = 1 << 15
                    for _ in range(5):
                        publish_safety()
                        rclpy.spin_once(node, timeout_sec=0.05)

                future = client.call_async(EmergencyStop.Request(emergency=1))
                wait_for(future.done)
                assert future.result().success, "local-forwarded semantics changed"
                expected_mode = "HOLD" if mode is None else mode
                expected_disarm = True if disarm is None else disarm
                expected = ([] if not expected_mode else [("mode", expected_mode)])
                if expected_disarm:
                    expected.append(("arm", False))
                wait_for(lambda: len(requests) == len(expected))
                assert sorted(requests) == sorted(expected), (requests, expected)
                # Clearing the request retains the common local-only behavior.
                future = client.call_async(EmergencyStop.Request(emergency=0))
                wait_for(future.done)
                assert future.result().success
                assert len(requests) == len(expected)
                print(f"PASS provider={args.provider} case={args.case}")
            except BaseException:
                log.flush()
                log.seek(0)
                print(log.read(), file=sys.stderr)
                raise
            finally:
                process.send_signal(signal.SIGINT)
                try:
                    process.wait(timeout=4)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
