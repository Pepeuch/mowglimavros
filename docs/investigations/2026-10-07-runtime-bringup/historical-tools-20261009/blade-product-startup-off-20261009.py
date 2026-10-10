import sys, time, struct, json
sys.path.insert(0, '/audit-old')
import mowgli_blade_charge_ab as old
import rclpy
from mavros_msgs.msg import Mavlink
from mowgli_interfaces.srv import MowerControl
from mowgli_interfaces.msg import Status

old_log = old.log
old.log = lambda event, **kw: old_log(event, **kw) if event != 'esc' else None
rclpy.init()
b = old.Bench('blade_product_startup_off_audit')
b.subscribe()
commands = []
generation = [None]
def status_generation(m, info):
    gid = info['publisher_gid'] if isinstance(info,dict) else info.publisher_gid
    generation[0] = json.dumps(gid, default=str, sort_keys=True)
status_sub = b.n.create_subscription(Status, '/hardware_bridge/status', status_generation, old.qos_profile_sensor_data)

def outgoing(m):
    if m.msgid != 76 or m.framing_status != Mavlink.FRAMING_OK:
        return
    p = b''.join(struct.pack('<Q',v) for v in m.payload64)[:m.len]
    values = struct.unpack('<7fHBBB',p.ljust(33,b'\0')[:33])
    if values[7] in (183,400):
        record = dict(wall=time.time(),command=values[7],p1=values[0],p2=values[1])
        commands.append(record)
        old_log('outgoing',**record)

sub = b.n.create_subscription(Mavlink,'/uas1/mavlink_sink',outgoing,old.qos_profile_sensor_data)
c = b.n.create_client(MowerControl,'/hardware_bridge/mower_control')
try:
    b.spin(3,False)
    initial_generation = generation[0]
    assert initial_generation, 'initial bridge publisher missing'
    old_log('observer_ready',generation=initial_generation)
    end = time.monotonic()+120
    while time.monotonic()<end:
        b.spin(.05,False)
        status = b.last.get('status',(0,None))[1]
        if generation[0]!=initial_generation and status and status.blade_requested_direction=='off':
            old_log('new_generation',generation=generation[0])
            break
    else:
        raise RuntimeError('new generation startup status not observed')
    # A new bridge publishes OFF while MAVROS is still disconnected. Do not
    # mistake that startup intention for a connected, settled FCU generation.
    stable_since = None
    deadline = time.monotonic()+20
    while time.monotonic()<deadline:
        b.spin(.05,False)
        try:
            state=b.get('state',2)
            ready=state.connected and not state.armed and state.mode=='MANUAL'
            ready=ready and list(b.get('out',2).channels[:3])==[1500]*3
            ready=ready and all(b.get('esc'+str(i),1).valid and b.get('esc'+str(i),1).rpm_valid and b.get('esc'+str(i),1).rpm==0 for i in range(3))
        except RuntimeError:
            ready=False
        if ready:
            if stable_since is None: stable_since=time.monotonic()
            if time.monotonic()-stable_since>=2: break
        else: stable_since=None
    else: raise RuntimeError('startup FCU did not settle disarmed/neutral/zero')
    state=b.get('state',2)
    assert state.connected and not state.armed and state.mode=='MANUAL'
    assert list(b.get('out',2).channels[:3])==[1500]*3
    assert all(b.get('esc'+str(i),1).rpm==0 for i in range(3))
    assert not b.get('status',1).mow_enabled
    assert any(x['command']==183 and x['p1']==3 and x['p2']==1500 for x in commands), 'startup neutral not captured'
    before=len(commands)
    for i in range(11):
        assert c.wait_for_service(timeout_sec=1)
        f=c.call_async(MowerControl.Request(mow_enabled=False, direction=0))
        deadline=time.monotonic()+5
        while not f.done() and time.monotonic()<deadline:
            b.spin(.02,False)
        assert f.done() and f.result().success
        b.spin(.1,False)
    b.spin(3,False)
    assert len(commands)==before, 'OFF spam or unexpected command'
    assert not any(x['command']==400 or (x['command']==183 and (x['p1']!=3 or x['p2']!=1500)) for x in commands)
    assert not b.get('state',2).armed
    assert list(b.get('out',2).channels[:3])==[1500]*3
    assert all(b.get('esc'+str(i),1).valid and b.get('esc'+str(i),1).rpm==0 for i in range(3))
    old_log('startup_off_pass',commands=commands,status=str(b.get('status',1)),esc=[str(b.get('esc'+str(i),1)) for i in range(3)])
except Exception as e:
    old_log('startup_off_FAIL',error=str(e),commands=commands)
    raise
finally:
    b.n.destroy_node()
    rclpy.shutdown()
