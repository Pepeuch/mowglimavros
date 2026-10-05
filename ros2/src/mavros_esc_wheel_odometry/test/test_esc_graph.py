#!/usr/bin/env python3
"""Real pinned MAVROS handlers + bridge, synthetic decoded frames, no FCU/device."""
import argparse
import copy
import json
import math
import os
from pathlib import Path
import signal
import struct
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bridge-executable', required=True)
    args = parser.parse_args()
    import rclpy
    from ament_index_python.packages import get_package_prefix
    from diagnostic_msgs.msg import DiagnosticArray
    from mavros_msgs.msg import Mavlink, State
    from mavros_esc_wheel_odometry.msg import EscObservation
    from mowgli_interfaces.msg import Status
    from nav_msgs.msg import Odometry
    from rcl_interfaces.srv import SetParametersAtomically
    from rclpy.parameter import Parameter
    from rclpy.qos import qos_profile_sensor_data

    rclpy.init()
    node = rclpy.create_node('esc_feature_contract_test')
    odom, observations, status, state, diagnostics = [], [], [], [], []
    esc_qos = copy.copy(qos_profile_sensor_data)
    esc_qos.depth = 64
    subscriptions = [
        node.create_subscription(Odometry, '/wheel_odom', odom.append, 20),
        node.create_subscription(EscObservation, '/mavros/esc_wheel_odometry/esc_observation', observations.append, esc_qos),
        node.create_subscription(Status, '/hardware_bridge/status', status.append, 20),
        node.create_subscription(State, '/mavros/state', state.append, 20),
        node.create_subscription(DiagnosticArray, '/diagnostics', diagnostics.append, 20),
    ]
    publisher = node.create_publisher(Mavlink, '/uas1/mavlink_source', 100)
    wheel_parameters = node.create_client(SetParametersAtomically, '/mavros/esc_wheel_odometry/set_parameters_atomically')
    bridge_parameters = node.create_client(SetParametersAtomically, '/hardware_bridge/set_parameters_atomically')

    def frame(msgid, payload, component=1, system=1):
        m = Mavlink()
        m.framing_status = Mavlink.FRAMING_OK
        m.magic = 253; m.len = len(payload); m.msgid = msgid; m.sysid = system; m.compid = component
        padded = payload + bytes((-len(payload)) % 8)
        m.payload64 = list(struct.unpack('<' + 'Q' * (len(padded) // 8), padded))
        return m

    heartbeat = frame(0, struct.pack('<IBBBBB', 0, 10, 3, 0, 4, 3))
    last_heartbeat = [0.0]
    heartbeat_enabled = [True]

    def spin(seconds=0.15):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            if heartbeat_enabled[0] and time.monotonic() - last_heartbeat[0] > 0.25:
                publisher.publish(heartbeat); last_heartbeat[0] = time.monotonic()
            rclpy.spin_once(node, timeout_sec=0.025)

    def wait(predicate, timeout=7):
        end = time.monotonic() + timeout
        while not predicate() and time.monotonic() < end: spin(0.05)
        assert predicate(), 'timeout waiting for ESC graph condition'

    def send(m):
        # Repeat same measurement to prove it is not republished/refreshed.
        for _ in range(3): publisher.publish(m); spin(0.08)
        spin(0.1)

    def esc(us, rpm=(-60, -60, -2450, 0), index=0, component=1):
        return frame(291, struct.pack('<Q4i4f4fB', us, *rpm, 24, 25, 26, 0, 1, 2, 4.5, 0, index), component)

    def info(us, count=3, temp=2534, index=0, component=1):
        return frame(290, struct.pack('<Q4IH4H4hBBBB', us, 10, 11, 12, 13, 1,
                                     0, 0, 2, 0, temp, temp, temp, temp, index, count, 0, 0), component)

    def distance(us, left, right, component=201, system=1):
        return frame(9000, struct.pack('<Q16dB', us, left, right, *([0.0] * 14), 2), component, system)

    def configure(client, values):
        wait(client.service_is_ready)
        params = [Parameter(name, value=value).to_parameter_msg() for name, value in values.items()]
        future = client.call_async(SetParametersAtomically.Request(parameters=params))
        wait(future.done)
        return future.result().result.successful

    with tempfile.TemporaryDirectory(prefix='esc-native-graph-') as directory:
        params = Path(directory) / 'mavros.yaml'
        params.write_text(json.dumps({
            '/**': {'ros__parameters': {
                'plugin_allowlist': ['sys_status', 'esc_status', 'esc_telemetry', 'esc_wheel_odometry'],
                'fcu_url': 'udp://127.0.0.1:19880@127.0.0.1:19881',
                'gcs_url': '', 'system_id': 255, 'tgt_system': 1, 'tgt_component': 1,
            }},
            '/**/esc_wheel_odometry': {'ros__parameters': {
                'source': 'auto', 'left_esc_slot': 1, 'right_esc_slot': 0,
                'left_rpm_instance': 1, 'right_rpm_instance': 2,
                'expected_esc_telem_mav_offset': 0,
                'left_wheel_radius_m': 0.1, 'right_wheel_radius_m': 0.1, 'track_width_m': 0.5,
                'left_wheel_index': 0, 'right_wheel_index': 1,
            }},
        }))
        mavros = str(Path(get_package_prefix('mavros')) / 'lib/mavros/mavros_node')
        logs = [open(Path(directory)/'mavros.log', 'w+'), open(Path(directory)/'bridge.log', 'w+')]
        processes = []
        try:
            processes.append(subprocess.Popen([mavros, '--ros-args', '--params-file', str(params)], stdout=logs[0], stderr=subprocess.STDOUT))
            processes.append(subprocess.Popen([args.bridge_executable], stdout=logs[1], stderr=subprocess.STDOUT))
            wait(lambda: state and state[-1].connected)
            wait(lambda: publisher.get_subscription_count() > 0)
            wait(lambda: node.count_publishers('/wheel_odom') == 1)

            # COMMON status alone: direction works independently of stock INFO count.
            before = len(odom); send(esc(1000000)); wait(lambda: len(odom) == before + 1)
            assert odom[-1].twist.twist.linear.x < 0
            assert odom[-1].header.frame_id == 'odom' and odom[-1].child_frame_id == 'base_link'
            assert math.isclose(odom[-1].twist.covariance[0], 0.01)
            assert odom[-1].pose.covariance[0] == 1e6
            wait(lambda: status and status[-1].mower_motor_rpm == 2450)
            assert status[-1].mower_esc_current == 4.5 and math.isnan(status[-1].mower_esc_temperature)
            assert status[-1].blade_status_stamp.sec > 0
            blade = [m for m in observations if m.esc_index == 2][-1]
            assert blade.rpm == -2450 and blade.rpm_direction_valid and not blade.temperature_valid
            assert math.isfinite(blade.temperature)  # Internal invalid flag, not sentinel.
            send(info(1000000)); wait(lambda: status and math.isclose(status[-1].mower_esc_temperature, 25.34, abs_tol=1e-4))
            assert len(odom) == before + 1, 'INFO must never publish wheel odometry'
            # Wire COMPID reaches both handlers; foreign COMMON data cannot enrich/replace owner.
            send(esc(1100000, rpm=(500, 500, -9000, 0), component=42))
            send(info(1100000, temp=9000, component=42))
            assert len(odom) == before + 1
            assert status[-1].mower_motor_rpm == 2450
            assert math.isclose(status[-1].mower_esc_temperature, 25.34, abs_tol=1e-4)
            assert not configure(wheel_parameters, {'common_pair_max_skew_s': 3.1})
            assert not configure(wheel_parameters, {'common_pair_max_skew_s': 0.0})
            assert not configure(wheel_parameters, {'esc_component_id': 256})

            # Hall encoder: same SYSID, different COMPID; no ESC observations generated.
            before = len(odom); send(distance(1000000, 100, 100)); assert len(odom) == before
            send(distance(2000000, 101, 101)); wait(lambda: len(odom) == before + 1)
            assert math.isclose(odom[-1].twist.twist.linear.x, 1)
            assert all(m.esc_index < 4 for m in observations)
            before = len(odom); send(distance(3000000, 102, 102, system=2)); assert len(odom) == before
            send(distance(3000000, 100, 100)); wait(lambda: len(odom) == before + 1)
            assert math.isclose(odom[-1].twist.twist.linear.x, -1)
            before = len(odom); send(esc(2000000)); assert len(odom) == before, 'preferred distance suppresses lower source'

            # COMMON epochs are independent of the live ESP32 distance baseline.
            send(info(100))  # INFO rollback invalidates metadata only.
            send(esc(100))  # STATUS rollback clears COMMON only.
            assert len(odom) == before
            active = [s for d in diagnostics for s in d.status if s.name == 'mavros_esc_wheel_odometry/source']
            assert active[-1].message == 'wheel_distance'

            # Invalid atomic update leaves prior geometry/baseline intact.
            assert not configure(wheel_parameters, {'left_esc_slot': 4, 'right_esc_slot': 4})
            before = len(odom); send(distance(4000000, 99, 99)); wait(lambda: len(odom) == before + 1)
            assert math.isclose(odom[-1].twist.twist.linear.x, -1)
            assert configure(wheel_parameters, {'left_wheel_index': 1, 'right_wheel_index': 0, 'track_width_m': 1.0})
            before = len(odom); send(distance(5000000, 1000, 1000)); assert len(odom) == before
            send(distance(6000000, 1001, 999)); wait(lambda: len(odom) == before + 1)
            assert math.isclose(odom[-1].twist.twist.linear.x, 0)
            assert math.isclose(odom[-1].twist.twist.angular.z, 2)
            assert node.count_publishers('/wheel_odom') == 1

            # Alternate physical blade role; no format logic in bridge.
            assert configure(bridge_parameters, {'blade_esc_slot': 7})
            assert not configure(bridge_parameters, {'blade_esc_slot': 0})
            send(info(7000000, count=8, temp=3000, index=4))
            send(esc(7000000, rpm=(0, 0, 0, -3500), index=4))
            wait(lambda: status and status[-1].mower_motor_rpm == 3500)
            assert math.isclose(status[-1].mower_esc_temperature, 30)

            # Runtime component restriction rejects old owner; both handlers follow the new owner.
            assert configure(wheel_parameters, {'esc_component_id': 42, 'common_pair_max_skew_s': 0.1})
            send(info(8000000, count=8, temp=2900, index=4, component=42))
            send(esc(8000000, rpm=(0, 0, 0, -4500), index=4, component=42))
            wait(lambda: status and status[-1].mower_motor_rpm == 4500)
            assert math.isclose(status[-1].mower_esc_temperature, 29)
            send(esc(9000000, rpm=(0, 0, 0, -9999), index=4))
            assert status[-1].mower_motor_rpm == 4500

            # Explicit legacy fallback: unsigned telemetry cannot publish wheels.
            assert configure(wheel_parameters, {'source': 'ardupilot_legacy', 'left_esc_slot': 1, 'right_esc_slot': 0})
            def telemetry(count):
                return frame(11030, struct.pack('<4H4H4H4H4H4B',
                    2500, 2600, 2700, 0, 100, 200, 450, 0,
                    1000, 2000, 3000, 0, 65000, 65000, 2450, 0,
                    count, count, count, 0, 40, 41, 42, 0))
            before = len(odom); send(telemetry(10)); send(telemetry(11)); assert len(odom) == before
            send(frame(226, struct.pack('<ff', -60, -60))); wait(lambda: len(odom) == before + 1)
            assert math.isclose(odom[-1].twist.twist.linear.x, -0.2 * math.pi, abs_tol=1e-12)
            send(frame(226, struct.pack('<ff', -60, -60))); assert len(odom) == before + 1
            wait(lambda: any(m.source == m.SOURCE_ARDUPILOT_LEGACY and m.esc_index == 1 for m in observations))
            normal = [m for m in observations if m.source == m.SOURCE_ARDUPILOT_LEGACY and m.esc_index == 1][-1]
            assert normal.rpm == 65000 and not normal.rpm_direction_valid
            assert normal.voltage == 26 and normal.current == 2 and normal.temperature == 41
            assert normal.totalcurrent == 2 and normal.count == 11
            assert normal.totalcurrent_valid and normal.count_valid

            # Timer selects stale -> none; canonical flags expire without fake refresh.
            spin(3.6)
            active = [s for d in diagnostics for s in d.status if s.name == 'mavros_esc_wheel_odometry/source']
            assert active and active[-1].message == 'none'
            assert not [m for m in observations if m.esc_index == 1][-1].valid
            assert node.count_publishers('/wheel_odom') == 1
            # Real MAVROS connection callback clears encoder baselines.
            assert configure(wheel_parameters, {'source': 'wheel_distance'})
            before = len(odom); send(distance(10000000, 200, 200)); assert len(odom) == before
            send(distance(11000000, 201, 201)); wait(lambda: len(odom) == before + 1)
            heartbeat_enabled[0] = False
            wait(lambda: state and not state[-1].connected, timeout=16)
            before = len(odom); send(distance(12000000, 202, 202)); assert len(odom) == before
            heartbeat_enabled[0] = True
            wait(lambda: state and state[-1].connected)
            before = len(odom); before_esc = len(observations)
            send(distance(100, 0, 0)); assert len(odom) == before
            send(distance(1000100, 1, 1)); wait(lambda: len(odom) == before + 1)
            assert math.isclose(odom[-1].twist.twist.linear.x, 1)
            assert len(observations) == before_esc, 'Hall encoder must not become ESCObservation'
            print('PASS native ESC graph: COMMON signed/status/info, ESP32 component, no duplicate odom, dynamic mappings/geometry, legacy and stale')
        except BaseException:
            for log in logs:
                log.flush(); log.seek(0); print(log.read())
            raise
        finally:
            for process in processes: process.send_signal(signal.SIGINT)
            for process in processes:
                try: process.wait(timeout=4)
                except subprocess.TimeoutExpired: process.kill(); process.wait()
            for log in logs: log.close()
    node.destroy_node(); rclpy.shutdown()


if __name__ == '__main__': main()
