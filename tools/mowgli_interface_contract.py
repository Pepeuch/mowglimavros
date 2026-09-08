#!/usr/bin/env python3
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path

def load(path):
    manifest=json.loads(path.read_text())
    if manifest.get("format") != 1 or list(manifest.get("interfaces",{})) != sorted(manifest.get("interfaces",{})): raise ValueError("invalid interface contract lock")
    return manifest

def fingerprint(root, paths):
    digest=hashlib.sha256()
    for relative in sorted(paths): digest.update(relative.encode()); digest.update(b"\0"); digest.update((root/relative).read_bytes()); digest.update(b"\0")
    return digest.hexdigest()

def validate(root, manifest):
    errors=[]
    for relative, expected in manifest["interfaces"].items():
        path=root/relative
        if not path.is_file(): errors.append(f"missing required interface: {relative}"); continue
        actual=hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != expected: errors.append(f"interface digest mismatch for {relative}")
    if not errors and fingerprint(root, manifest["interfaces"]) != manifest["fingerprint"]: errors.append("aggregate interface fingerprint mismatch")
    return errors

def sync(source, destination, manifest):
    revision=subprocess.check_output(["git","-C",str(source),"rev-parse","HEAD"],text=True).strip()
    if revision != manifest["source_revision"]: raise ValueError(f"source revision mismatch: expected {manifest['source_revision']}, got {revision}")
    if errors:=validate(source,manifest): raise ValueError("pinned source does not match lock: "+"; ".join(errors))
    for relative in manifest["interfaces"]:
        target=destination/relative; target.parent.mkdir(parents=True,exist_ok=True); shutil.copyfile(source/relative,target)
    if errors:=validate(destination,manifest): raise ValueError("generated copy does not match lock: "+"; ".join(errors))

def main():
    parser=argparse.ArgumentParser(); parser.add_argument("command",choices=("check","sync")); parser.add_argument("--interface-root",type=Path,default=Path("ros2/src/mowgli_interfaces")); parser.add_argument("--source-root",type=Path); args=parser.parse_args(); manifest=load(args.interface_root/"interface-contract.lock.json")
    try:
        if args.command == "sync":
            if args.source_root is None: parser.error("sync requires --source-root")
            sync(args.source_root,args.interface_root,manifest); errors=[]
        else: errors=validate(args.interface_root,manifest)
    except (OSError,ValueError,subprocess.CalledProcessError) as error: print(f"MM-602 interface contract check failed: {error}"); return 1
    if errors: print("MM-602 interface contract check failed:\n"+"\n".join("- "+error for error in errors)); return 1
    print("MM-602 interface contract check passed"); return 0
if __name__ == "__main__": raise SystemExit(main())
