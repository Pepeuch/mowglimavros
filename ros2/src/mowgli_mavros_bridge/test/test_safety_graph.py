#!/usr/bin/env python3
"""Exercise bridge safety inputs through a real MAVROS 2.16 graph."""
import argparse
import json
from pathlib import Path
import signal
import struct
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bridge-executable", required=True)
    args = parser.parse_args()

    import rclpy
    from ament_index_python.packages import get_package_prefix
    from diagnostic_msgs.msg import DiagnosticArray, DiagnosticStatus
    from mavros_msgs.msg import Mavlink, State
    from mowgli_interfaces.msg import Emergency
    from mowgli_interfaces.srv import EmergencyStop
    from rcl_interfaces.srv import SetParametersAtomically
    from rclpy.parameter import Parameter

    rclpy.init()
    node = rclpy.create_node("safety_contract_test")
    states, emergencies, diagnostics = [], [], []
    subscriptions = [
        node.create_subscription(State, "/mavros/state", states.append, 20),
        node.create_subscription(
            Emergency, "/hardware_bridge/emergency", emergencies.append, 20
        ),
        node.create_subscription(DiagnosticArray, "/diagnostics", diagnostics.append, 20),
    ]
    raw_publisher = node.create_publisher(Mavlink, "/uas1/mavlink_source", 100)
    parameters = node.create_client(
        SetParametersAtomically, "/hardware_bridge/set_parameters_atomically"
    )
    emergency_stop = node.create_client(
        EmergencyStop, "/hardware_bridge/emergency_stop"
    )

    def frame(msgid, payload):
        message = Mavlink()
        message.framing_status = Mavlink.FRAMING_OK
        message.magic = Mavlink.MAVLINK_V20
        message.len = len(payload)
        message.sysid = 1
        message.compid = 1
        message.msgid = msgid
        padded = payload + bytes((-len(payload)) % 8)
        message.payload64 = list(
            struct.unpack("<" + "Q" * (len(padded) // 8), padded)
        )
        return message

    heartbeat = frame(0, struct.pack("<IBBBBB", 0, 10, 3, 0, 4, 3))
    heartbeat_enabled = [True]
    last_heartbeat = [0.0]

    def spin(seconds=0.15):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            if heartbeat_enabled[0] and time.monotonic() - last_heartbeat[0] > 0.25:
                raw_publisher.publish(heartbeat)
                last_heartbeat[0] = time.monotonic()
            rclpy.spin_once(node, timeout_sec=0.025)

    def wait(predicate, timeout=7):
        end = time.monotonic() + timeout
        while not predicate() and time.monotonic() < end:
            spin(0.05)
        assert predicate(), "timeout waiting for safety graph condition"

    def send(message):
        for _ in range(3):
            raw_publisher.publish(message)
            spin(0.08)
        spin(0.1)

    def button(state):
        return frame(257, struct.pack("<IIB", 100, 90, state))

    def sys_status(motor_outputs_enabled):
        motor_outputs = 1 << 15
        enabled = motor_outputs if motor_outputs_enabled else 0
        return frame(
            1,
            struct.pack(
                "<IIIHHhHHHHHHb",
                motor_outputs,
                enabled,
                motor_outputs,
                0,
                25000,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                100,
            ),
        )

    def latest_diagnostic(name):
        matches = [
            item
            for array in diagnostics
            for item in array.status
            if item.name == name
        ]
        return matches[-1] if matches else None

    def diagnostic_values(status):
        return {value.key: value.value for value in status.values}

    def configure(values):
        wait(parameters.service_is_ready)
        request = SetParametersAtomically.Request(
            parameters=[
                Parameter(name, value=value).to_parameter_msg()
                for name, value in values.items()
            ]
        )
        future = parameters.call_async(request)
        wait(future.done)
        return future.result().result.successful

    with tempfile.TemporaryDirectory(prefix="safety-graph-") as directory:
        config = Path(directory) / "mavros.yaml"
        config.write_text(
            json.dumps(
                {
                    "/**": {
                        "ros__parameters": {
                            "plugin_allowlist": ["sys_status"],
                            "fcu_url": "udp://127.0.0.1:19882@127.0.0.1:19883",
                            "gcs_url": "",
                            "system_id": 255,
                            "tgt_system": 1,
                            "tgt_component": 1,
                        }
                    }
                }
            )
        )
        mavros = str(Path(get_package_prefix("mavros")) / "lib/mavros/mavros_node")
        logs = [
            open(Path(directory) / "mavros.log", "w+"),
            open(Path(directory) / "bridge.log", "w+"),
        ]
        processes = []
        try:
            processes.append(
                subprocess.Popen(
                    [mavros, "--ros-args", "--params-file", str(config)],
                    stdout=logs[0],
                    stderr=subprocess.STDOUT,
                )
            )
            processes.append(
                subprocess.Popen(
                    [args.bridge_executable],
                    stdout=logs[1],
                    stderr=subprocess.STDOUT,
                )
            )
            wait(lambda: states and states[-1].connected)
            wait(lambda: raw_publisher.get_subscription_count() >= 2)
            wait(
                lambda: latest_diagnostic("mowgli_mavros_bridge/wheel_lift")
                is not None
            )
            assert (
                latest_diagnostic("mowgli_mavros_bridge/wheel_lift").level
                == DiagnosticStatus.STALE
            )

            send(button(0x01))
            wait(
                lambda: emergencies
                and emergencies[-1].lift_warning
                and not emergencies[-1].active_emergency
            )
            wait(
                lambda: latest_diagnostic("mowgli_mavros_bridge/wheel_lift").level
                == DiagnosticStatus.WARN
            )
            wheel_lift = latest_diagnostic("mowgli_mavros_bridge/wheel_lift")
            assert wheel_lift.level == wheel_lift.WARN
            assert diagnostic_values(wheel_lift)["left_lifted"] == "true"
            assert diagnostic_values(wheel_lift)["right_lifted"] == "false"

            send(button(0x02))
            wait(
                lambda: diagnostic_values(
                    latest_diagnostic("mowgli_mavros_bridge/wheel_lift")
                ).get("right_lifted")
                == "true"
            )
            send(button(0x03))
            wait(
                lambda: emergencies
                and emergencies[-1].active_emergency
                and emergencies[-1].reason == "WHEEL_LIFT"
            )

            assert configure({"wheel_lift_safety_enabled": False})
            wait(
                lambda: emergencies
                and not emergencies[-1].lift_warning
                and not emergencies[-1].active_emergency
            )
            wait(
                lambda: diagnostic_values(
                    latest_diagnostic("mowgli_mavros_bridge/wheel_lift")
                ).get("safety_enabled")
                == "false"
            )
            wheel_lift = latest_diagnostic("mowgli_mavros_bridge/wheel_lift")
            assert wheel_lift.level == wheel_lift.ERROR
            assert diagnostic_values(wheel_lift)["raw_button_state"] == "3"

            send(sys_status(False))
            wait(
                lambda: emergencies
                and emergencies[-1].active_emergency
                and emergencies[-1].reason == "HARDWARE_SAFETY_SWITCH"
            )
            wait(
                lambda: latest_diagnostic(
                    "mowgli_mavros_bridge/hardware_emergency_stop"
                ).message
                == "engaged"
            )
            wait(emergency_stop.service_is_ready)
            future = emergency_stop.call_async(EmergencyStop.Request(emergency=0))
            wait(future.done)
            assert future.result().success
            wait(
                lambda: emergencies[-1].active_emergency
                and emergencies[-1].reason == "HARDWARE_SAFETY_SWITCH"
            )

            send(sys_status(True))
            wait(
                lambda: latest_diagnostic(
                    "mowgli_mavros_bridge/hardware_emergency_stop"
                ).message
                == "released"
            )
            wait(lambda: not emergencies[-1].active_emergency)

            heartbeat_enabled[0] = False
            wait(lambda: states and not states[-1].connected, timeout=16)
            wait(
                lambda: diagnostic_values(
                    latest_diagnostic("mowgli_mavros_bridge/wheel_lift")
                ).get("state_valid")
                == "false"
            )
            wait(
                lambda: latest_diagnostic(
                    "mowgli_mavros_bridge/hardware_emergency_stop"
                ).message
                == "unknown"
            )
            print(
                "PASS safety graph: MAVROS raw BUTTON_CHANGE, SysStatus, dynamic gate, "
                "diagnostics, priority and disconnect"
            )
        except BaseException:
            for log in logs:
                log.flush()
                log.seek(0)
                print(log.read())
            raise
        finally:
            for process in processes:
                process.send_signal(signal.SIGINT)
            for process in processes:
                try:
                    process.wait(timeout=4)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            for log in logs:
                log.close()
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
