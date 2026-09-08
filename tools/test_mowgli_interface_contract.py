import shutil,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from mowgli_interface_contract import fingerprint,load,validate
ROOT=Path(__file__).resolve().parents[1]; INTERFACES=ROOT/"ros2/src/mowgli_interfaces"
class ContractTest(unittest.TestCase):
 def setUp(self): self.manifest=load(INTERFACES/"interface-contract.lock.json")
 def test_pinned_set_and_fingerprint(self): self.assertEqual([],validate(INTERFACES,self.manifest)); self.assertEqual(self.manifest["fingerprint"],fingerprint(INTERFACES,self.manifest["interfaces"]))
 def test_changed_definition_fails_closed(self):
  with tempfile.TemporaryDirectory() as temporary:
   copied=Path(temporary)/"interfaces"; shutil.copytree(INTERFACES,copied); status=copied/"msg/Status.msg"; status.write_text(status.read_text()+"\n# modified\n"); self.assertTrue(validate(copied,self.manifest))
 def test_copy_is_deterministic(self):
  with tempfile.TemporaryDirectory() as temporary:
   first,second=Path(temporary)/"first",Path(temporary)/"second"
   for root in (first,second):
    for relative in self.manifest["interfaces"]: target=root/relative; target.parent.mkdir(parents=True,exist_ok=True); shutil.copyfile(INTERFACES/relative,target)
   self.assertEqual(fingerprint(first,self.manifest["interfaces"]),fingerprint(second,self.manifest["interfaces"])); self.assertEqual([],validate(first,self.manifest)); self.assertEqual([],validate(second,self.manifest))
if __name__ == "__main__": unittest.main()
