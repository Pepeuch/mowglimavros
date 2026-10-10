#!/usr/bin/env python3
"""Real compiled bridge, mocked FCU services: never connects to hardware."""
import argparse
import os
from pathlib import Path
import signal
import struct
import subprocess
import tempfile
import time

import rclpy
from std_msgs.msg import Header
from mavros_msgs.msg import State, SysStatus, RCOut
from mavros_msgs.msg import Mavlink
from sensor_msgs.msg import Imu
from geometry_msgs.msg import Quaternion
from mavros_msgs.srv import CommandBool, CommandLong, SetMode
from mavros_esc_wheel_odometry.msg import EscObservation
from mowgli_interfaces.msg import Status
from mowgli_interfaces.srv import MowerControl, EmergencyStop


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bridge-executable", required=True)
    args = parser.parse_args()
    rclpy.init()
    node = rclpy.create_node("blade_contract_mock")
    commands, statuses, authority = [], [], []
    pwm, rpm, count = 1500, 0, 0
    freeze, reject, delayed = False, False, False
    state = State(connected=True, armed=True)
    tilted = False
    safety = SysStatus(sensors_present=1 << 15, sensors_enabled=1 << 15)

    def command(req, res):
        nonlocal pwm, rpm
        commands.append((req.command, req.param1, req.param2))
        assert req.command == 183 and req.param1 == 3
        res.success, res.result = not reject, 4 if reject else 0
        if not reject:
            pwm = int(req.param2)
            rpm = 0 if pwm == 1500 else 500  # unsigned legacy; no sign inference
        return res

    def arm(req, res):
        authority.append(("arm", req.value))
        res.success = True
        return res

    def mode(req, res):
        authority.append(("mode", req.custom_mode))
        res.mode_sent = True
        return res

    services = [node.create_service(CommandLong, "/mavros/cmd/command", command),
                node.create_service(CommandBool, "/mavros/cmd/arming", arm),
                node.create_service(SetMode, "/mavros/set_mode", mode)]
    pubs = [node.create_publisher(State, "/mavros/state", 10),
            node.create_publisher(SysStatus, "/mavros/sys_status", 10),
            node.create_publisher(EscObservation, "/mavros/esc_wheel_odometry/esc_observation", 10),
            node.create_publisher(RCOut, "/mavros/rc/out", 10)]
    raw_pub = node.create_publisher(Mavlink, "/uas1/mavlink_source", 10)
    outgoing = node.create_publisher(Mavlink, "/uas1/mavlink_sink", 10)
    imu_pub = node.create_publisher(Imu, "/mavros/imu/data", 10)

    def frame(msgid, payload):
        m = Mavlink(framing_status=Mavlink.FRAMING_OK, magic=Mavlink.MAVLINK_V20,
                    len=len(payload), sysid=1, compid=1, msgid=msgid)
        padded = payload + bytes((-len(payload)) % 8)
        m.payload64 = list(struct.unpack('<' + 'Q' * (len(padded) // 8), padded))
        return m
    subscription = node.create_subscription(Status, "/hardware_bridge/status", statuses.append, 10)

    def publish():
        nonlocal count
        if not freeze:
            count += 1
        stamp = node.get_clock().now().to_msg()
        esc_stamp = node.get_clock().now().to_msg()
        if delayed:
            esc_stamp.sec -= 2
        pubs[0].publish(state)
        pubs[1].publish(safety)
        pubs[2].publish(EscObservation(header=Header(stamp=esc_stamp), metadata_stamp=esc_stamp,
            source=2, esc_index=2, valid=True, rpm=rpm, rpm_valid=True,
            count=count, count_valid=True, current=0.8, current_valid=True,
            temperature=32.0, temperature_valid=True))
        pubs[3].publish(RCOut(header=Header(stamp=stamp), channels=[1500, 1500, pwm]))
        imu_pub.publish(Imu(orientation=Quaternion(x=.70710678 if tilted else 0.,
                                                   w=.70710678 if tilted else 1.)))

    timer = node.create_timer(.04, publish)
    mower = node.create_client(MowerControl, "/hardware_bridge/mower_control")
    emergency = node.create_client(EmergencyStop, "/hardware_bridge/emergency_stop")

    def wait(pred, timeout=8):
        end = time.monotonic() + timeout
        while not pred() and time.monotonic() < end:
            rclpy.spin_once(node, timeout_sec=.02)
        assert pred(), "blade graph timeout"

    def spin(sec):
        end = time.monotonic() + sec
        while time.monotonic() < end:
            rclpy.spin_once(node, timeout_sec=.02)

    def call(client, req):
        f = client.call_async(req)
        wait(f.done)
        return f.result()

    def mow(enabled, direction=0):
        return call(mower, MowerControl.Request(mow_enabled=enabled, mow_direction=direction)).success

    def neutral_reconnect():
        nonlocal rpm, pwm, freeze, reject
        state.connected = False
        freeze = reject = False
        rpm, pwm = 0, 1500
        spin(.3)
        state.connected = True
        spin(1.5)

    with tempfile.TemporaryDirectory(prefix="blade-graph-") as directory:
        with open(Path(directory) / "bridge.log", "w+") as log:
            process = subprocess.Popen([args.bridge_executable], stdout=log, stderr=log,
                env=dict(os.environ, MAVROS_RESOLVED_FIRMWARE="ardupilot"))
            try:
                wait(lambda: mower.service_is_ready() and emergency.service_is_ready())
                wait(lambda: (183, 3., 1500.) in commands)
                spin(1.5)
                initial = len(commands)
                assert mow(False)
                for _ in range(10):
                    assert mow(False)
                    spin(.1)  # BT repetition at 10 Hz, not just a tight loop.
                assert len(commands) == initial
                assert authority == []
                # No FCU ARM owner is introduced by repeated unauthorized ON.
                state.armed = False
                spin(.2)
                denied_baseline = len(commands)
                for _ in range(10):
                    assert not mow(True)
                    spin(.1)
                assert len(commands) == denied_baseline
                assert authority == []
                state.armed = True
                spin(1.5)
                for step, (direction, target) in enumerate(((0, 1450), (1, 1550), (0, 1450))):
                    before = len(commands)
                    assert mow(True, direction)
                    expected = [(183, 3., float(target))]
                    if step > 0:
                        expected.insert(0, (183, 3., 1500.))
                    assert commands[before:] == expected
                    assert authority == []  # ON and inversion do not ARM/DISARM.
                    wait(lambda: statuses and statuses[-1].mow_enabled)
                    assert statuses[-1].blade_requested_direction == ("forward" if direction == 0 else "reverse")
                    assert statuses[-1].mower_motor_rpm == 500
                    assert statuses[-1].mower_esc_current == 0.8 or abs(statuses[-1].mower_esc_current - .8) < 1e-5
                    before = len(commands)
                    for _ in range(3):
                        assert mow(True, direction)
                    assert len(commands) == before
                assert mow(False)
                wait(lambda: statuses[-1].blade_requested_direction == "off" and not statuses[-1].mow_enabled)
                spin(1.5)
                assert mow(True)
                before = len(commands)
                reverse = mower.call_async(MowerControl.Request(mow_enabled=1, mow_direction=1))
                wait(lambda: (183, 3., 1500.) in commands[before:])
                node.destroy_service(services[0])
                services[0] = None
                spin(.3)  # Discover actual service disappearance before OFF.
                assert not mow(False)  # Cannot claim ACK through absent transport.
                wait(reverse.done)
                assert not reverse.result().success
                spin(1.5)
                services[0] = node.create_service(CommandLong, "/mavros/cmd/command", command)
                wait(lambda: commands[-1] == (183, 3., 1500.))
                spin(.4)
                assert (183, 3., 1550.) not in commands[before:]
                assert mow(False)
                assert authority == []
                spin(1.5)
                assert mow(True)
                before = len(commands)
                assert call(emergency, EmergencyStop.Request(emergency=1)).success
                wait(lambda: (183, 3., 1500.) in commands[before:])
                assert not mow(True)
                assert ("mode", "HOLD") in authority and ("arm", False) in authority
                assert call(emergency, EmergencyStop.Request(emergency=0)).success
                spin(1.5)
                assert mow(True)
                freeze = True
                before = len(commands)
                wait(lambda: (183, 3., 1500.) in commands[before:])
                wait(lambda: statuses and not statuses[-1].mow_enabled)
                neutral_reconnect()
                delayed = True
                neutral_reconnect()
                assert not mow(True)  # Advancing counters do not freshen a 2s-old acquisition.
                delayed = False
                neutral_reconnect()
                reject = True
                assert not mow(True)  # actual result4, not enqueue=true
                reject = False
                spin(.3)
                assert not mow(True)  # failure retained, not retried at each tick
                neutral_reconnect()
                assert mow(True, 1)
                before = len(commands)
                safety.sensors_enabled = 0
                wait(lambda: (183, 3., 1500.) in commands[before:])
                assert not mow(True)
                safety.sensors_enabled = 1 << 15
                neutral_reconnect()
                assert mow(False)
                assert mow(True)
                spin(2.2)  # no outstanding own-wire expectation
                before = len(commands)
                outgoing.publish(frame(76, struct.pack('<7fHBBB',3.,1450.,0.,0.,0.,0.,0.,183,1,1,0)))
                wait(lambda: (183,3.,1500.) in commands[before:])
                spin(1.5)
                assert mow(True)
                before = len(commands)
                raw_pub.publish(frame(257, struct.pack('<IIB',100,90,3)))
                wait(lambda: (183,3.,1500.) in commands[before:])
                assert not mow(True)
                raw_pub.publish(frame(257, struct.pack('<IIB',101,91,0)))
                spin(.3)
                assert call(emergency, EmergencyStop.Request(emergency=0)).success
                neutral_reconnect()
                assert mow(True)
                before = len(commands)
                tilted = True
                wait(lambda: (183,3.,1500.) in commands[before:])
                assert not mow(True)
                tilted = False
                spin(.3)
                assert call(emergency, EmergencyStop.Request(emergency=0)).success
                neutral_reconnect()
                assert mow(False)
                print("PASS blade startup/OFF/FWD/REV/inversion/safety/stale/reject/reconnect/status")
            finally:
                process.send_signal(signal.SIGINT)
                process.wait(timeout=5)
                if process.returncode not in (0, -signal.SIGINT):
                    log.seek(0)
                    print(log.read())
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
