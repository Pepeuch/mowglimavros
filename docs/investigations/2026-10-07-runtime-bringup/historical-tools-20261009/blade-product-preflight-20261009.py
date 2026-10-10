import sys, time, json
sys.path.insert(0, '/audit-old')
import mowgli_blade_charge_ab as old
import rclpy
from rcl_interfaces.srv import GetParameters

rclpy.init()
b = old.Bench('blade_product_passive_preflight')
b.subscribe()
b.spin(3, False)
initial = [b.get('esc'+str(i), 1).count for i in range(3)]
b.spin(5, False)
state = b.get('state', 2)
out = b.get('out', 2)
em = b.get('emergency', 1)
hls = b.get('hls', 2)
status = b.get('status', 1)
power = b.get('power', 2)
esc = [b.get('esc'+str(i), 1) for i in range(3)]
tw = b.get('cmd_vel', 1).twist
man = b.get('manual', 1)
checks = {
    'disarmed_manual': state.connected and not state.armed and state.mode=='MANUAL',
    'neutral': list(out.channels[:3])==[1500]*3,
    'esc_zero_fresh': all(e.valid and e.rpm_valid and e.rpm==0 and e.count!=initial[i] for i,e in enumerate(esc)),
    'safety_idle': not em.active_emergency and not em.latched_emergency and not hls.emergency and hls.state_name=='IDLE',
    'traction_zero': not any(getattr(v,k)!=0 for v in (tw.linear,tw.angular) for k in ('x','y','z')) and not any(getattr(man,k)!=0 for k in ('x','y','z','r')),
    'blade_off': not status.mow_enabled,
    'power_fresh': -.5 <= time.time()-power.stamp.sec-power.stamp.nanosec/1e9 <= 2,
}
print(json.dumps({'event':'product_preflight','wall':time.time(),'checks':checks,
    'state':str(state),'out':list(out.channels[:3]),'status':str(status),
    'esc':[str(e) for e in esc],'initial_counts':initial,'power':str(power)}, default=str), flush=True)
client = b.n.create_client(GetParameters, '/mavros/param/get_parameters')
assert client.wait_for_service(timeout_sec=3)
for name, expected in [('SERVO1_FUNCTION',74),('SERVO2_FUNCTION',73),('SERVO3_FUNCTION',0),
                       ('SERVO3_MIN',1000),('SERVO3_TRIM',1500),('SERVO3_MAX',2000)]:
    future = client.call_async(GetParameters.Request(names=[name]))
    deadline = time.monotonic()+3
    while not future.done() and time.monotonic()<deadline:
        b.spin(.02,False)
    assert future.done() and len(future.result().values)==1, name
    param = future.result().values[0]
    value = param.integer_value if param.type==2 else param.double_value
    print(json.dumps({'event':'fcu_param_read','name':name,'value':value,'expected':expected}),flush=True)
    assert value == expected, name
b.n.destroy_node()
rclpy.shutdown()
sys.exit(0 if all(checks.values()) else 1)
