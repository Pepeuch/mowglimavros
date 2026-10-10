import sys,time,struct
sys.path.insert(0,'/audit-old')
import mowgli_blade_charge_ab as old
import rclpy
from mavros_msgs.msg import Mavlink
from mowgli_interfaces.srv import MowerControl
old_log=old.log
old.log=lambda event,**kw: old_log(event,**kw) if event!='esc' else None
rclpy.init()
b=old.Bench('blade_product_off_repeat_audit');b.subscribe()
wire=[]
def outgoing(m):
    if m.msgid!=76 or m.framing_status!=Mavlink.FRAMING_OK:return
    p=b''.join(struct.pack('<Q',x) for x in m.payload64)[:m.len]
    values=struct.unpack('<7fHBBB',p.ljust(33,b'\0')[:33])
    if values[7] in (183,400):
        wire.append(values)
        old_log('wire',command=values[7],p1=values[0],p2=values[1])
sub=b.n.create_subscription(Mavlink,'/uas1/mavlink_sink',outgoing,old.qos_profile_sensor_data)
c=b.n.create_client(MowerControl,'/hardware_bridge/mower_control')
try:
    b.spin(3,False)
    s=b.get('state',2)
    assert s.connected and not s.armed and s.mode=='MANUAL'
    assert list(b.get('out',2).channels[:3])==[1500]*3
    assert b.get('status',1).blade_requested_direction=='off'
    assert not b.get('status',1).mow_enabled
    assert c.wait_for_service(timeout_sec=2)
    initial=len(wire)
    for i in range(11):
        f=c.call_async(MowerControl.Request(mow_enabled=0,mow_direction=0))
        end=time.monotonic()+5
        while not f.done() and time.monotonic()<end:b.spin(.02,False)
        assert f.done() and f.result().success
        old_log('off_response',attempt=i,success=f.result().success)
        b.spin(.1,False)
    b.spin(3,False)
    assert len(wire)==initial==0,'unexpected command from repeated OFF'
    assert not b.get('state',2).armed
    assert list(b.get('out',2).channels[:3])==[1500]*3
    assert all(b.get('esc'+str(i),1).valid and b.get('esc'+str(i),1).rpm==0 for i in range(3))
    old_log('off_repeat_pass',requests=11,commands=0,armed=False,status=str(b.get('status',1)))
finally:
    b.n.destroy_node();rclpy.shutdown()
