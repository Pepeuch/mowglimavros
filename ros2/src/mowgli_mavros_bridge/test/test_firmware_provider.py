#!/usr/bin/env python3
"""Provider capabilities and firmware bootstrap metadata contracts."""
from dataclasses import FrozenInstanceError
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from mowgli_mavros_bridge.firmware_provider import PUBLIC_FIRMWARES, get_firmware_provider


class FirmwareProviderTest(unittest.TestCase):
    def test_profiles_and_capabilities(self):
        for firmware, profile in (("ardupilot", "apm"), ("px4", "px4")):
            with self.subTest(firmware=firmware):
                provider = get_firmware_provider(firmware)
                self.assertEqual(provider.name, firmware)
                self.assertEqual(provider.mavros_profile, profile)
                self.assertEqual(provider.parameter_files, (
                    profile + "_pluginlists.yaml", profile + "_config.yaml"))
                self.assertTrue(provider.capabilities.mavros_bootstrap)
                self.assertTrue(provider.capabilities.heartbeat_autodetection)
                self.assertIs(provider.require_bootstrap(), provider)
                self.assertIs(get_firmware_provider(firmware.upper()), provider)

    def test_recognized_unimplemented_providers_have_no_capabilities(self):
        for firmware in ("betaflight", "inav", "mowgli"):
            with self.subTest(firmware=firmware):
                provider = get_firmware_provider(firmware)
                self.assertEqual(provider.name, firmware)
                self.assertIsNone(provider.mavros_profile)
                self.assertFalse(provider.capabilities.mavros_bootstrap)
                self.assertFalse(provider.capabilities.heartbeat_autodetection)
                with self.assertRaisesRegex(NotImplementedError, firmware + ": not implemented"):
                    provider.require_bootstrap()
                with self.assertRaises(NotImplementedError):
                    _ = provider.parameter_files

    def test_auto_is_selection_policy_not_a_concrete_provider(self):
        self.assertEqual(PUBLIC_FIRMWARES, ("ardupilot", "px4", "betaflight", "inav", "mowgli", "auto"))
        for value in ("auto", "apm", "", "unknown", " ardupilot "):
            with self.subTest(value=value), self.assertRaises(RuntimeError):
                get_firmware_provider(value)

    def test_provider_metadata_is_immutable(self):
        provider = get_firmware_provider("ardupilot")
        with self.assertRaises(FrozenInstanceError):
            provider.mavros_profile = "px4"
        with self.assertRaises(FrozenInstanceError):
            provider.capabilities.mavros_bootstrap = False


if __name__ == "__main__":
    unittest.main()
