"""Read-only MAVROS receive audit. No publisher or FCU command client."""
import time,json,struct,collections,statistics,math,re,glob,sys
import rclpy
from rclpy.qos import qos_profile_sensor_data, QoSProfile, ReliabilityPolicy
from mavros_msgs.msg import Mavlink,State,RCOut
from mavros_esc_wheel_odometry.msg import EscObservation
from rcl_interfaces.srv import ListParameters,GetParameters
from rosidl_runtime_py.utilities import get_message

names={}
for path in glob.glob('/sdk/include/**/mavlink_msg_*.h',recursive=True):
    with open(path) as f: match=re.search(r'#define MAVLINK_MSG_ID_(\w+) (\d+)\s',f.read(1000))
    if match:names[int(match[2])]=match[1]
rclpy.init();node=rclpy.create_node('mavlink_passive_rate_audit')
received=collections.defaultdict(list);stamps=collections.defaultdict(list)
esc_delivery=collections.defaultdict(list);esc_unique=collections.defaultdict(list)
esc_identity={};outgoing=[];states=[];outputs=[];invalid=[0];window=[None,None]
sequence={};sequence_gaps=collections.Counter()
instances=collections.defaultdict(list);topic_times=collections.defaultdict(list)
seconds=float(sys.argv[1]) if len(sys.argv)>1 else 60
def active():return window[0] is not None and time.monotonic()<window[1]
def incoming(m):
    if not active():return
    if m.framing_status!=Mavlink.FRAMING_OK:invalid[0]+=1;return
    source=(m.sysid,m.compid)
    if source in sequence:
        advance=(m.seq-sequence[source])%256
        if advance>1:sequence_gaps[str(source)]+=advance-1
    sequence[source]=m.seq
    key=(m.sysid,m.compid,m.msgid);received[key].append(time.monotonic())
    stamps[key].append(m.header.stamp.sec+m.header.stamp.nanosec/1e9)
    if m.msgid==147:
        p=b''.join(struct.pack('<Q',v) for v in m.payload64)[:m.len].ljust(36,b'\0')
        instances[(m.sysid,m.compid,m.msgid,p[32])].append(time.monotonic())
    elif m.msgid==36:
        p=b''.join(struct.pack('<Q',v) for v in m.payload64)[:m.len].ljust(21,b'\0')
        instances[(m.sysid,m.compid,m.msgid,p[20])].append(time.monotonic())
def sink(m):
    if not active():return
    if m.msgid==76:
        p=b''.join(struct.pack('<Q',v) for v in m.payload64)[:m.len]
        v=struct.unpack('<7fHBBB',p.ljust(33,b'\0')[:33])
        if v[7] in (183,400,511,512,32000):outgoing.append(dict(wall=time.time(),command=v[7],param1=v[0],param2=v[1]))
    elif m.msgid==66:
        outgoing.append(dict(wall=time.time(),message='REQUEST_DATA_STREAM',payload64=list(m.payload64),length=m.len))
def esc(m):
    if not active() or m.esc_index>2:return
    now=time.monotonic();esc_delivery[m.esc_index].append(now)
    identity=(m.source,m.count if m.count_valid else None,m.header.stamp.sec,m.header.stamp.nanosec)
    # A valid legacy counter must progress; a changing republish stamp alone
    # is not a new ESC acquisition.
    previous=esc_identity.get(m.esc_index)
    distinct=previous is None or (identity[0]==2 and identity[1]!=previous[1]) or (identity[0]==1 and identity[2:]!=previous[2:])
    if distinct and m.valid and m.rpm_valid:
        esc_unique[m.esc_index].append(now);esc_identity[m.esc_index]=identity
def state(m):
    if active():states.append(dict(connected=m.connected,armed=m.armed,mode=m.mode))
def rc(m):
    if active():outputs.append(list(m.channels[:3]))
bulk_qos=QoSProfile(depth=4096,reliability=ReliabilityPolicy.BEST_EFFORT)
subs=[node.create_subscription(Mavlink,'/uas1/mavlink_source',incoming,bulk_qos),
      node.create_subscription(Mavlink,'/uas1/mavlink_sink',sink,bulk_qos),
      node.create_subscription(EscObservation,'/mavros/esc_wheel_odometry/esc_observation',esc,qos_profile_sensor_data),
      node.create_subscription(State,'/mavros/state',state,qos_profile_sensor_data),
      node.create_subscription(RCOut,'/mavros/rc/out',rc,qos_profile_sensor_data)]
def spin(seconds):
    end=time.monotonic()+seconds
    while time.monotonic()<end:rclpy.spin_once(node,timeout_sec=.02)
def percentile(values,q):
    values=sorted(values);position=(len(values)-1)*q;left=math.floor(position);right=math.ceil(position)
    return values[left]+(values[right]-values[left])*(position-left)
