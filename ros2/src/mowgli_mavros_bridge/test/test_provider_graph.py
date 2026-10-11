#!/usr/bin/env python3
"""Existing ROS interfaces, real bridge, software-only FCU service mocks."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bridge-executable', required=True)
    parser.add_argument('--provider', choices=['ardupilot', 'px4'])
    parser.add_argument('--mowing-enabled', default='true')
    args = parser.parse_args()
    if args.provider is None:
        for provider in ('ardupilot', 'px4'):
            for enabled in ('true', 'false'):
                subprocess.run([os.sys.executable, __file__, '--bridge-executable', args.bridge_executable,
                                '--provider', provider, '--mowing-enabled', enabled],
                    env=dict(os.environ, MAVROS_RESOLVED_FIRMWARE=provider,
                             ROS_DOMAIN_ID='198', ROS_AUTOMATIC_DISCOVERY_RANGE='LOCALHOST'),
                    check=True, timeout=65)
        return

    import rclpy
    from std_msgs.msg import Header
    from geometry_msgs.msg import TwistStamped
    from diagnostic_msgs.msg import DiagnosticArray
    from mavros_msgs.msg import ManualControl, PositionTarget, State, SysStatus
    from mavros_msgs.srv import CommandBool, CommandLong, SetMode
    from mavros_esc_wheel_odometry.msg import EscObservation
    from mowgli_interfaces.msg import HighLevelStatus
    from mowgli_interfaces.srv import EmergencyStop, MowerControl
    from std_srvs.srv import Trigger

    rclpy.init()
    node = rclpy.create_node('provider_session_mock')
    requests, manual, guided, diagnostics = [], [], [], []
    state = State(connected=True, armed=False, mode='HOLD')
    hl = HighLevelStatus(state=1)
    confirm, reject_mode, reject_arm, reject_blade = True, False, False, False
    count = 0
    def mode(req, res):
        requests.append(('mode', req.custom_mode))
        res.mode_sent = not reject_mode
        if confirm and not reject_mode:
            state.mode = req.custom_mode
        return res
    def arm(req, res):
        requests.append(('arm', req.value))
        res.success, res.result = not reject_arm, 4 if reject_arm else 0
        if confirm and not reject_arm:
            state.armed = req.value
        return res
    def command(req, res):
        requests.append(('command', req.command, req.param1, req.param2))
        refused = reject_blade and req.command == 183 and req.param2 != 1500.
        res.success, res.result = not refused, 4 if refused else 0
        return res
    services = [node.create_service(SetMode, '/mavros/set_mode', mode),
                node.create_service(CommandBool, '/mavros/cmd/arming', arm),
                node.create_service(CommandLong, '/mavros/cmd/command', command)]
    subscriptions = [node.create_subscription(ManualControl, '/mavros/manual_control/send', manual.append, 100),
        node.create_subscription(PositionTarget, '/mavros/setpoint_raw/local', guided.append, 100),
        node.create_subscription(DiagnosticArray, '/diagnostics', diagnostics.append, 100)]
    vel_pub = node.create_publisher(TwistStamped, '/cmd_vel', 10)
    state_pub = node.create_publisher(State, '/mavros/state', 10)
    safety_pub = node.create_publisher(SysStatus, '/mavros/sys_status', 10)
    hl_pub = node.create_publisher(HighLevelStatus, '/behavior_tree_node/high_level_status', 10)
    esc_pub = node.create_publisher(EscObservation, '/mavros/esc_wheel_odometry/esc_observation', 10)
    safety = SysStatus(sensors_present=1 << 15, sensors_enabled=1 << 15)
    def publish():
        nonlocal count
        count += 1
        stamp = node.get_clock().now().to_msg()
        state.header.stamp = safety.header.stamp = stamp
        state_pub.publish(state)
        safety_pub.publish(safety)
        hl_pub.publish(hl)
        esc_pub.publish(EscObservation(header=Header(stamp=stamp), source=2, esc_index=2,
            valid=True, rpm=0, rpm_valid=True, count=count, count_valid=True))
    timer = node.create_timer(.04, publish)
    emergency = node.create_client(EmergencyStop, '/hardware_bridge/emergency_stop')
    mower = node.create_client(MowerControl, '/hardware_bridge/mower_control')
    reboot = node.create_client(Trigger, '/hardware_bridge/reboot_board')
    def spin(seconds=.2):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            rclpy.spin_once(node, timeout_sec=.02)
    def wait(pred, timeout=8):
        end = time.monotonic() + timeout
        while not pred() and time.monotonic() < end:
            spin(.04)
        assert pred(), 'provider mock timeout'
    def call(client, req):
        future = client.call_async(req)
        wait(future.done)
        return future.result()
    def velocity(x=.2, yaw=-.35):
        msg = TwistStamped()
        msg.header.stamp = node.get_clock().now().to_msg()
        msg.twist.linear.x, msg.twist.angular.z = x, yaw
        vel_pub.publish(msg)
        spin(.1)
    def idle():
        hl.state = 1
        wait(lambda: state.mode == 'HOLD' and not state.armed)
        spin(.2)
    def values():
        matches = [s for array in diagnostics for s in array.status
                   if s.name == 'mowgli_mavros_bridge/fcu_session']
        return matches[-1] if matches else None

    with tempfile.TemporaryDirectory(prefix='provider-session-') as directory:
        params = Path(directory) / 'params.yaml'
        params.write_text(json.dumps({'hardware_bridge': {'ros__parameters':
            {'mowing_enabled': args.mowing_enabled == 'true'}}}))
        with open(Path(directory) / 'bridge.log', 'w+') as log:
            process = subprocess.Popen([args.bridge_executable, '--ros-args', '--params-file', str(params)],
                stdout=log, stderr=log)
            try:
                wait(lambda: emergency.service_is_ready() and mower.service_is_ready())
                spin(1.5)
                initial = len(requests)
                velocity()
                assert manual[-1].y == manual[-1].z == 0
                spin(.2)
                assert len(requests) == initial  # idle never arms on velocity
                before = len(manual)
                velocity(float('nan'), 0.)
                assert len(manual) == before
                guided_mode = 'GUIDED' if args.provider == 'ardupilot' else 'MANUAL'
                confirm = False
                hl.state = 2
                wait(lambda: ('mode', guided_mode) in requests)
                assert ('arm', True) not in requests
                velocity()
                assert manual[-1].z == 0
                state.mode = guided_mode
                wait(lambda: ('arm', True) in requests)
                # ACK is not ARM confirmation; no nonneutral setpoint is sent.
                velocity()
                assert manual[-1].z == 0
                old_requests = len(requests)
                state.armed = True
                spin(.2)
                velocity()
                if args.provider == 'ardupilot':
                    target = guided[-1]
                    assert target.coordinate_frame == PositionTarget.FRAME_BODY_NED
                    assert target.type_mask == 0x05E7
                    assert abs(target.velocity.x - .2) < 1e-6
                    assert abs(target.yaw_rate + .35) < 1e-6
                    assert all(m.y == m.z == 0 for m in manual)
                else:
                    assert manual[-1].y == -350 and manual[-1].z == 200
                spin(.3)
                assert len(requests) == old_requests
                confirm = True
                idle()
                for target in (2, 3, 4):
                    hl.state = target
                    wait(lambda: state.mode == (guided_mode if target == 2 else 'MANUAL') and state.armed)
                    spin(.1)
                    velocity()
                    if target == 2 and args.provider == 'ardupilot':
                        assert abs(guided[-1].velocity.x - .2) < 1e-6
                    else:
                        assert manual[-1].y == -350 and manual[-1].z == 200
                    before = requests.count(('arm', False))
                    off = call(mower, MowerControl.Request(mow_enabled=0))
                    assert off.success == (args.provider == 'ardupilot')
                    assert requests.count(('arm', False)) == before  # wheel session persists
                    idle()
                # Direct backend MowerControl ON (not the actual GUI chain) tests temporary HOLD/ARM.
                if args.provider == 'ardupilot' and args.mowing_enabled == 'true':
                    spin(1.5)
                    confirm = False
                    before = len(requests)
                    on = mower.call_async(MowerControl.Request(mow_enabled=1, mow_direction=0))
                    wait(lambda: ('arm', True) in requests[before:])
                    spin(.2)
                    assert not on.done()
                    assert ("command", 183, 3., 1450.) not in requests[before:]
                    velocity()
                    assert manual[-1].y == manual[-1].z == 0
                    state.armed = True
                    wait(on.done)
                    assert on.result().success and state.mode == 'HOLD'
                    velocity()
                    assert manual[-1].y == manual[-1].z == 0
                    assert guided[-1].velocity.x == guided[-1].yaw_rate == 0
                    confirm = True
                    assert call(mower, MowerControl.Request(mow_enabled=0)).success
                    wait(lambda: not state.armed)
                    # OFF cancels an ON waiting for ARM, including late ARM.
                    spin(1.5)
                    confirm = False
                    before = len(requests)
                    on = mower.call_async(MowerControl.Request(mow_enabled=1, mow_direction=1))
                    wait(lambda: ('arm', True) in requests[before:])
                    off = mower.call_async(MowerControl.Request(mow_enabled=0))
                    wait(off.done); wait(on.done)
                    assert off.result().success and not on.result().success
                    state.armed = True
                    confirm = True
                    wait(lambda: not state.armed)
                    assert ('command', 183, 3., 1550.) not in requests[before:]
                    # Refused isolated ON releases temporary ARM; a late FCU
                    # ARM never resurrects the cancelled servo command.
                    for failure in ('refused', 'expired'):
                        spin(1.5)
                        confirm = False
                        reject_arm = failure == 'refused'
                        before = len(requests)
                        on = mower.call_async(MowerControl.Request(mow_enabled=1, mow_direction=0))
                        wait(lambda: ('arm', True) in requests[before:])
                        wait(on.done)
                        assert not on.result().success, failure
                        snapshot = requests.count(('arm', True))
                        spin(.3)
                        assert requests.count(('arm', True)) == snapshot
                        reject_arm = False
                        state.armed = True  # an unconfirmable/refused ARM can arrive late
                        confirm = True
                        wait(lambda: not state.armed)
                        spin(.2)
                        assert ('command', 183, 3., 1450.) not in requests[before:]
                        assert requests.count(('arm', True)) == snapshot
                    # A servo refusal releases an isolated test but preserves
                    # the movement session's ARM in both MANUAL and GUIDED.
                    for movement in (1, 4, 2):
                        hl.state = movement
                        if movement != 1:
                            wait(lambda: state.armed and state.mode ==
                                 ('MANUAL' if movement == 4 else 'GUIDED'))
                        spin(1.5)
                        reject_blade = True
                        before = requests.count(('arm', False))
                        command_before = len(requests)
                        result = call(mower, MowerControl.Request(mow_enabled=1))
                        assert not result.success
                        assert ('command', 183, 3., 1450.) in requests[command_before:], movement
                        wait(lambda: ('command', 183, 3., 1500.) in requests[command_before:])
                        if movement == 1:
                            wait(lambda: not state.armed)
                        else:
                            spin(.3)
                            assert state.armed and requests.count(('arm', False)) == before
                        reject_blade = False
                        off = call(mower, MowerControl.Request(mow_enabled=0))
                        # BladeControl keeps an active-session servo refusal
                        # latched; OFF does not disguise that existing fault.
                        assert off.success == (movement == 1), movement
                        idle()

                else:
                    before = len(requests)
                    assert not call(mower, MowerControl.Request(mow_enabled=1)).success
                    assert requests[before:] == []
                # Safety DISARM preempts unconfirmed MODE immediately.
                confirm = False
                hl.state = 2
                before = len(requests)
                wait(lambda: ('mode', guided_mode) in requests[before:])
                assert call(emergency, EmergencyStop.Request(emergency=1)).success
                wait(lambda: ('arm', False) in requests[before:])
                assert requests.index(('arm', False), before) > requests.index(('mode', guided_mode), before)
                state.mode, state.armed = 'HOLD', False
                confirm = True
                spin(.2)
                velocity()
                assert manual[-1].y == manual[-1].z == 0
                # Clear uses the still-active MowgliNext demand, without a new action.
                assert call(emergency, EmergencyStop.Request(emergency=0)).success
                wait(lambda: state.mode == guided_mode and state.armed)
                before = len(requests)
                spin(.4)
                assert len(requests) == before
                # Explicit disconnect retains that demand on reconnection.
                state.connected = False
                spin(.2)
                state.mode, state.armed, state.connected = 'HOLD', False, True
                wait(lambda: state.mode == guided_mode and state.armed)
                idle()
                # Refused ARM is diagnosed and never retried by repeated status.
                reject_arm = True
                before = requests.count(('arm', True))
                hl.state = 4
                wait(lambda: state.mode == 'MANUAL')
                wait(lambda: requests.count(('arm', True)) > before)
                wait(lambda: values() is not None and values().message == 'arming:request_refused')
                before = requests.count(('arm', True))
                spin(.5)
                assert requests.count(('arm', True)) == before
                velocity()
                assert manual[-1].z == 0
                reject_arm = False
                idle()
                # Refused MODE blocks ARM and does not create a retry storm.
                reject_mode = True
                hl.state = 2
                wait(lambda: values() is not None and values().message == 'mode:request_refused')
                before = len(requests)
                spin(.4)
                assert len(requests) == before
                reject_mode = False
                idle()
                # Missing HOLD endpoint cannot block safety DISARM forwarding.
                hl.state = 4
                wait(lambda: state.mode == 'MANUAL' and state.armed)
                node.destroy_service(services[0])
                spin(.4)
                confirm = False
                before = len(requests)
                assert call(emergency, EmergencyStop.Request(emergency=1)).success
                wait(lambda: ('arm', False) in requests[before:])
                wait(lambda: values() is not None and
                     dict((v.key, v.value) for v in values().values).get('command_acknowledged') == 'true')
                info = dict((v.key, v.value) for v in values().values)
                assert info['confirmed_arm'] == 'true' and info['requested_arm'] == 'false'
                assert info['state_confirmed'] == 'false' and values().message == 'waiting_fcu'
                assert ('mode', 'HOLD') not in requests[before:]
                state.armed = False
                spin(.2)
                # Existing reboot service remains a command246 forwarding path.
                assert call(reboot, Trigger.Request()).success
                wait(lambda: ('command', 246, 1., 0.) in requests)
                print(f'PASS provider={args.provider} mowing_enabled={args.mowing_enabled}: '
                      'idle/transitions/ARM/FCU-confirmation/GUIDED/blade-test/safety/refusal/reconnect')
            except BaseException:
                log.flush(); log.seek(0); print(log.read())
                raise
            finally:
                process.send_signal(signal.SIGINT)
                process.wait(timeout=5)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
