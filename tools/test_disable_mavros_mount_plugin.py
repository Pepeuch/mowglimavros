import unittest

from disable_mavros_mount_plugin import disable_mount_control


class DisableMountControlTest(unittest.TestCase):
    def test_appends_to_existing_denylist_and_preserves_entries(self):
        source = "/**:\n  ros__parameters:\n    plugin_denylist:\n      - image_pub\n"
        result = disable_mount_control(source)
        self.assertIn("      - mount_control\n", result)
        self.assertIn("      - image_pub\n", result)

    def test_is_idempotent(self):
        source = "plugin_denylist:\n  - mount_control\n"
        self.assertEqual(disable_mount_control(source), source)

    def test_requires_real_denylist(self):
        with self.assertRaisesRegex(ValueError, "plugin_denylist"):
            disable_mount_control("plugin_allowlist: []\n")


if __name__ == "__main__":
    unittest.main()
