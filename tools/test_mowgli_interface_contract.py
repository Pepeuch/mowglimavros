import shutil,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from mowgli_interface_contract import fingerprint,load,validate,validate_source,validate_installed
import json
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]; INTERFACES=ROOT/"ros2/src/mowgli_interfaces"
class ContractTest(unittest.TestCase):
 def setUp(self): self.manifest=load(INTERFACES/"interface-contract.lock.json")
 def test_pinned_set_and_fingerprint(self): self.assertEqual([],validate(INTERFACES,self.manifest)); self.assertEqual(self.manifest["fingerprint"],fingerprint(INTERFACES,self.manifest["interfaces"]))
 def test_changed_definition_fails_closed(self):
  with tempfile.TemporaryDirectory() as temporary:
   copied=Path(temporary)/"interfaces"; shutil.copytree(INTERFACES,copied); status=copied/"msg/GnssStatus.msg"; status.write_text(status.read_text()+"\n# modified\n"); self.assertTrue(validate(copied,self.manifest))
 def test_copy_is_deterministic(self):
  with tempfile.TemporaryDirectory() as temporary:
   first,second=Path(temporary)/"first",Path(temporary)/"second"
   for root in (first,second):
    for relative in self.manifest["interfaces"]: target=root/relative; target.parent.mkdir(parents=True,exist_ok=True); shutil.copyfile(INTERFACES/relative,target)
   self.assertEqual(fingerprint(first,self.manifest["interfaces"]),fingerprint(second,self.manifest["interfaces"])); self.assertEqual([],validate(first,self.manifest)); self.assertEqual([],validate(second,self.manifest))
class ConsumerAndInstalledContractTest(unittest.TestCase):
 def setUp(self): self.manifest=load(INTERFACES/"interface-contract.lock.json")
 def test_canonical_checkout_is_verified_even_when_local_copy_is_valid(self):
  with patch('mowgli_interface_contract.subprocess.check_output',return_value='wrong-revision\n'):
   self.assertIn('source revision mismatch',validate_source(INTERFACES,self.manifest)[0])
  with patch('mowgli_interface_contract.subprocess.check_output',return_value=self.manifest['source_revision']+'\n'):
   self.assertEqual([],validate_source(INTERFACES,self.manifest))
 def test_peer_without_provenance_fields_is_rejected(self):
  with tempfile.TemporaryDirectory() as directory:
   peer=Path(directory); shutil.copytree(INTERFACES,peer,dirs_exist_ok=True)
   path=peer/'msg/GnssStatus.msg'
   path.write_text('\n'.join(line for line in path.read_text().splitlines() if not line.startswith(('string source_id','string source_incarnation')))+'\n')
   self.assertTrue(validate(peer,self.manifest))
 def test_every_generated_type_must_match_the_lock(self):
  with tempfile.TemporaryDirectory() as directory:
   prefix=Path(directory)
   for relative,wanted in self.manifest['type_hashes'].items():
    stem=str(Path(relative).with_suffix('')); path=prefix/'share/mowgli_interfaces'/(stem+'.json'); path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps({'type_hashes':[{'type_name':'mowgli_interfaces/'+stem,'hash_string':wanted}]}))
   self.assertEqual([],validate_installed(prefix,self.manifest))
   path=prefix/'share/mowgli_interfaces/msg/GnssStatus.json'
   path.write_text(json.dumps({'type_hashes':[{'type_name':'mowgli_interfaces/msg/GnssStatus','hash_string':'RIHS01_6765a75d0f9bc5858eea44f685f485335e8c877b770f35d1474ca08e1ad5285a'}]}))
   self.assertIn('installed type hash mismatch',validate_installed(prefix,self.manifest)[0])
   path.unlink(); self.assertIn('missing installed type description',validate_installed(prefix,self.manifest)[0])
 def test_an_incomplete_type_hash_lock_fails_closed(self):
  self.manifest['type_hashes'].pop('msg/GnssStatus.msg')
  self.assertEqual(['lock must identify every installed public ROS type'],validate_installed(Path('/unused'),self.manifest))
 def test_publication_build_checks_source_generated_types_and_mapping(self):
  workflow=(ROOT/'.github/workflows/docker.yml').read_text()
  self.assertIn('ref: ${{ steps.interface.outputs.source_revision }}',workflow)
  self.assertIn('--source-root .interface-source/ros2/src/mowgli_interfaces',workflow)
  self.assertIn('python3 tools/test_mowgli_interface_contract.py',workflow)
  self.assertIn('io.mowgli.interfaces.gnss-type-hash=',workflow)
  docker=(ROOT/'ros2/Dockerfile').read_text().split('FROM universal-gnss-build AS workspace-build',1)[1].split('FROM ros:',1)[0]
  self.assertIn('check --interface-root src/mowgli_interfaces --installed-root install/mowgli_interfaces',docker)
  self.assertIn('--packages-select mavros_gnss_adapter',docker)
  self.assertIn("--no-tests=error -R '^test_gnss_adapter$'",docker)
  self.assertIn('install(FILES interface-contract.lock.json', (INTERFACES/'CMakeLists.txt').read_text())

if __name__ == "__main__": unittest.main()