def metrics(times,duration):
    delta=[b-a for a,b in zip(times,times[1:])]
    result=dict(count=len(times),hz_count_over_window=len(times)/duration)
    if delta:
        result.update(period_mean_s=statistics.mean(delta),period_min_s=min(delta),period_max_s=max(delta),period_p95_s=percentile(delta,.95),period_p99_s=percentile(delta,.99),largest_gap_s=max(delta),hz_span=(len(times)-1)/(times[-1]-times[0]))
    if times:result['largest_boundary_gap_s']=max(times[0]-window[0],window[1]-times[-1],max(delta,default=0))
    return result
spin(3)
graph={}
critical=('/mavros/state','/mavros/sys_status','/mavros/imu/data','/mavros/rc/out','/hardware_bridge/power','/gps/status')
for topic,types in node.get_topic_names_and_types():
    if topic not in critical or len(types)!=1:continue
    typ=get_message(types[0])
    subs.append(node.create_subscription(typ,topic,lambda m,t=topic:topic_times[t].append(time.monotonic()) if active() else None,bulk_qos))
    graph[topic]=dict(type=types[0],publishers=[info.node_namespace+'/'+info.node_name for info in node.get_publishers_info_by_topic(topic)],subscribers=[info.node_namespace+'/'+info.node_name for info in node.get_subscriptions_info_by_topic(topic)])
spin(1);wall_start=time.time();window[0]=time.monotonic();window[1]=window[0]+seconds
print(json.dumps(dict(event='audit_started',wall=wall_start,duration_requested_s=seconds)),flush=True)
spin(seconds);duration=time.monotonic()-window[0]
rows=[]
for key,times in sorted(received.items()):
    row=dict(sysid=key[0],compid=key[1],id=key[2],name=names.get(key[2],'UNKNOWN'),**metrics(times,duration))
    ts=stamps[key];delta=[b-a for a,b in zip(ts,ts[1:]) if b>a]
    if delta:row['header_stamp_max_gap_s']=max(delta)
    rows.append(row)
report=dict(event='audit_result',wall_start=wall_start,wall_end=time.time(),duration_s=duration,rows=rows,invalid_framing=invalid[0],sequence_gap_lower_bound=dict(sequence_gaps),subscriber_depth=4096,outgoing=outgoing,
            esc_observation={str(i):dict(delivery=metrics(esc_delivery[i],duration),distinct=metrics(esc_unique[i],duration)) for i in range(3)},
            state_changes=[x for i,x in enumerate(states) if i==0 or x!=states[i-1]],out_unique=sorted(set(tuple(x) for x in outputs)))
report['message_instances']=[dict(sysid=k[0],compid=k[1],id=k[2],instance=k[3],**metrics(v,duration)) for k,v in instances.items()]
report['topic_metrics']={t:metrics(v,duration) for t,v in topic_times.items()}
report['topic_graph']=graph
print(json.dumps(report),flush=True)
# Cached FCU/ROS parameter reads occur AFTER the measurement. No param pull,
# stream service, command-long, arming or publisher is created by this process.
for target in ('/mavros/param','/mavros_node','/mavros','/mavros/sys','/mavros/time','/mavros/esc_wheel_odometry','/mavros/battery_observer','/hardware_bridge'):
    listing=node.create_client(ListParameters,target+'/list_parameters')
    getter=node.create_client(GetParameters,target+'/get_parameters')
    if not listing.wait_for_service(timeout_sec=1) or not getter.wait_for_service(timeout_sec=1):continue
    future=listing.call_async(ListParameters.Request(depth=100))
    rclpy.spin_until_future_complete(node,future,timeout_sec=4)
    if not future.done():continue
    keys=future.result().result.names
    if target=='/mavros/param':keys=[k for k in keys if re.match(r'^(SERIAL\d+_|SR\d+_|MAV\d*_).*',k)]
    else:keys=[k for k in keys if re.search(r'timeout|fresh|stale|rate|url|plugin|source|slot|safety|wheel|firmware|blade|gnss_required|heartbeat|conn\.|timesync',k)]
    values={}
    for offset in range(0,len(keys),80):
        batch=keys[offset:offset+80];future=getter.call_async(GetParameters.Request(names=batch))
        rclpy.spin_until_future_complete(node,future,timeout_sec=4)
        if not future.done():continue
        for key,value in zip(batch,future.result().values):
            fields={1:'bool_value',2:'integer_value',3:'double_value',4:'string_value',5:'byte_array_value',6:'bool_array_value',7:'integer_array_value',8:'double_array_value',9:'string_array_value'}
            values[key]=getattr(value,fields[value.type]) if value.type in fields else None
    print(json.dumps(dict(event='parameters_read',node=target,values=values),default=list),flush=True)
node.destroy_node();rclpy.shutdown()
