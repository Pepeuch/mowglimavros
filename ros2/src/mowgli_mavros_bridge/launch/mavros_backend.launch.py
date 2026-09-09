import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import EnvironmentVariable, LaunchConfiguration
from launch_ros.actions import Node
import yaml


def _mavros_node(context, mavros_share, autopilot, fcu_url, gcs_url, tgt_system, tgt_component, gnss_source):
    source = gnss_source.perform(context)
    if source not in ("gps1", "gps2"):
        raise RuntimeError("GNSS_MAVROS_SOURCE must be gps1 or gps2")
    autopilot_value = autopilot.perform(context).lower()
    if autopilot_value in ("ardupilot", "apm"):
        plugin_list = "apm_pluginlists.yaml"
        config = "apm_config.yaml"
    elif autopilot_value == "px4":
        plugin_list = "px4_pluginlists.yaml"
        config = "px4_config.yaml"
    else:
        raise RuntimeError("MAVROS_AUTOPILOT must be ardupilot, apm, or px4")
    plugin_xml = os.path.join(
        get_package_share_directory("universal_gnss_mavros"),
        "universal_gnss_mavros_plugins.xml",
    )
    if not os.path.isfile(plugin_xml):
        raise RuntimeError("Universal GNSS MAVROS pluginlib export is unavailable")
    source_root = f"/mavros/universal_gnss/{source}"
    return [
        Node(
            package="mavros",
            executable="mavros_node",
            output="screen",
            parameters=[
                os.path.join(mavros_share, "launch", plugin_list),
                os.path.join(mavros_share, "launch", config),
                {
                    "fcu_url": fcu_url.perform(context),
                    "gcs_url": gcs_url.perform(context),
                    "tgt_system": int(tgt_system.perform(context)),
                    "tgt_component": int(tgt_component.perform(context)),
                },
            ],
            remappings=[
                (f"{source_root}/fix", "/gps/fix"),
                (f"{source_root}/status", "/gps/status"),
            ],
        )
    ]


def generate_launch_description():
    bridge_share = get_package_share_directory("mowgli_mavros_bridge")
    ntrip_share = get_package_share_directory("mowgli_ntrip_client")
    mavros_share = get_package_share_directory("mavros")

    robot_config_path = "/ros2_ws/config/mowgli_robot.yaml"
    robot_params = {}
    if os.path.isfile(robot_config_path):
        with open(robot_config_path, "r", encoding="utf-8") as config_file:
            robot_config = yaml.safe_load(config_file) or {}
        robot_params = robot_config.get("mowgli", {}).get("ros__parameters", {})

    bridge_params = os.path.join(bridge_share, "config", "hardware_bridge_mavros.yaml")
    ntrip_launch = os.path.join(ntrip_share, "launch", "mowgli_ntrip_client.launch.py")

    hardware_bridge_remappings = [
        ("~/imu/data_raw", "/imu/data"),
        ("~/wheel_odom", "/wheel_odom"),
        ("~/emergency", "/hardware_bridge/emergency"),
        ("~/power", "/hardware_bridge/power"),
        ("~/status", "/hardware_bridge/status"),
        ("~/cmd_vel", "/cmd_vel"),
    ]

    mavros_autopilot = EnvironmentVariable("MAVROS_AUTOPILOT", default_value="ardupilot")
    mavros_fcu_url = EnvironmentVariable("MAVROS_FCU_URL", default_value="serial:///dev/mavros:921600")
    mavros_gcs_url = EnvironmentVariable("MAVROS_GCS_URL", default_value="")
    mavros_tgt_system = EnvironmentVariable("MAVROS_TGT_SYSTEM", default_value="1")
    mavros_tgt_component = EnvironmentVariable("MAVROS_TGT_COMPONENT", default_value="1")
    gnss_mavros_source = EnvironmentVariable("GNSS_MAVROS_SOURCE", default_value="gps1")
    use_ntrip_default = os.environ.get(
        "NTRIP_ENABLED", str(robot_params.get("ntrip_enabled", False)).lower()
    )
    ntrip_host_default = os.environ.get("NTRIP_HOST", str(robot_params.get("ntrip_host", "127.0.0.1")))
    ntrip_port_default = os.environ.get("NTRIP_PORT", str(robot_params.get("ntrip_port", 2101)))
    ntrip_mountpoint_default = os.environ.get(
        "NTRIP_MOUNTPOINT", str(robot_params.get("ntrip_mountpoint", ""))
    )
    ntrip_username_default = os.environ.get("NTRIP_USERNAME", str(robot_params.get("ntrip_user", "")))
    ntrip_password_default = os.environ.get("NTRIP_PASSWORD", str(robot_params.get("ntrip_password", "")))
    use_ntrip = LaunchConfiguration("use_ntrip")

    return LaunchDescription(
        [
            DeclareLaunchArgument("use_ntrip", default_value=use_ntrip_default),
            OpaqueFunction(
                function=_mavros_node,
                args=[
                    mavros_share,
                    mavros_autopilot,
                    mavros_fcu_url,
                    mavros_gcs_url,
                    mavros_tgt_system,
                    mavros_tgt_component,
                    gnss_mavros_source,
                ],
            ),
            Node(
                package="mowgli_mavros_bridge",
                executable="mavros_hardware_bridge_node",
                name="hardware_bridge",
                output="screen",
                parameters=[bridge_params],
                remappings=hardware_bridge_remappings,
            ),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(ntrip_launch),
                condition=IfCondition(use_ntrip),
                launch_arguments={
                    "enabled": "true",
                    "host": ntrip_host_default,
                    "port": ntrip_port_default,
                    "mountpoint": ntrip_mountpoint_default,
                    "username": ntrip_username_default,
                    "password": ntrip_password_default,
                    "frame_id": EnvironmentVariable("NTRIP_FRAME_ID", default_value="gps"),
                    "user_agent": EnvironmentVariable("NTRIP_USER_AGENT", default_value="mowgli_ntrip_client/0.1"),
                    "reconnect_delay_ms": EnvironmentVariable("NTRIP_RECONNECT_DELAY_MS", default_value="5000"),
                    "connect_timeout_ms": EnvironmentVariable("NTRIP_CONNECT_TIMEOUT_MS", default_value="5000"),
                    "read_timeout_ms": EnvironmentVariable("NTRIP_READ_TIMEOUT_MS", default_value="15000"),
                    "status_log_period_ms": EnvironmentVariable("NTRIP_STATUS_LOG_PERIOD_MS", default_value="10000"),
                }.items(),
            ),
        ]
    )
