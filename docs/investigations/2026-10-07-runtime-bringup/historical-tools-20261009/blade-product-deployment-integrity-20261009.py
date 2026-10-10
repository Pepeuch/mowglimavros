import json, subprocess, pathlib

root=pathlib.Path('/tmp/mowgli-blade-product-rollback-20261009.Xng0jP')
before=json.loads((root/'mavros-before.json').read_text())[0]
after=json.loads(subprocess.check_output(['docker','inspect','mowgli-mavros']))[0]
checks={}
for key in ('Env','Entrypoint','Cmd'):
    left,right=before['Config'][key],after['Config'][key]
    checks['config_'+key]=(sorted(left)==sorted(right)) if key=='Env' else left==right
for key in ('NetworkMode','IpcMode','Privileged','Devices','Binds'):
    checks['host_'+key]=before['HostConfig'][key]==after['HostConfig'][key]
checks['mounts']=before['Mounts']==after['Mounts']
others=subprocess.check_output(['docker','inspect','--format','{{.Name}} {{.Id}} {{.State.StartedAt}}','mowgli-ros2','mowgli-gui','mowgli-gps']).decode()
checks['other_containers_unchanged']=others==(root/'others-before').read_text()
def without_image(text):
    return [line for line in text.splitlines() if not line.startswith('MAVROS_IMAGE=')]
checks['env_other_keys_unchanged']=without_image((root/'env-before').read_text())==without_image(pathlib.Path('/home/pepeuch/mowglinext/docker/.env').read_text())
checks['new_image']=after['Image']=='sha256:788d583129ffeec5cebd660cff1ddfe15357bae272b7741bf7be76bdba0759e1'
checks['sidecar_running']=after['State']['Running']
print(json.dumps({'checks':checks,'image':after['Image'],'started_at':after['State']['StartedAt']}))
assert all(checks.values())
