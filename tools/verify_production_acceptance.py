#!/usr/bin/env python3
"""Release attestations are required; this checker does not prove physical safety."""

import os
from pathlib import Path, PurePosixPath
import subprocess
import sys


def verify_record(root, record, kind, ros2_tree, tracked):
    path = PurePosixPath(record)
    if (not record or path.is_absolute() or ".." in path.parts
            or path.parts[:2] != ("docs", "acceptance") or path.suffix != ".md"):
        raise ValueError("Acceptance record must be a tracked docs/acceptance/*.md path")
    file = root / path
    if (not tracked(str(path)) or not file.is_file() or file.is_symlink()
            or not file.resolve().is_relative_to(root.resolve() / "docs/acceptance")):
        raise ValueError("Acceptance record is missing, untracked or outside acceptance directory")
    fields = {}
    for line in file.read_text().splitlines():
        if ": " in line:
            key, value = line.split(": ", 1)
            if key in fields:
                raise ValueError("Duplicate acceptance field")
            fields[key] = value
    required = {"Acceptance-Result": "PASS", "Acceptance-Kind": kind,
                "Validated-ROS2-Tree": ros2_tree, "Operator-Acceptance": "CONFIRMED"}
    if any(fields.get(key) != value for key, value in required.items()):
        raise ValueError("Acceptance is not confirmed PASS for the current ROS2 source tree")
    evidence = fields.get("Evidence-Record", "")
    evidence_path = PurePosixPath(evidence)
    if (not evidence or evidence_path.is_absolute() or ".." in evidence_path.parts
            or evidence_path.parts[:1] != ("docs",) or evidence == str(path)
            or not tracked(evidence) or not (root / evidence).is_file()
            or (root / evidence).is_symlink()
            or not (root / evidence).resolve().is_relative_to(root.resolve() / "docs")):
        raise ValueError("Acceptance needs a separate tracked evidence record")


def main():
    root = Path(__file__).resolve().parents[1]

    def git(*args):
        return subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True)

    tree = git("rev-parse", "HEAD:ros2")
    if tree.returncode:
        raise ValueError("Cannot establish the release ROS2 tree")

    def tracked(path):
        return git("ls-files", "--error-unmatch", "--", path).returncode == 0

    for variable, kind in (("FAILSAFE_RECORD", "companion-loss-failsafe"),
                           ("AUTHORITY_RECORD", "blade-exclusive-authority")):
        verify_record(root, os.environ.get(variable, ""), kind, tree.stdout.strip(), tracked)
    print("Release attestations and source baseline verified; physical evidence remains operator-owned")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError) as error:
        print("Publication blocked: " + str(error), file=sys.stderr)
        sys.exit(1)
