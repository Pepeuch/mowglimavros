#!/usr/bin/env python3
"""Verify pinned source, peer definitions and the installed ROS type contracts."""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path


def load(path):
    manifest = json.loads(path.read_text())
    if (manifest.get('format') != 1
            or list(manifest.get('interfaces', {})) != sorted(manifest.get('interfaces', {}))
            or 'external_interface_sources' in manifest
            or 'msg/GnssStatus.msg' not in manifest.get('interfaces', {})):
        raise ValueError('invalid interface contract lock')
    return manifest


def fingerprint(root, paths):
    digest = hashlib.sha256()
    for relative in sorted(paths):
        digest.update(relative.encode())
        digest.update(b'\0')
        digest.update((root / relative).read_bytes())
        digest.update(b'\0')
    return digest.hexdigest()


def validate(root, manifest):
    errors = []
    for relative, expected in manifest['interfaces'].items():
        path = root / relative
        if not path.is_file():
            errors.append(f'missing required interface: {relative}')
            continue
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            errors.append(f'interface digest mismatch for {relative}')
    if not errors and fingerprint(root, manifest['interfaces']) != manifest['fingerprint']:
        errors.append('aggregate interface fingerprint mismatch')
    return errors


def validate_source(source, manifest):
    revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    if revision != manifest['source_revision']:
        return [f"source revision mismatch: expected {manifest['source_revision']}, got {revision}"]
    return validate(source, manifest)


def validate_installed(prefix, manifest):
    expected = manifest.get('type_hashes', {})
    if set(expected) != set(manifest['interfaces']):
        return ['lock must identify every installed public ROS type']
    errors = []
    for relative, wanted in expected.items():
        stem = str(Path(relative).with_suffix(''))
        path = prefix / 'share/mowgli_interfaces' / (stem + '.json')
        if not path.is_file():
            errors.append(f'missing installed type description: {relative}')
            continue
        doc = json.loads(path.read_text())
        name = 'mowgli_interfaces/' + stem
        matches = [h['hash_string'] for h in doc.get('type_hashes', []) if h.get('type_name') == name]
        if matches != [wanted] or not wanted.startswith('RIHS01_'):
            errors.append(f'installed type hash mismatch for {relative}: expected {wanted}, got {matches}')
    return errors


def sync(source, destination, manifest):
    if errors := validate_source(source, manifest):
        raise ValueError('pinned source does not match lock: ' + '; '.join(errors))
    for relative in manifest['interfaces']:
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / relative, target)
    if errors := validate(destination, manifest):
        raise ValueError('generated copy does not match lock: ' + '; '.join(errors))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=('check', 'sync'))
    parser.add_argument('--interface-root', type=Path, default=Path('ros2/src/mowgli_interfaces'))
    parser.add_argument('--source-root', type=Path, help='exact canonical checkout at the locked commit')
    parser.add_argument('--peer-root', type=Path, help='actual consumer interfaces; compare content, not its branch name')
    parser.add_argument('--installed-root', type=Path, help='installed mowgli_interfaces prefix, including share/')
    args = parser.parse_args()
    try:
        manifest = load(args.interface_root / 'interface-contract.lock.json')
        if args.command == 'sync':
            if args.source_root is None:
                parser.error('sync requires --source-root')
            sync(args.source_root, args.interface_root, manifest)
        errors = validate(args.interface_root, manifest)
        if args.source_root:
            errors += validate_source(args.source_root, manifest)
        if args.peer_root:
            errors += ['peer ' + e for e in validate(args.peer_root, manifest)]
        if args.installed_root:
            errors += validate_installed(args.installed_root, manifest)
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f'MM-602 interface contract check failed: {error}')
        return 1
    if errors:
        print('MM-602 interface contract check failed:\n' + '\n'.join('- ' + e for e in errors))
        return 1
    print('MM-602 interface contract check passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
