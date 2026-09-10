#!/usr/bin/env python3
"""Generate and validate the README progress dashboard from TODO.md."""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TODO_PATH = ROOT / "TODO.md"
README_PATH = ROOT / "README.md"
SVG_PATH = ROOT / "docs/status/mowglimavros_progress.svg"
BEGIN = "<!-- MOWGLIMAVROS_PROGRESS_BEGIN -->"
END = "<!-- MOWGLIMAVROS_PROGRESS_END -->"
STATUSES = ("TODO", "IN PROGRESS", "DONE", "BLOCKED", "DEFERRED")
STATUS_COLORS = {"DONE": "#1f8a70", "IN PROGRESS": "#d97706", "TODO": "#2563eb", "BLOCKED": "#c2410c", "DEFERRED": "#64748b"}
PHASE = re.compile(r"^# Phase (?P<name>.+)$")
ITEM = re.compile(r"^#{2,3} (?P<id>MM-\d{3})\b")
STATUS = re.compile(r"^Status: (?P<value>TODO|IN PROGRESS|DONE|BLOCKED|DEFERRED)(?:\b| \()")


class ProgressError(ValueError):
    """Raised when canonical TODO state cannot be rendered truthfully."""


@dataclass(frozen=True)
class Item:
    identifier: str
    phase: str
    status: str
    hardware_pending: bool


def parse_todo(text: str) -> list[Item]:
    lines = text.splitlines()
    phase = "Unphased"
    items: list[Item] = []
    for index, line in enumerate(lines):
        if match := PHASE.match(line):
            phase = match.group("name")
        match = ITEM.match(line)
        if not match:
            continue
        metadata = []
        for candidate in lines[index + 1:]:
            if not candidate.strip():
                break
            metadata.append(candidate)
        status_lines = [candidate for candidate in metadata if candidate.startswith("Status:")]
        if len(status_lines) != 1:
            raise ProgressError(f"{match.group('id')} must have exactly one immediate Status field")
        status_match = STATUS.match(status_lines[0])
        if not status_match:
            raise ProgressError(f"{match.group('id')} has unknown or malformed status: {status_lines[0]}")
        items.append(Item(match.group("id"), phase, status_match.group("value"), "HARDWARE_PENDING" in status_lines[0]))
    if not items:
        raise ProgressError("TODO contains no MM work items")
    if len({item.identifier for item in items}) != len(items):
        raise ProgressError("TODO contains duplicate MM work-item IDs")
    return items


def counts(items: list[Item]) -> Counter[str]:
    return Counter(item.status for item in items)


def complete(items: list[Item]) -> int:
    return sum(item.status == "DONE" and not item.hardware_pending for item in items)


def percent(done: int, total: int) -> str:
    return f"{done * 100 / total:.2f}%"


def render_svg(items: list[Item]) -> str:
    total, done = len(items), complete(items)
    width = 720
    fill = 664 * done / total
    return "\n".join((
        '<?xml version="1.0" encoding="UTF-8"?>',
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} 118" role="img" aria-labelledby="title desc">',
        '<title id="title">MowgliMAVROS development status</title>',
        f'<desc id="desc">{done} of {total} work items are fully complete; software DONE items with hardware pending remain open.</desc>',
        f'<rect width="{width}" height="118" rx="12" fill="#f8fafc"/>',
        '<text x="28" y="36" font-family="sans-serif" font-size="22" font-weight="700" fill="#0f172a">MowgliMAVROS Development Status</text>',
        f'<text x="28" y="61" font-family="sans-serif" font-size="14" fill="#334155">Verified completion</text>',
        f'<text x="692" y="61" text-anchor="end" font-family="sans-serif" font-size="14" font-weight="700" fill="#0f172a">{done} / {total} · {percent(done, total)}</text>',
        '<rect x="28" y="74" width="664" height="16" rx="8" fill="#dbe4ef"/>',
        f'<rect x="28" y="74" width="{fill:.2f}" height="16" rx="8" fill="#1f8a70"/>',
        '</svg>', ''))


def render_block(items: list[Item]) -> str:
    total, done, status_counts = len(items), complete(items), counts(items)
    hardware_pending = sum(item.hardware_pending for item in items)
    lines = [BEGIN, '## Project Progress', '', '![Generated MowgliMAVROS progress](docs/status/mowglimavros_progress.svg)', '', f'**Verified completion: {done} / {total} ({percent(done, total)})**. An item counts as fully complete only when its TODO status is `DONE` and it has no `HARDWARE_PENDING` qualifier.', '', f"Status: **DONE {status_counts['DONE']}** · **IN PROGRESS {status_counts['IN PROGRESS']}** · **TODO {status_counts['TODO']}** · **BLOCKED {status_counts['BLOCKED']}** · **DEFERRED {status_counts['DEFERRED']}**. Hardware-pending items: **{hardware_pending}** (software completion is not presented as physical validation).", '', '### Phase Progress', '']
    for phase in dict.fromkeys(item.phase for item in items):
        scoped = [item for item in items if item.phase == phase]
        lines.append(f"- **{phase}:** {complete(scoped)} / {len(scoped)} verified ({percent(complete(scoped), len(scoped))}); " + ", ".join(f"{status} {counts(scoped)[status]}" for status in STATUSES if counts(scoped)[status]))
    lines += ['', 'Generated solely from [`TODO.md`](TODO.md). Run `python3 tools/update_readme_progress.py` to update, or `python3 tools/update_readme_progress.py --check` to fail on stale output.', END, '']
    return "\n".join(lines)


def render_readme(readme: str, block: str) -> str:
    begin, end = readme.find(BEGIN), readme.find(END)
    if begin == -1 and end == -1:
        anchor = 'Sidecar ROS 2 repository for the optional MAVROS backend used by MowgliNext.\n'
        if anchor not in readme:
            raise ProgressError("README does not contain the progress insertion anchor")
        return readme.replace(anchor, anchor + "\n" + block + "\n", 1)
    if begin == -1 or end == -1 or end < begin:
        raise ProgressError("README has malformed progress markers")
    return readme[:begin] + block + "\n\n" + readme[end + len(END):].lstrip("\n")

def generate(check: bool) -> None:
    items = parse_todo(TODO_PATH.read_text(encoding="utf-8"))
    expected_readme = render_readme(README_PATH.read_text(encoding="utf-8"), render_block(items))
    expected_svg = render_svg(items)
    stale = [str(path.relative_to(ROOT)) for path, expected in ((README_PATH, expected_readme), (SVG_PATH, expected_svg)) if not path.exists() or path.read_text(encoding="utf-8") != expected]
    if stale and check:
        raise ProgressError("generated progress dashboard is stale: " + ", ".join(stale) + "; run python3 tools/update_readme_progress.py")
    if not check:
        README_PATH.write_text(expected_readme, encoding="utf-8")
        SVG_PATH.parent.mkdir(parents=True, exist_ok=True)
        SVG_PATH.write_text(expected_svg, encoding="utf-8")
    print(f"items={len(items)} verified_complete={complete(items)} statuses={dict(counts(items))}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if generated README/dashboard output is stale")
    args = parser.parse_args()
    try:
        generate(args.check)
    except ProgressError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
