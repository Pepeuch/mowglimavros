import json,subprocess
data=subprocess.check_output(['docker','logs','mowgli-blade-product-fwd-main-20261009'],stderr=subprocess.STDOUT).decode()
events=[]
for line in data.splitlines():
    try:events.append(json.loads(line))
    except ValueError:pass
raw=[e for e in events if e.get('event')=='raw1034']
summary={}
for index in range(3):
    samples=[e for e in raw if e['index']==index]
    summary[str(index)]={'samples':len(samples),'rpm_min':min((e['rpm'] for e in samples),default=None),'rpm_max':max((e['rpm'] for e in samples),default=None),'current_max':max((e['current'] for e in samples),default=None)}
distinct=[]
for e in events:
    if e.get('event')=='esc' and e['index']==2 and (not distinct or distinct[-1]['count']!=e['count']):distinct.append(e)
print(json.dumps({'raw1034':summary,'esc2_distinct_acquisitions':[{'wall':e['wall'],'count':e['count'],'phase':e['phase'],'rpm':e['rpm']} for e in distinct],'max_distinct_esc2_gap':max((b['wall']-a['wall'] for a,b in zip(distinct,distinct[1:])),default=None)}))
for e in events:
    if e.get('event') in ('ARM_stable','product_trial_STOP','product_trial_final'):
        print(json.dumps(e))
