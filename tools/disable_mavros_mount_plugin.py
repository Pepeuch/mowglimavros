#!/usr/bin/env python3
"""Add mount_control to a MAVROS plugin denylist without replacing other entries."""
from pathlib import Path
import re
import sys


def disable_mount_control(text: str) -> str:
    if re.search(r"^\s*-\s*mount_control\s*$", text, re.MULTILINE):
        return text
    lines = text.splitlines(keepends=True)
    for index, line in enumerate(lines):
        match = re.match(r"^(\s*)plugin_denylist\s*:\s*(?:#.*)?$", line.rstrip("\r\n"))
        if match:
            newline = "\r\n" if line.endswith("\r\n") else "\n"
            lines.insert(index + 1, f"{match.group(1)}  - mount_control{newline}")
            return "".join(lines)
    raise ValueError("plugin_denylist not found in MAVROS profile")


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: disable_mavros_mount_plugin.py PROFILE.yaml")
    path = Path(sys.argv[1])
    path.write_text(disable_mount_control(path.read_text(encoding="utf-8")), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
