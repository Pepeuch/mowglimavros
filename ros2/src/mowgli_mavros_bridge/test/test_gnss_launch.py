#!/usr/bin/env python3
"""Exercise firmware selection and the GNSS launch environment contract."""
import importlib.util
import os
import subprocess
import sys
from pathlib import Path
import unittest
from unittest.mock import patch

from launch import LaunchContext
import warnings


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))

path = Path(__file__).resolve().parents[1] / "launch/mavros_backend.launch.py"
spec = importlib.util.spec_from_file_location("mavros_backend_launch", path)
launch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launch)


class Value:
    def __init__(self, value):
        self.value = value

    def perform(self, _context):
        return self.value


def backend_nodes(context, mavros_share, *values):
    return launch._backend_nodes(
        context, mavros_share, *values,
        bridge_params="/bridge.yaml", hardware_bridge_remappings=[("~/status", "/hardware_bridge/status")],
    )


class FirmwareLaunchTest(unittest.TestCase):
    def test_supported_profiles_preserve_node_configuration(self):
        for firmware, profile in (("ardupilot", "apm"), ("px4", "px4")):
            for selected in (firmware, firmware.upper()):
                with self.subTest(firmware=selected), \
                        patch.object(launch, "get_package_share_directory", return_value="/test"), \
                        patch.object(launch.os.path, "isfile", return_value=True), \
                        patch.object(launch, "Node", side_effect=lambda **kwargs: kwargs), \
                        patch.object(launch.subprocess, "run") as probe:
                    result = backend_nodes(
                        None, "/mavros", *map(Value, (
                            selected, "serial:///dev/mavros:921600", "", "255", "1", "1",
                            "mavros", "gps1")))
                    self.assertEqual(result[:1], [{
                        "package": "mavros", "executable": "mavros_node", "output": "screen",
                        "additional_env": {"GNSS_SOURCE": "mavros", "GNSS_MAVROS_SOURCE": "gps1"},
                        "parameters": [
                            f"/mavros/launch/{profile}_pluginlists.yaml",
                            f"/mavros/launch/{profile}_config.yaml",
                            "/test/config/esc_wheel_odometry.yaml",
                            "/ros2_ws/config/esc_wheel_odometry.yaml",
                            "/test/config/battery_observer.yaml",
                            {"fcu_url": "serial:///dev/mavros:921600", "gcs_url": "",
                             "system_id": 255, "tgt_system": 1, "tgt_component": 1},
                        ],
                        "remappings": [("/rtcm", "/rtcm")],
                    }])
                    probe.assert_not_called()

    def test_unimplemented_firmware_never_launches_a_node(self):
        for firmware in ("betaflight", "inav", "mowgli"):
            with self.subTest(firmware=firmware), patch.object(launch, "Node") as node, \
                    self.assertRaisesRegex(NotImplementedError, f"{firmware}: not implemented"):
                backend_nodes(None, "/test", *map(Value, (
                    firmware, "unused", "", "255", "1", "1", "mavros", "gps1")))
            node.assert_not_called()

    def test_alias_and_unknown_values_are_rejected(self):
        for firmware in ("apm", "APM", "", "unknown", " ardupilot "):
            with self.subTest(firmware=firmware), self.assertRaisesRegex(RuntimeError, "MAVROS_FIRMWARE must be"):
                launch.resolve_mavros_profile(firmware)

    def test_environment_selection_and_default_without_legacy_fallback(self):
        environments = (
            ({}, "px4", True),
            ({"MAVROS_FIRMWARE": "auto"}, "px4", True),
            ({"MAVROS_FIRMWARE": "ardupilot"}, "apm", False),
            ({"MAVROS_FIRMWARE": "px4"}, "px4", False),
            ({"MAVROS_" + "AUTOPILOT": "ardupilot"}, "px4", True),
        )
        for env, profile, detected in environments:
            with self.subTest(env=env), patch.dict(os.environ, env, clear=True), \
                    patch.object(launch, "get_package_share_directory", return_value="/test"), \
                    patch.object(launch, "get_package_prefix", return_value="/prefix"), \
                    patch.object(launch.subprocess, "run", return_value=subprocess.CompletedProcess(
                        [], 0, "px4\n", "")) as probe, \
                    patch.object(launch.os.path, "isfile", return_value=True), \
                    patch.object(launch, "Node", side_effect=lambda **kwargs: kwargs):
                description = launch.generate_launch_description()
                result = description.entities[0].execute(LaunchContext())
                self.assertEqual(result[0]["parameters"][0], f"/test/launch/{profile}_pluginlists.yaml")
                self.assertEqual(probe.call_count, int(detected))

    def test_default_detection_failure_never_falls_back_to_ardupilot(self):
        with patch.dict(os.environ, {}, clear=True), \
                patch.object(launch, "get_package_share_directory", return_value="/test"), \
                patch.object(launch, "get_package_prefix", return_value="/prefix"), \
                patch.object(launch.subprocess, "run", side_effect=OSError("no FCU")), \
                patch.object(launch, "Node", side_effect=lambda **kwargs: kwargs):
            description = launch.generate_launch_description()
            with self.assertRaisesRegex(RuntimeError, "auto: detection failed"):
                description.entities[0].execute(LaunchContext())

    def test_one_resolution_drives_profile_and_cpp_provider_without_changing_bridge(self):
        remappings = [
            ("~/imu/data_raw", "/imu/data"),
            ("~/emergency", "/hardware_bridge/emergency"),
            ("~/status", "/hardware_bridge/status"),
            ("~/cmd_vel", "/cmd_vel"),
        ]
        for selected, detected in (("auto", "ardupilot"), ("auto", "px4"),
                                   ("ardupilot", "ardupilot"), ("px4", "px4")):
            env = {"MAVROS_FIRMWARE": selected}
            with self.subTest(selected=selected, detected=detected), \
                        patch.dict(os.environ, env, clear=True), \
                        patch.object(launch, "get_package_share_directory", return_value="/test"), \
                        patch.object(launch, "get_package_prefix", return_value="/prefix"), \
                        patch.object(launch.os.path, "isfile", return_value=True), \
                        patch.object(launch, "Node", side_effect=lambda **kwargs: kwargs), \
                        patch.object(launch.subprocess, "run", return_value=subprocess.CompletedProcess(
                            [], 0, detected + "\n", "")) as probe:
                    description = launch.generate_launch_description()
                    result = description.entities[0].execute(LaunchContext())
                    self.assertEqual(len(result), 2)
                    self.assertEqual(probe.call_count, int(selected == "auto"))
                    profile = "apm" if detected == "ardupilot" else "px4"
                    self.assertEqual(result[0]["parameters"][0], f"/test/launch/{profile}_pluginlists.yaml")
                    self.assertEqual(result[1], {
                        "package": "mowgli_mavros_bridge",
                        "executable": "mavros_hardware_bridge_node",
                        "name": "hardware_bridge", "output": "screen",
                        "additional_env": {"MAVROS_RESOLVED_FIRMWARE": detected},
                        "parameters": ["/test/config/hardware_bridge_mavros.yaml", "/ros2_ws/config/hardware_bridge.yaml"],
                        "remappings": remappings,
                    })

    def test_auto_uses_target_transport_and_resolves_detected_profile(self):
        for firmware, profile in (("ardupilot", "apm"), ("px4", "px4")):
            with self.subTest(firmware=firmware), \
                    patch.object(launch, "get_package_prefix", return_value="/prefix"), \
                    patch.object(launch.subprocess, "run", return_value=subprocess.CompletedProcess(
                        [], 0, firmware + "\n", "")) as probe:
                self.assertEqual(launch.resolve_mavros_profile("auto", "udp://:14550", 42, 1), profile)
                probe.assert_called_once_with(
                    ["/prefix/lib/mowgli_mavros_bridge/detect_mavros_firmware", "udp://:14550", "42", "1"],
                    capture_output=True, text=True, timeout=12, check=True)

    def test_auto_detection_failure_has_no_fallback(self):
        errors = (OSError("device unavailable"), subprocess.TimeoutExpired("probe", 12),
                  subprocess.CalledProcessError(1, "probe", stderr="unknown FCU"))
        for error in errors:
            with self.subTest(error=error), \
                    patch.object(launch, "get_package_prefix", return_value="/prefix"), \
                    patch.object(launch.subprocess, "run", side_effect=error), \
                    self.assertRaisesRegex(RuntimeError, "auto: detection failed"):
                launch.resolve_mavros_profile("auto", "unused")
        for output in ("auto", "apm", "", "betaflight"):
            with self.subTest(output=output), \
                    patch.object(launch, "get_package_prefix", return_value="/prefix"), \
                    patch.object(launch.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, output, "")), \
                    self.assertRaisesRegex(RuntimeError, "unrecognized detection result"):
                launch.resolve_mavros_profile("auto", "unused")


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
                    result = backend_nodes(
                        None, "/test", *map(Value, (
                            "ardupilot", "udp://127.0.0.1:0@127.0.0.1:19999", "", "255", "1", "1",
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
