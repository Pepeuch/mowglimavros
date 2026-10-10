"""Temporary secured bench: product FWD only, independent rescue mandatory."""
import sys,time,struct
sys.path.insert(0,'/audit-old')
import mowgli_blade_charge_ab as old
import rclpy
from std_msgs.msg import String
from mavros_msgs.msg import Mavlink, SysStatus
from mavros_msgs.srv import CommandBool
from mowgli_interfaces.srv import MowerControl
TOKEN=sys.argv[1]
rclpy.init()
b=old.Bench('blade_product_first_fwd');b.charge=True;b.subscribe()
c=b.n.create_client(MowerControl,'/hardware_bridge/mower_control')
lease=b.n.create_publisher(String,'/bench/blade_ab_lease',10)
ready=[0.];counter=[0];heartbeat_at=[0.];forward=[False];renew_at=[0.];busy=[False]
subs=[b.n.create_subscription(String,'/bench/blade_ab_rescue_ready',lambda m:ready.__setitem__(0,time.monotonic()) if m.data==TOKEN else None,10),
      b.n.create_subscription(SysStatus,'/mavros/sys_status',lambda m:b.save('sys_status',m),old.qos_profile_sensor_data)]
def wire(m):
    if m.msgid!=76 or m.framing_status!=Mavlink.FRAMING_OK:return
    p=b''.join(struct.pack('<Q',x) for x in m.payload64)[:m.len]
    v=struct.unpack('<7fHBBB',p.ljust(33,b'\0')[:33])
    if v[7] in (183,400,32000):old.log('wire',phase=b.phase,command=v[7],param1=v[0],param2=v[1])
subs.append(b.n.create_subscription(Mavlink,'/uas1/mavlink_sink',wire,old.qos_profile_sensor_data))
frames=[0]
subs.append(b.n.create_subscription(Mavlink,'/uas1/mavlink_source',lambda m:frames.__setitem__(0,frames[0]+1) if m.msgid in (386,387) else None,old.qos_profile_sensor_data))
native=b.spin
def spin(sec,guard=True):
    end=time.monotonic()+sec
    while time.monotonic()<end:
        native(.01,guard)
        now=time.monotonic()
        if now>=heartbeat_at[0]:
            counter[0]+=1;lease.publish(String(data=TOKEN+':'+str(counter[0])));heartbeat_at[0]=now+.1
        if guard and b.guard_on and now-ready[0]>1:raise RuntimeError('rescue readiness stale')
        if forward[0] and not busy[0] and now>=renew_at[0]:
            busy[0]=True;renew_at[0]=now+2
            try:b.command(32000,1)
            finally:busy[0]=False
b.spin=spin
def mower(enabled):
    assert c.service_is_ready(),'mower service unavailable'
    f=c.call_async(MowerControl.Request(mow_enabled=int(enabled),mow_direction=0))
    end=time.monotonic()+4
    while not f.done() and time.monotonic()<end:b.spin(.02)
    assert f.done() and f.result().success,'mower_control refused or timed out'
    old.log('mower_response',phase=b.phase,enabled=enabled,success=f.result().success)
def safe_final():
    s=b.get('state',2.5)
    assert s.connected and not s.armed and s.mode=='MANUAL'
    assert list(b.get('out',2).channels[:3])==[1500]*3
    assert all(b.get('esc'+str(i),1).valid and b.get('esc'+str(i),1).rpm_valid and b.get('esc'+str(i),1).rpm==0 for i in range(3))
passed=False;restored=False
try:
    b.spin(4,False);b.guard();safe_final()
    end=time.monotonic()+12
    while time.monotonic()<end and time.monotonic()-ready[0]>1:b.spin(.05,False)
    assert time.monotonic()-ready[0]<=1,'independent rescue not ready'
    hardware=b.get('sys_status',2)
    assert hardware.sensors_present & (1<<15) and hardware.sensors_enabled & (1<<15),'hardware safety not released'
    b.snapshot('secured_preflight')
    b.guard_on=True;b.phase='OFF';mower(False)
    b.filter();b.spin(.3);busy[0]=True
    try:b.command(32000,1)
    finally:busy[0]=False
    forward[0]=True;renew_at[0]=time.monotonic()+2;b.spin(1)
    b.raw_required=True;b.wait_zero(4)
    b.phase='explicit_ARM';b.call(b.arm,CommandBool.Request(value=True))
    end=time.monotonic()+4
    while time.monotonic()<end and not b.get('state',2).armed:b.spin(.05)
    assert b.get('state',2).armed,'ARM not observed'
    b.armed_required=True;b.spin(2)
    assert list(b.get('out',2).channels[:3])==[1500]*3
    b.snapshot('ARM_stable')
    b.phase='FWD_1450';mower(True);start=time.monotonic()
    while time.monotonic()-start<1:
        b.spin(.02)
        if b.get('out',2).channels[2]==1450 and b.get('status',1).mow_enabled:break
    else:raise RuntimeError('FWD output/status did not become confirmed')
    while time.monotonic()-start<3:
        b.spin(.02)
        assert b.get('out',2).channels[2]==1450,'FWD output not 1450'
        assert b.get('status',1).mow_enabled,'product ON unexpectedly cleared'
    b.snapshot('pulse_end');b.phase='OFF_after_FWD';mower(False)
    b.wait_zero();b.spin(1);b.wait_zero(3)
    ds=[d for d in b.samples if d['index']==2 and d['phase']=='FWD_1450' and d['rpm']]
    assert ds,'no raw ESC2 actuation'
    old.log('fwd_measurements',rpm_min=min(d['rpm'] for d in ds),rpm_max=max(d['rpm'] for d in ds),current_max=max(d['current'] for d in ds))
    passed=True
except BaseException as e:old.log('product_trial_STOP',error=str(e),phase=b.phase)
finally:
    b.guard_on=False;b.armed_required=False;b.phase='cleanup';errors=[]
    try:
        mower(False)
        if forward[0]:b.wait_zero()
    except Exception as e:
        errors.append(str(e))
        try:b.command(183,3,1500)
        except Exception as fallback:errors.append(str(fallback))
    try:b.call(b.arm,CommandBool.Request(value=False))
    except Exception as e:errors.append(str(e))
    forward[0]=False
    try:b.command(32000,0);b.filter(True)
    except Exception as e:errors.append(str(e))
    try:
        b.spin(.5,False);before=frames[0];b.spin(2.2,False);safe_final()
        assert frames[0]==before,'CAN forwarding did not stop'
        restored=not errors
    except Exception as e:errors.append(str(e))
    b.snapshot('final');old.log('product_trial_final',pass_=passed,restored=restored,errors=errors)
    if restored:
        for _ in range(10):lease.publish(String(data=TOKEN+':DONE'));b.spin(.05,False)
    b.n.destroy_node();rclpy.shutdown()
sys.exit(0 if passed and restored else 1)
