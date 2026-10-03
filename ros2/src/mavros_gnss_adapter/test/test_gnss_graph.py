#!/usr/bin/env python3
"""Isolated real-plugin GNSS regression; source the built workspace before running.

No serial device or robot connection. Each case runs a fresh local ROS domain
and feeds MAVROS's internal, already-decoded MAVLink input with synthetic frames.
"""
import argparse
import math
import os
from pathlib import Path
import signal
import struct
import subprocess
import sys
import tempfile
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bridge-executable")
    parser.add_argument("--mode", choices=("mavros", "direct"))
    parser.add_argument("--source", choices=("gps1", "gps2"), default="gps1")
    args = parser.parse_args()
    if args.mode is None:
        for index, (mode, source) in enumerate(
            (("mavros", "gps1"), ("mavros", "gps2"), ("direct", "gps1"), ("direct", "gps2"))
        ):
            env = dict(os.environ, ROS_DOMAIN_ID=str(211 + index),
                       ROS_AUTOMATIC_DISCOVERY_RANGE="LOCALHOST",
                       GNSS_SOURCE=mode, GNSS_MAVROS_SOURCE=source)
            command = [sys.executable, __file__, "--mode", mode, "--source", source]
            if args.bridge_executable:
                command.extend(["--bridge-executable", args.bridge_executable])
            subprocess.run(command, env=env, check=True, timeout=60)
        return

    import rclpy
    from ament_index_python.packages import get_package_prefix
    from mavros_msgs.msg import Mavlink
    from mowgli_interfaces.msg import GnssStatus as Status
    from rclpy.qos import qos_profile_sensor_data
    from sensor_msgs.msg import NavSatFix
    from universal_gnss_msgs.msg import GnssStatus as UniversalStatus

    def binary(package, executable):
        return str(Path(get_package_prefix(package)) / "lib" / package / executable)

    rclpy.init()
    node = rclpy.create_node("gnss_contract_test")
    statuses, fixes, private_statuses, private_fixes = [], [], [], []
    subscriptions = [
        node.create_subscription(Status, "/gps/status", statuses.append, 10),
        node.create_subscription(NavSatFix, "/gps/fix", fixes.append, 10),
        node.create_subscription(UniversalStatus, f"/mavros/universal_gnss/{args.source}/status",
                                 private_statuses.append, qos_profile_sensor_data),
        node.create_subscription(NavSatFix, f"/mavros/universal_gnss/{args.source}/fix",
                                 private_fixes.append, qos_profile_sensor_data),
    ]
    raw_pub = node.create_publisher(Mavlink, "/uas1/mavlink_source", 10)
    sequence = 0

    def spin(duration):
        until = time.monotonic() + duration
        while time.monotonic() < until:
            rclpy.spin_once(node, timeout_sec=0.02)

    def send(msgid, payload):
        nonlocal sequence
        msg = Mavlink()
        msg.framing_status = Mavlink.FRAMING_OK
        msg.magic = Mavlink.MAVLINK_V20
        msg.sysid = 1
        msg.compid = 1
        msg.seq = sequence % 256
        sequence += 1
        msg.msgid = msgid
        msg.len = len(payload)
        data = bytes(payload) + bytes((-len(payload)) % 8)
        msg.payload64 = list(struct.unpack("<" + "Q" * (len(data) // 8), data))
        raw_pub.publish(msg)

    def heartbeat():
        send(0, struct.pack("<IBBBBB", 0, 10, 3, 0, 3, 3))

    observation = 0

    def gps(source, fix_type=6, extensions=True, ellipsoid_bytes=None, height_override=None):
        nonlocal observation
        observation += 1
        payload = bytearray(52 if source == "gps1" else 57)
        # Distinct receiver tuples prove that source selection is exclusive.
        lat, lon, msl, height = ((439542000, 22022000, 180000, 230000)
                                 if source == "gps1" else (449542000, 32022000, 280000, 330000))
        if height_override is not None:
            height = height_override
        struct.pack_into("<Qiii", payload, 0, observation * 500000, lat, lon, msl)
        if source == "gps1":
            struct.pack_into("<HHHHBBiIIIIH", payload, 20,
                             54, 80, 0, 0, fix_type, 26, height, 10, 20, 0, 0, 0)
            if not extensions:
                payload = payload[:30]
            if ellipsoid_bytes is not None:
                payload = payload[:30 + ellipsoid_bytes]
            send(24, payload)
        else:
            struct.pack_into("<IHHHHBBBH", payload, 20, 1200, 54, 80, 0, 0, fix_type, 26, 1, 0)
            struct.pack_into("<iIIII", payload, 37, height, 10, 20, 0, 0)
            if not extensions:
                payload = payload[:35]
            if ellipsoid_bytes is not None:
                payload = payload[:37 + ellipsoid_bytes]
            send(124, payload)

    def rtk():
        payload = bytearray(35)
        payload[33] = 19
        send(127 if args.source == "gps1" else 128, payload)

    processes = []
    with tempfile.TemporaryDirectory(prefix="mm-gnss-graph-") as directory:
        config = Path(directory) / "plugins.yaml"
        config.write_text("/**:\n  ros__parameters:\n    plugin_allowlist: [sys_status, universal_gnss, mowgli_gnss]\n")
        logfile = Path(directory) / "runtime.log"
        with logfile.open("w") as log:
            env = dict(os.environ, GNSS_SOURCE=args.mode, GNSS_MAVROS_SOURCE=args.source)
            try:
                processes.append(subprocess.Popen(
                    [binary("mavros", "mavros_node"), "--ros-args", "--params-file", str(config),
                     "-p", "fcu_url:=udp://127.0.0.1:0@127.0.0.1:19999", "-p", "gcs_url:=''"],
                    stdout=log, stderr=log, env=env, start_new_session=True))
                processes.append(subprocess.Popen(
                    [args.bridge_executable or binary("mowgli_mavros_bridge", "mavros_hardware_bridge_node")],
                    stdout=log, stderr=log, env=env, start_new_session=True))
                deadline = time.monotonic() + 15
                while raw_pub.get_subscription_count() == 0 and time.monotonic() < deadline:
                    assert all(p.poll() is None for p in processes), logfile.read_text()
                    spin(0.1)
                assert raw_pub.get_subscription_count() > 0, logfile.read_text()
                for _ in range(6):
                    heartbeat()
                    spin(0.15)
                # Wait for DDS endpoint discovery, then verify endpoint ownership.
                spin(1.0)
                expected = 1 if args.mode == "mavros" else 0
                for topic in ("/gps/status", "/gps/fix"):
                    publishers = node.get_publishers_info_by_topic(topic)
                    assert len(publishers) == expected, (topic, publishers, logfile.read_text())
                    if publishers:
                        assert publishers[0].node_name == "mowgli_gnss", publishers
                subscribers = node.get_subscriptions_info_by_topic("/gps/status")
                assert any(info.node_name == "hardware_bridge" for info in subscribers), subscribers
                for _ in range(4):
                    heartbeat()
                    gps("gps1")
                    gps("gps2")
                    spin(0.25)
                assert private_statuses and private_fixes, logfile.read_text()
                if args.mode == "direct":
                    assert not statuses and not fixes
                    print(f"PASS {args.mode}/{args.source}: zero canonical publishers; private GNSS active")
                    return
                assert fixes and statuses, (len(fixes), len(statuses), logfile.read_text())
                latest = statuses[-1]
                assert latest.fix_valid and latest.fix_type == Status.FIX_TYPE_RTK_FIXED, latest
                assert latest.rtk_mode == Status.RTK_MODE_FIXED, latest
                assert latest.differential_corrections and latest.corrections_active, latest
                assert math.isclose(latest.hdop, 0.54, abs_tol=1e-5), latest
                assert math.isclose(latest.vdop, 0.8, abs_tol=1e-5), latest
                assert math.isclose(latest.horizontal_accuracy_m, 0.01, abs_tol=1e-5), latest
                assert latest.satellites_visible == 26, latest
                assert latest.position_observation_sequence == private_statuses[-1].position_observation_sequence
                assert all(f.altitude == (230.0 if args.source == "gps1" else 330.0) for f in fixes), fixes
                assert private_fixes[-1].altitude == (180.0 if args.source == "gps1" else 280.0)
                before = len(fixes)
                other = "gps2" if args.source == "gps1" else "gps1"
                gps(other)
                spin(0.25)
                assert len(fixes) == before, "unselected receiver published a canonical fix"
                seq = statuses[-1].position_observation_sequence
                rtk()
                spin(0.15)
                assert statuses[-1].satellites_used == 19, statuses[-1]
                assert statuses[-1].position_observation_sequence == seq
                # RTK updates and heartbeats cannot refresh an old position.
                for _ in range(13):
                    heartbeat()
                    rtk()
                    spin(0.25)
                assert not statuses[-1].fix_valid, statuses[-1]
                assert statuses[-1].position_observation_sequence == seq
                assert len(fixes) == before
                gps(args.source, fix_type=1)
                spin(0.25)
                assert not statuses[-1].fix_valid
                assert len(fixes) == before
                gps(args.source, extensions=False)
                spin(0.25)
                assert len(fixes) == before + 1
                assert math.isnan(fixes[-1].altitude), "missing ellipsoid extension leaked MSL/zero"
                for field_bytes in range(5):
                    count = len(fixes)
                    gps(args.source, ellipsoid_bytes=field_bytes)
                    spin(0.25)
                    assert len(fixes) == count + 1
                    if field_bytes < 4:
                        assert math.isnan(fixes[-1].altitude), (
                            "partially present ellipsoid field published height", field_bytes)
                    else:
                        assert fixes[-1].altitude == (230.0 if args.source == "gps1" else 330.0)
                # A full payload with later nonzero accuracy extensions still
                # does not prove that an all-zero altitude field was populated.
                count = len(fixes)
                gps(args.source, height_override=0)
                spin(0.25)
                assert len(fixes) == count + 1
                assert math.isnan(fixes[-1].altitude), "zero extension published 0 metres"
                gps(args.source)
                spin(0.25)
                assert statuses[-1].fix_valid
                assert fixes[-1].altitude == (230.0 if args.source == "gps1" else 330.0)
                print(f"PASS {args.mode}/{args.source}: ownership, source, mapping, sequence, RTK/stale/no-fix, ellipsoid")
            except BaseException:
                print(logfile.read_text(), file=sys.stderr)
                raise
            finally:
                for process in processes:
                    if process.poll() is None:
                        os.killpg(process.pid, signal.SIGINT)
                for process in processes:
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        os.killpg(process.pid, signal.SIGKILL)
                        process.wait()
                for subscription in subscriptions:
                    node.destroy_subscription(subscription)
                node.destroy_node()
                rclpy.shutdown()


if __name__ == "__main__":
    main()
