#!/usr/bin/env python3
"""Focused MM-607 regression tests for the external MAVROS backend contract."""

import json
import re
import subprocess
import sys
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / ("ros2/src" if (ROOT / "ros2/src").is_dir() else "src")
LOCK = json.loads((ROOT / "tools/external_backend_contract.lock.json").read_text())


def read(relative):
    relative = Path(relative)
    if not (ROOT / relative).exists() and relative.parts[0] == "ros2":
        relative = Path(*relative.parts[1:])
    return (ROOT / relative).read_text()


class ExternalBackendContractTest(unittest.TestCase):
    def test_mm602_interface_lock(self):
        result = subprocess.run(
            [
                sys.executable,
                "tools/mowgli_interface_contract.py",
                "check",
                "--interface-root",
                str(SRC / "mowgli_interfaces"),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

        interfaces = json.loads(
            (SRC / "mowgli_interfaces/interface-contract.lock.json").read_text()
        )
        self.assertEqual(
            interfaces["source_repository"],
            "https://github.com/mowglinext/mowglinext.git",
        )
        self.assertEqual(
            interfaces["source_path"],
            "ros2/src/mowgli_interfaces",
        )
        self.assertNotIn("external_interface_sources", interfaces)

    def test_pins_match_the_contract_lock(self):
        docker = read("ros2/Dockerfile")

        args = dict(
            re.findall(
                r"^ARG ([A-Z_]+)=([^\n]+)$",
                docker,
                re.MULTILINE,
            )
        )

        self.assertEqual(
            args["MAVROS_VERSION"],
            LOCK["pins"]["mavros"]["version"],
        )
        self.assertEqual(
            args["MAVROS_COMMIT"],
            LOCK["pins"]["mavros"]["commit"],
        )
        self.assertEqual(
            args["UNIVERSAL_GNSS_REPOSITORY"],
            LOCK["pins"]["universal_gnss"]["repository"],
        )
        self.assertEqual(
            args["UNIVERSAL_GNSS_COMMIT"],
            LOCK["pins"]["universal_gnss"]["commit"],
        )

        self.assertIn(
            'rev-list -n 1 "${MAVROS_VERSION}")" = "${MAVROS_COMMIT}"',
            docker,
        )
        self.assertIn(
            'rev-parse FETCH_HEAD)" = "${UNIVERSAL_GNSS_COMMIT}"',
            docker,
        )
        self.assertIn("ENV ROS_DISTRO=${ROS_DISTRO}", docker)

    def test_canonical_launch_topics_types_and_frames(self):
        launch = read(
            "ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py"
        )

        self.assertIn(
            'if source not in ("gps1", "gps2")',
            launch,
        )
        self.assertIn(
            'gnss_source_mode = EnvironmentVariable(',
            launch,
        )
        self.assertIn(
            '"GNSS_SOURCE",',
            launch,
        )
        self.assertIn(
            'if source_mode not in ("direct", "mavros")',
            launch,
        )
        self.assertIn(
            '("/rtcm", rtcm_input)',
            launch,
        )
        self.assertIn(
            '"/mavros/universal_gnss/rtcm_disabled"',
            launch,
        )
        self.assertNotIn(
            '(f"{source_root}/fix", "/gps/fix")',
            launch,
        )
        self.assertNotIn(
            '(f"{source_root}/status", "/gps/status")',
            launch,
        )
        for legacy_remap in (
            '("/mavros/esc_wheel_odometry/wheel_odom", "/wheel_odom")',
            '("/mavros/battery_observer/power", "/hardware_bridge/power")',
            '("/mavros/battery_observer/battery_state", "/battery_state")',
        ):
            self.assertNotIn(legacy_remap, launch)

        bridge = read(
            "ros2/src/mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp"
        )
        battery = read(
            "ros2/src/mavros_battery_observer/src/battery_observer_plugin.cpp"
        )

        self.assertIn(
            'create_publisher<mowgli_interfaces::msg::Power>("/hardware_bridge/power"',
            battery,
        )
        self.assertIn(
            'create_publisher<sensor_msgs::msg::BatteryState>("/battery_state"',
            battery,
        )

        gnss = read("ros2/src/mavros_gnss_adapter/src/gnss_adapter_plugin.cpp")
        self.assertIn('create_publisher<sensor_msgs::msg::NavSatFix>("/gps/fix", 10)', gnss)
        self.assertNotIn('create_publisher<sensor_msgs::msg::NavSatFix>', bridge)
        self.assertNotIn('create_publisher<mowgli_interfaces::msg::GnssStatus>', bridge)
        self.assertIn('create_subscription<mowgli_interfaces::msg::GnssStatus>', bridge)
        self.assertIn('"/gps/status", sensor_qos', bridge)
        for legacy in ("gps1_canonical_enabled", "serial_gps_projection", "on_serial_gps_raw", "publish_gps_stale"):
            self.assertNotIn(legacy, bridge)
            self.assertNotIn(legacy, read("ros2/src/mowgli_mavros_bridge/include/mavros_hardware_bridge_node.hpp"))
        self.assertIn(
            'create_publisher<diagnostic_msgs::msg::DiagnosticArray>("/diagnostics"',
            bridge,
        )

        wheel = read(
            "ros2/src/mavros_esc_wheel_odometry/config/esc_wheel_odometry.yaml"
        )
        wheel_plugin = read(
            "ros2/src/mavros_esc_wheel_odometry/src/esc_wheel_odometry_plugin.cpp"
        )
        self.assertIn(
            'create_publisher<nav_msgs::msg::Odometry>("/wheel_odom"',
            wheel_plugin,
        )
        self.assertIn(
            f'frame_id: {LOCK["frames"]["wheel_odom"]}',
            wheel,
        )
        self.assertIn(
            f'child_frame_id: {LOCK["frames"]["wheel_child"]}',
            wheel,
        )

    def test_gnss_source_ownership_and_rtcm_gating(self):
        launch = read(
            "ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py"
        )
        entrypoint = read("ros2_entrypoint.sh")
        package = read(
            "ros2/src/mowgli_mavros_bridge/package.xml"
        )

        self.assertIn(
            'default_value="mavros"',
            launch,
        )
        self.assertIn(
            'source_mode == "mavros"',
            launch,
        )
        self.assertIn(
            'else "/mavros/universal_gnss/rtcm_disabled"',
            launch,
        )
        self.assertNotIn('gps1_canonical_enabled', launch)
        self.assertIn('additional_env={"GNSS_SOURCE": source_mode, "GNSS_MAVROS_SOURCE": source}', launch)
        self.assertIn('<exec_depend>mavros_gnss_adapter</exec_depend>', package)

        # Universal GNSS owns NTRIP. MowgliMAVROS only translates RTCM
        # to MAVLink when the selected GNSS receiver is behind the FCU.
        self.assertNotIn("mowgli_ntrip_client", launch)
        self.assertNotIn("IncludeLaunchDescription", launch)
        self.assertNotIn(
            "<exec_depend>mowgli_ntrip_client</exec_depend>",
            package,
        )

        self.assertIn(
            ': "${GNSS_SOURCE:=mavros}"',
            entrypoint,
        )
        self.assertIn(
            "export GNSS_SOURCE",
            entrypoint,
        )

    def test_firmware_entrypoint_exports_the_selection(self):
        entrypoint = read("ros2_entrypoint.sh")
        self.assertIn(': "${MAVROS_FIRMWARE:=auto}"', entrypoint)
        self.assertIn("export MAVROS_FIRMWARE", entrypoint)
        legacy_variable = "MAVROS_" + "AUTOPILOT"
        self.assertNotIn(legacy_variable, entrypoint)
        self.assertNotIn(legacy_variable, read(
            "ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py"))

    def test_plugin_descriptors_and_exports(self):
        descriptors = {
            "mowgli_gnss": SRC / "mavros_gnss_adapter/mavros_plugins.xml",
            "esc_wheel_odometry":
                SRC / "mavros_esc_wheel_odometry/mavros_plugins.xml",
            "battery_observer":
                SRC / "mavros_battery_observer/mavros_plugins.xml",
        }

        for name, descriptor in descriptors.items():
            root = ET.parse(descriptor).getroot()
            self.assertIn(
                name,
                {item.attrib["name"] for item in root.findall("class")},
            )
            self.assertEqual(
                root.find("class").attrib["base_class_type"],
                "mavros::plugin::PluginFactory",
            )

        for package in (
            "mavros_gnss_adapter",
            "mavros_esc_wheel_odometry",
            "mavros_battery_observer",
        ):
            self.assertIn(
                "pluginlib_export_plugin_description_file(mavros mavros_plugins.xml)",
                read(f"ros2/src/{package}/CMakeLists.txt"),
            )

        self.assertIn(
            '"universal_gnss_mavros_plugins.xml"',
            read(
                "ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py"
            ),
        )

    def test_fail_closed_configuration_defaults(self):
        wheel = read(
            "ros2/src/mavros_esc_wheel_odometry/config/esc_wheel_odometry.yaml"
        )

        for setting in (
            "left_esc_slot: -1",
            "right_esc_slot: -1",
            "left_rpm_instance: -1",
            "right_rpm_instance: -1",
            "expected_esc_telem_mav_offset: -1",
            "left_wheel_radius_m: 0.0",
            "right_wheel_radius_m: 0.0",
            "track_width_m: 0.0",
        ):
            self.assertIn(setting, wheel)

        power = read(
            "ros2/src/mavros_battery_observer/config/battery_observer.yaml"
        )

        self.assertIn("dock_battery_instance: 1", power)
        self.assertIn("traction_battery_instance: 0", power)
        self.assertIn("charger_on_voltage: 14.0", power)
        self.assertIn("charger_off_voltage: 13.0", power)

        bridge_config = read(
            "ros2/src/mowgli_mavros_bridge/config/hardware_bridge_mavros.yaml"
        )

        self.assertIn(
            "manual_control_enabled: false",
            bridge_config,
        )
        self.assertIn(
            "blade_control_enabled: false",
            bridge_config,
        )

    def test_single_owner_and_readiness_boundaries(self):
        bridge = read(
            "ros2/src/mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp"
        )
        battery = read(
            "ros2/src/mavros_battery_observer/src/battery_observer_plugin.cpp"
        )

        self.assertEqual(
            battery.count(
                'create_publisher<sensor_msgs::msg::BatteryState>("/battery_state"'
            ),
            1,
        )
        self.assertEqual(
            battery.count(
                'create_publisher<mowgli_interfaces::msg::Power>("/hardware_bridge/power"'
            ),
            1,
        )

        self.assertNotIn(
            "create_publisher<sensor_msgs::msg::BatteryState>",
            bridge,
        )
        self.assertNotIn(
            "create_publisher<mowgli_interfaces::msg::Power>",
            bridge,
        )

        self.assertIn(
            'add_value(fcu_status, "firmware_compatible", "unknown")',
            bridge,
        )

        for call in (
            "readiness_.connection(msg->connected)",
            "readiness_.gnss(",
            "readiness_.wheel(",
            "readiness_.traction(",
        ):
            self.assertIn(call, bridge)

        readiness = read(
            "ros2/src/mowgli_mavros_bridge/src/readiness_state.cpp"
        )

        for reset in (
            "gnss_stamp_.reset()",
            "wheel_stamp_.reset()",
            "traction_stamp_.reset()",
        ):
            self.assertIn(reset, readiness)

    def test_forbidden_legacy_paths_are_absent(self):
        production = [
            SRC / "mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp",
            SRC / "mowgli_mavros_bridge/launch/mavros_backend.launch.py",
            SRC / "mavros_esc_wheel_odometry/src/esc_wheel_odometry_plugin.cpp",
            SRC / "mavros_esc_wheel_odometry/config/esc_wheel_odometry.yaml",
        ]

        text = "\n".join(
            path.read_text()
            for path in production
        )

        for forbidden in (
            "/mavros/local_position/odom",
            "/mavros/global_position/raw/fix",
            "global_position/raw/fix",
            "wheel_odometry plugin",
        ):
            self.assertNotIn(forbidden, text)

        self.assertNotIn(
            "mavros_battery_observer",
            read(
                "ros2/src/mowgli_mavros_bridge/CMakeLists.txt"
            ),
        )

        self.assertIn(
            "mavros_battery_observer",
            read(
                "ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py"
            ),
        )

    def test_imu_relay_type_qos_and_remap(self):
        bridge = read(
            "ros2/src/mowgli_mavros_bridge/src/mavros_hardware_bridge_node.cpp"
        )

        input_qos = "rclcpp::SensorDataQoS()"
        output_qos = (
            'create_publisher<sensor_msgs::msg::Imu>("~/imu/data_raw", 10)'
        )

        self.assertIn(
            "create_subscription<sensor_msgs::msg::Imu>(",
            bridge,
        )
        self.assertIn(
            f'"{LOCK["imu"]["input_topic"]}",',
            bridge,
        )
        self.assertIn(
            input_qos,
            bridge,
        )
        self.assertIn(
            output_qos,
            bridge,
        )
        self.assertIn(
            "pub_imu_->publish(*msg);",
            bridge,
        )

        launch = read(
            "ros2/src/mowgli_mavros_bridge/launch/mavros_backend.launch.py"
        )

        self.assertIn(
            '("~/imu/data_raw", "/imu/data")',
            launch,
        )

        self.assertEqual(
            LOCK["topics"]["/imu/data"],
            "sensor_msgs/msg/Imu",
        )
        self.assertEqual(
            LOCK["frames"]["imu"],
            "base_link",
        )
        self.assertEqual(
            LOCK["imu"]["input_qos"],
            "best_effort/volatile/keep_last:5",
        )
        self.assertEqual(
            LOCK["imu"]["output_qos"],
            "reliable/volatile/keep_last:10",
        )


if __name__ == "__main__":
    unittest.main()