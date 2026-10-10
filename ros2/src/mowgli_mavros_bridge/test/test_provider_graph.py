#!/usr/bin/env python3
"""Prove traction and blade mappings against real bridge ROS interfaces."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bridge-executable", required=True)
    parser.add_argument("--provider", choices=("ardupilot", "px4"))
    parser.add_argument("--mowing-enabled", choices=("true", "false"))
    args = parser.parse_args()
    if args.provider is None:
        for provider_index, provider in enumerate(("ardupilot", "px4")):
            for enabled_index, enabled in enumerate(("true", "false")):
                env = dict(
                    os.environ,
                    ROS_DOMAIN_ID=str(221 + provider_index * 2 + enabled_index),
                    ROS_AUTOMATIC_DISCOVERY_RANGE="LOCALHOST",
                    MAVROS_RESOLVED_FIRMWARE=provider,
                )
                subprocess.run(
                    [
                        sys.executable,
                        __file__,
                        "--bridge-executable",
                        args.bridge_executable,
                        "--provider",
                        provider,
                        "--mowing-enabled",
                        enabled,
                    ],
                    env=env,
                    check=True,
                    timeout=30,
                )
        return

    import rclpy
    from geometry_msgs.msg import TwistStamped
    from mavros_msgs.msg import ManualControl, State, SysStatus
    from mavros_msgs.srv import CommandBool, CommandLong, SetMode
    from mowgli_interfaces.msg import HighLevelStatus
    from mowgli_interfaces.srv import EmergencyStop, MowerControl
    from std_srvs.srv import Trigger

    mowing_enabled = args.mowing_enabled == "true"
    rclpy.init()
    node = rclpy.create_node("provider_contract_mock")
    requests = []
    commands = []

    def mode_request(request, response):
        requests.append(("mode", request.custom_mode))
        response.mode_sent = True
        return response

    def arm_request(request, response):
        requests.append(("arm", request.value))
        response.success = True
        return response

    def command_request(request, response):
        requests.append(("command", request.command, request.param1, request.param2))
        response.success = True
        return response

    services = [
        node.create_service(SetMode, "/mavros/set_mode", mode_request),
        node.create_service(CommandBool, "/mavros/cmd/arming", arm_request),
        node.create_service(CommandLong, "/mavros/cmd/command", command_request),
    ]
    subscription = node.create_subscription(
        ManualControl, "/mavros/manual_control/send", commands.append, 10
    )
    publisher = node.create_publisher(TwistStamped, "/cmd_vel", 10)
    state_publisher = node.create_publisher(State, "/mavros/state", 10)
    safety_publisher = node.create_publisher(SysStatus, "/mavros/sys_status", 10)
    high_level_publisher = node.create_publisher(
        HighLevelStatus, "/behavior_tree_node/high_level_status", 10
    )
    fcu_state = State(connected=True, armed=False)
    safety_status = SysStatus(sensors_present=1 << 15, sensors_enabled=1 << 15)

    def publish_fcu():
        state_publisher.publish(fcu_state)
        safety_publisher.publish(safety_status)

    safety_timer = node.create_timer(0.05, publish_fcu)
    emergency = node.create_client(EmergencyStop, "/hardware_bridge/emergency_stop")
    mower = node.create_client(MowerControl, "/hardware_bridge/mower_control")
    reboot = node.create_client(Trigger, "/hardware_bridge/reboot_board")

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
        return commands[before:]

    def call(client, request):
        future = client.call_async(request)
        wait_for(future.done)
        return future.result()

    with tempfile.TemporaryDirectory(prefix="provider-graph-") as directory:
        config = Path(directory) / "parameters.yaml"
        config.write_text(
            json.dumps(
                {
                    "hardware_bridge": {
                        "ros__parameters": {"mowing_enabled": mowing_enabled}
                    }
                }
            )
        )
        with open(Path(directory) / "bridge.log", "w+") as log:
            process = subprocess.Popen(
                [args.bridge_executable, "--ros-args", "--params-file", str(config)],
                stdout=log,
                stderr=subprocess.STDOUT,
            )
            try:
                wait_for(
                    lambda: emergency.service_is_ready()
                    and mower.service_is_ready()
                    and reboot.service_is_ready()
                    and publisher.get_subscription_count() == 1
                )
                wait_for(lambda: subscription.get_publisher_count() == 1)
                wait_for(
                    lambda: state_publisher.get_subscription_count() == 1
                    and safety_publisher.get_subscription_count() == 1
                )
                if args.provider == "ardupilot":
                    wait_for(lambda: ("command", 183, 3.0, 1500.0) in requests)
                requests.clear()  # Startup neutral is blade-owned, not traction-owned.

                # Traction is forwarded while FCU armed=false. It must not ARM or
                # change mode as a side effect of receiving /cmd_vel.
                output = send_twist(0.2, -0.35)
                assert output
                assert all(
                    (msg.x, msg.y, msg.z, msg.r, msg.buttons)
                    == (0, -350, 200, 0, 0)
                    for msg in output
                )
                assert not requests
                assert not send_twist(float("nan"), 0.0)

                # PLAY states request MANUAL exactly on entry.
                high_level_publisher.publish(
                    HighLevelStatus(state=HighLevelStatus.HIGH_LEVEL_STATE_AUTONOMOUS)
                )
                wait_for(lambda: ("mode", "MANUAL") in requests)

                # ON is rejected while the global FCU is disarmed.
                # Neither ON nor OFF owns the FCU ARM/DISARM authority.
                before = len(requests)
                response = call(
                    mower,
                    MowerControl.Request(mow_enabled=True, mow_direction=1),
                )
                assert not response.success
                assert requests[before:] == []

                before = len(requests)
                response = call(
                    mower,
                    MowerControl.Request(mow_enabled=False, mow_direction=0),
                )
                if args.provider == "ardupilot":
                    assert response.success
                    assert requests[before:] == []  # Startup neutral already ACKed.
                    for _ in range(10):
                        assert call(
                            mower,
                            MowerControl.Request(mow_enabled=False, mow_direction=1),
                        ).success
                    assert requests[before:] == []
                else:
                    # No unsupported PX4 servo primitive or ARM alias fallback.
                    assert not response.success
                    assert requests[before:] == []

                # E-stop produces HOLD + immediate neutral + blade DISARM and
                # keeps subsequent traction neutral until the stop is released.
                before_commands = len(commands)
                before_requests = len(requests)
                assert call(emergency, EmergencyStop.Request(emergency=1)).success
                expected = [
                    ("arm", False),
                    ("mode", "HOLD"),
                ]
                if args.provider == "ardupilot":
                    expected.append(("command", 183, 3.0, 1500.0))
                wait_for(lambda: len(requests) == before_requests + len(expected))
                assert sorted(requests[before_requests:]) == sorted(expected)
                wait_for(lambda: len(commands) > before_commands)
                assert commands[-1].y == commands[-1].z == 0
                stopped = send_twist(0.2, 0.0)
                assert stopped and all(msg.y == msg.z == 0 for msg in stopped)

                assert call(emergency, EmergencyStop.Request(emergency=0)).success
                resumed = send_twist(0.2, 0.0)
                assert resumed and all(msg.z == 200 for msg in resumed)

                before = len(requests)
                reboot_response = call(reboot, Trigger.Request())
                assert reboot_response.success
                wait_for(lambda: len(requests) == before + 1)
                command = requests[-1]
                assert command[0:2] == ("command", 246)
                assert command[2] == 1.0
                print(
                    f"PASS provider={args.provider} mowing_enabled={mowing_enabled}"
                )
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
