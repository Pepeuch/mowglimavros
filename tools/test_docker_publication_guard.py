#!/usr/bin/env python3
"""Offline workflow contract checks; no GitHub token, Docker or hardware access."""

from itertools import product
from pathlib import Path
import re
import os
import subprocess
import tempfile
import textwrap
import unittest

from verify_production_acceptance import verify_record


WORKFLOW = Path(__file__).parents[1] / ".github/workflows/docker.yml"
GUARD = "needs.pins.outputs.publish_images == 'true'"
TERMS = (
    "github.event_name == 'workflow_dispatch'",
    "github.ref == 'refs/heads/main'",
    "inputs.publication_mode == 'production'",
    "vars.ENABLE_PRODUCTION_IMAGE_PUBLICATION == 'true'",
    "vars.BLADE_FAILSAFE_VALIDATED == 'true'",
    "vars.BLADE_EXCLUSIVE_AUTHORITY_VALIDATED == 'true'",
)


class PublicationGuard(unittest.TestCase):
    def setUp(self):
        self.text = WORKFLOW.read_text()
        self.pins, self.build = self.text.split("\n  build:\n", 1)
        self.build, self.publish = self.build.split("\n  publish-manifest:\n", 1)

    def test_release_requires_every_gate_and_defaults_closed(self):
        allowed = re.search(r"^\s+PRODUCTION_ALLOWED: (.+)$", self.pins, re.MULTILINE).group(1)
        self.assertEqual(allowed, "${{ " + " && ".join(TERMS) + " }}")
        dispatch = self.pins.split("  workflow_dispatch:", 1)[1].split("\npermissions:", 1)[0]
        self.assertIn("type: choice", dispatch)
        self.assertIn("default: none", dispatch)
        self.assertIn("options: [none, development, production]", dispatch)
        self.assertIn("enabled=false", self.pins)
        self.assertIn("set -euo pipefail", self.pins)
        self.assertIn("publish_images: ${{ steps.publication.outputs.enabled }}", self.pins)
        self.assertIn("publish_production: ${{ steps.publication.outputs.production }}", self.pins)

    def test_all_registry_writes_share_the_gate(self):
        self.assertIn("push: ${{ " + GUARD + " }}", self.build)
        self.assertRegex(self.build, r"name: Login to GHCR\n\s+if: " + re.escape(GUARD))
        self.assertRegex(self.publish, r"^\s+needs: \[pins, build\]\n\s+if: " + re.escape(GUARD))
        self.assertNotRegex(self.text, r"(?m)^\s+push: true\s*$")
        self.assertEqual(self.text.count("docker/build-push-action@"), 1)
        self.assertEqual(self.text.count("docker buildx imagetools create"), 1)
        self.assertNotIn("docker push", self.text)
        self.assertIn("python3 tools/verify_production_acceptance.py", self.pins)

    def test_normal_push_and_missing_variables_never_publish(self):
        # Truth table for the exact conjunction checked above, not a claim that
        # GitHub Actions itself was executed locally.
        for event, ref, mode, enabled, failsafe, authority in product(
            ("push", "workflow_dispatch"),
            ("refs/heads/main", "refs/heads/dev"),
            ("none", "development", "production", "invalid"),
            (None, "false", "TRUE", "true"),
            (None, "false", "true"),
            (None, "false", "true"),
        ):
            publish = (
                event == "workflow_dispatch" and ref == "refs/heads/main" and mode == "production"
                and enabled == "true" and failsafe == "true" and authority == "true"
            )
            if publish:
                self.assertEqual((event, ref, mode, enabled, failsafe, authority),
                                 ("workflow_dispatch", "refs/heads/main", "production",
                                  "true", "true", "true"))
            else:
                self.assertFalse(publish)

    def test_compilation_and_existing_validation_are_retained(self):
        self.assertIn("branches: [ main, dev ]", self.pins)
        self.assertIn("python3 tools/test_docker_publication_guard.py", self.pins)
        self.assertIn("MAVROS_COMMIT", self.pins)
        self.assertIn("platform: linux/amd64", self.build)
        self.assertIn("platform: linux/arm64", self.build)
        self.assertIn("file: ros2/Dockerfile", self.build)
        self.assertIn("cache-to: type=gha", self.build)
        self.assertNotRegex(self.build, r"(?m)^ {4}if:")

    def test_output_step_fails_closed_for_every_non_true_value(self):
        section = self.pins.split("      - name: Resolve publication mode and production gate", 1)[1]
        section = section.split("\n      - id: pins", 1)[0]
        script = textwrap.dedent(section.split("        run: |\n", 1)[1])
        for allowed in ("", "false", "TRUE"):
            with tempfile.NamedTemporaryFile() as output:
                env = dict(os.environ, DEVELOPMENT_ALLOWED=allowed, PRODUCTION_ALLOWED=allowed,
                           GITHUB_SHA="fixture-sha", GITHUB_OUTPUT=output.name)
                subprocess.run(["bash", "-c", script], env=env, check=True,
                               capture_output=True, text=True)
                self.assertEqual(Path(output.name).read_text(),
                                 "enabled=false\nproduction=false\ndevelopment=false\n"
                                 "image_tag_prefix=ci-fixture-sha\n")
        with tempfile.NamedTemporaryFile() as output:
            env = dict(os.environ, DEVELOPMENT_ALLOWED="false", PRODUCTION_ALLOWED="true",
                       GITHUB_SHA="fixture-sha", GITHUB_OUTPUT=output.name,
                       FAILSAFE_RECORD="", AUTHORITY_RECORD="")
            result = subprocess.run(["bash", "-c", script], env=env, capture_output=True,
                                    text=True, cwd=WORKFLOW.parents[2])
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(Path(output.name).read_text(), "")

    def test_development_is_manual_only_without_production_acceptance(self):
        allowed = re.search(r"^\s+DEVELOPMENT_ALLOWED: (.+)$", self.pins, re.MULTILINE).group(1)
        self.assertEqual(allowed, "${{ github.event_name == 'workflow_dispatch' && "
                         "github.ref == 'refs/heads/main' && inputs.publication_mode == 'development' }}")
        section = self.pins.split("      - name: Resolve publication mode and production gate", 1)[1]
        script = textwrap.dedent(section.split("        run: |\n", 1)[1].split("\n      - id: pins", 1)[0])
        with tempfile.NamedTemporaryFile() as output:
            env = dict(os.environ, DEVELOPMENT_ALLOWED="true", PRODUCTION_ALLOWED="false",
                       GITHUB_SHA="fixture-sha", GITHUB_OUTPUT=output.name,
                       FAILSAFE_RECORD="", AUTHORITY_RECORD="")
            subprocess.run(["bash", "-c", script], env=env, check=True,
                           capture_output=True, text=True, cwd=WORKFLOW.parents[2])
            self.assertEqual(Path(output.name).read_text(),
                             "enabled=true\nproduction=false\ndevelopment=true\n"
                             "image_tag_prefix=test-fixture-sha\n")
        with tempfile.NamedTemporaryFile() as output:
            env = dict(os.environ, DEVELOPMENT_ALLOWED="true", PRODUCTION_ALLOWED="true",
                       GITHUB_SHA="fixture-sha", GITHUB_OUTPUT=output.name)
            result = subprocess.run(["bash", "-c", script], env=env, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(Path(output.name).read_text(), "")

    def test_experimental_tags_cannot_replace_production_tags(self):
        dev, prod = self.publish.split("      - name: Define experimental manifest tag", 1)[1].split(
            "      - name: Extract production metadata", 1)
        self.assertIn("if: needs.pins.outputs.publish_development == 'true'", dev)
        self.assertIn("TEST_TAG: ${{ env.IMAGE_NAME }}:test-${{ github.sha }}", dev)
        self.assertNotIn("latest", dev)
        self.assertNotIn("type=raw", dev)
        self.assertIn("if: needs.pins.outputs.publish_production == 'true'", prod)
        self.assertIn("type=raw,value=latest", prod)
        self.assertIn("steps.development_tag.outputs.tags || steps.meta.outputs.tags", prod)
        self.assertIn("${{ needs.pins.outputs.image_tag_prefix }}-${{ matrix.ros_distro }}-amd64", prod)
        self.assertIn("${{ needs.pins.outputs.image_tag_prefix }}-${{ matrix.ros_distro }}-arm64", prod)
        self.assertNotIn(":build-${{", self.text)

    def test_records_must_match_baseline_and_have_separate_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            record = "docs/acceptance/failsafe.md"
            evidence = "docs/evidence.md"
            (root / "docs/acceptance").mkdir(parents=True)
            (root / evidence).write_text("Synthetic unit-test evidence, NOT hardware acceptance\n")
            fields = ("Acceptance-Result: PASS\nAcceptance-Kind: companion-loss-failsafe\n"
                      "Validated-ROS2-Tree: fixture-tree\nOperator-Acceptance: CONFIRMED\n"
                      "Evidence-Record: " + evidence + "\n")
            (root / record).write_text(fields)
            tracked = lambda path: path in (record, evidence)
            verify_record(root, record, "companion-loss-failsafe", "fixture-tree", tracked)
            for path, kind, tree, tracking in (
                ("", "companion-loss-failsafe", "fixture-tree", tracked),
                ("../failsafe.md", "companion-loss-failsafe", "fixture-tree", tracked),
                (record, "blade-exclusive-authority", "fixture-tree", tracked),
                (record, "companion-loss-failsafe", "different-tree", tracked),
                (record, "companion-loss-failsafe", "fixture-tree", lambda path: False),
            ):
                with self.assertRaises(ValueError):
                    verify_record(root, path, kind, tree, tracking)
            (root / record).write_text(fields.replace("Acceptance-Result: PASS", "Acceptance-Result: FAIL"))
            with self.assertRaises(ValueError):
                verify_record(root, record, "companion-loss-failsafe", "fixture-tree", tracked)

    def test_runtime_verification_is_read_only_and_experimental_only(self):
        for name in ("Snapshot production tag digests",
                     "Verify experimental manifest and unchanged production tags"):
            section = self.publish.split("      - name: " + name, 1)[1].split("\n      - name:", 1)[0]
            self.assertIn("if: needs.pins.outputs.publish_development == 'true'", section)
            self.assertIn("docker buildx imagetools inspect", section)
            self.assertNotIn("imagetools create", section)
        self.assertIn('test "$publication_digest" = "$BASELINE_LATEST"', self.publish)
        self.assertIn('test "$publication_digest" = "$BASELINE_LYRICAL"', self.publish)
        self.assertIn('[("linux","amd64"),("linux","arm64")]', self.publish)


if __name__ == "__main__":
    unittest.main()
