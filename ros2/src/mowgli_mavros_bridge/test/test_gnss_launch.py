#!/usr/bin/env python3
"""Exercise the GNSS launch environment and transitional compatibility guard."""
import importlib.util
import os
from pathlib import Path
import unittest
from unittest.mock import patch
import warnings


path = Path(__file__).resolve().parents[1] / "launch/mavros_backend.launch.py"
spec = importlib.util.spec_from_file_location("mavros_backend_launch", path)
launch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launch)


class Value:
    def __init__(self, value):
        self.value = value

    def perform(self, _context):
        return self.value


class GnssLaunchTest(unittest.TestCase):
    def setUp(self):
        self.package_patch = patch.object(launch, "get_package_share_directory", return_value="/test")
        self.package_patch.start()
        self.addCleanup(self.package_patch.stop)

    def test_mode_and_receiver_reach_mavros_and_rtcm_is_gated(self):
        for mode in ("direct", "mavros"):
            for source in ("gps1", "gps2"):
                with self.subTest(mode=mode, source=source), \
                        patch.object(launch.os.path, "isfile", return_value=True), \
                        patch.object(launch, "Node", side_effect=lambda **kwargs: kwargs):
                    result = launch._mavros_node(
                        None, "/test", *map(Value, (
                            "apm", "udp://127.0.0.1:0@127.0.0.1:19999", "", "255", "1", "1",
                            mode, source)))
                    self.assertEqual(result[0]["additional_env"], {
                        "GNSS_SOURCE": mode, "GNSS_MAVROS_SOURCE": source})
                    rtcm = "/rtcm" if mode == "mavros" else "/mavros/universal_gnss/rtcm_disabled"
                    self.assertEqual(result[0]["remappings"], [("/rtcm", rtcm)])

    def test_deprecated_flag_checks_consistency_but_does_not_control_publication(self):
        for mode in ("direct", "mavros"):
            for source in ("gps1", "gps2"):
                expected = mode == "mavros" and source == "gps1"
                env = {"GNSS_SOURCE": mode, "GNSS_MAVROS_SOURCE": source}
                with self.subTest(mode=mode, source=source), patch.dict(os.environ, env, clear=True):
                    launch.generate_launch_description()  # Flag is optional for both sources.
                    os.environ["MAVROS_GPS1_CANONICAL"] = str(expected).lower()
                    with warnings.catch_warnings(record=True) as caught:
                        warnings.simplefilter("always")
                        launch.generate_launch_description()
                    self.assertTrue(any("deprecated" in str(item.message) for item in caught))
                    os.environ["MAVROS_GPS1_CANONICAL"] = str(not expected).lower()
                    with warnings.catch_warnings(), self.assertRaisesRegex(RuntimeError, "conflicts"):
                        warnings.simplefilter("ignore", FutureWarning)
                        launch.generate_launch_description()

    def test_invalid_selection_is_rejected(self):
        for env in ({"GNSS_SOURCE": "invalid"}, {"GNSS_MAVROS_SOURCE": "gps3"}):
            with patch.dict(os.environ, env, clear=True), self.assertRaises(RuntimeError):
                launch.generate_launch_description()


if __name__ == "__main__":
    unittest.main()
