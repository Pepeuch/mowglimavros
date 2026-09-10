#!/usr/bin/env python3
"""Focused checks for the TODO-derived README dashboard."""
import importlib.util
import sys
import unittest
from pathlib import Path

SCRIPT = Path(__file__).with_name("update_readme_progress.py")
SPEC = importlib.util.spec_from_file_location("update_readme_progress", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class ProgressTests(unittest.TestCase):
    def test_parses_known_statuses_and_hardware_pending(self):
        items = MODULE.parse_todo("# Phase X\n## MM-001 — One\nStatus: DONE (software); HARDWARE_PENDING\n\n## MM-002 — Two\nStatus: IN PROGRESS\n\n## MM-003 — Three\nStatus: TODO\n\n## MM-004 — Four\nStatus: BLOCKED\n\n## MM-005 — Five\nStatus: DEFERRED\n")
        self.assertEqual(["DONE", "IN PROGRESS", "TODO", "BLOCKED", "DEFERRED"], [item.status for item in items])
        self.assertTrue(items[0].hardware_pending)
        self.assertEqual(0, MODULE.complete(items))
        self.assertEqual("0.00%", MODULE.percent(0, 5))

    def test_output_is_stable(self):
        items = MODULE.parse_todo("# Phase X\n## MM-001 — One\nStatus: DONE\n")
        self.assertEqual(MODULE.render_block(items), MODULE.render_block(items))
        self.assertEqual(MODULE.render_svg(items), MODULE.render_svg(items))

    def test_unknown_or_malformed_status_fails_closed(self):
        for status in ("Status: MAYBE", "Status: DONE\nStatus: TODO"):
            with self.assertRaises(MODULE.ProgressError):
                MODULE.parse_todo(f"# Phase X\n## MM-001 — One\n{status}\n")


if __name__ == "__main__":
    unittest.main()
