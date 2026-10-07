import os
import subprocess
import warnings

from ament_index_python.packages import get_package_prefix, get_package_share_directory
from launch import LaunchDescription
from launch.actions import OpaqueFunction
from launch.logging import get_logger
from launch.substitutions import EnvironmentVariable
from launch_ros.actions import Node
from mowgli_mavros_bridge.firmware_provider import PUBLIC_FIRMWARES, get_firmware_provider


def _as_bool(value):
    return value.strip().lower() in ("1", "true", "yes")


def resolve_firmware_provider(firmware, fcu_url=None, tgt_system=1, tgt_component=1):
    """Resolve the selected/detected firmware without aliases or fallback."""
    firmware = firmware.lower()
    if firmware not in PUBLIC_FIRMWARES:
        raise RuntimeError(
            "MAVROS_FIRMWARE must be " + ", ".join(PUBLIC_FIRMWARES)
        )
    if firmware == "auto":
        executable = os.path.join(
            get_package_prefix("mowgli_mavros_bridge"),
            "lib", "mowgli_mavros_bridge", "detect_mavros_firmware",
        )
        try:
            result = subprocess.run(
                [executable, fcu_url, str(tgt_system), str(tgt_component)],
                capture_output=True, text=True, timeout=12, check=True,
            )
        except (OSError, subprocess.SubprocessError) as error:
            detail = getattr(error, "stderr", None) or str(error)
            raise RuntimeError(
                f"MAVROS_FIRMWARE=auto: detection failed: {detail}"
            ) from error
        firmware = result.stdout.strip()
        if firmware not in PUBLIC_FIRMWARES or firmware == "auto" or not (
            get_firmware_provider(firmware).capabilities.heartbeat_autodetection
        ):
            raise RuntimeError("MAVROS_FIRMWARE=auto: unrecognized detection result")
        get_logger(__name__).info(
            f"Target HEARTBEAT firmware: {firmware}; detection probe exited cleanly"
        )
    return get_firmware_provider(firmware).require_bootstrap()


def resolve_mavros_profile(firmware, fcu_url=None, tgt_system=1, tgt_component=1):
    return resolve_firmware_provider(
        firmware, fcu_url, tgt_system, tgt_component
    ).mavros_profile


def _backend_nodes(
    context,
    mavros_share,
    firmware,
    fcu_url,
    gcs_url,
    system_id,
    tgt_system,
    tgt_component,
    gnss_source_mode,
    gnss_mavros_source,
    bridge_params,
    hardware_bridge_remappings,
):
    source_mode = gnss_source_mode.perform(context).strip().lower()
    if source_mode not in ("direct", "mavros"):
        raise RuntimeError("GNSS_SOURCE must be direct or mavros")

    source = gnss_mavros_source.perform(context).strip().lower()
    if source not in ("gps1", "gps2"):
        raise RuntimeError("GNSS_MAVROS_SOURCE must be gps1 or gps2")

    provider = resolve_firmware_provider(
        firmware.perform(context), fcu_url.perform(context),
        tgt_system.perform(context), tgt_component.perform(context),
    )
    profile = provider.mavros_profile
    get_logger(__name__).info(
        f"MAVROS_FIRMWARE={firmware.perform(context)}: selected MAVROS profile {profile}"
    )
    plugin_list, config = provider.parameter_files

    plugin_xml = os.path.join(
        get_package_share_directory("universal_gnss_mavros"),
        "universal_gnss_mavros_plugins.xml",
    )
    if not os.path.isfile(plugin_xml):
        raise RuntimeError("Universal GNSS MAVROS pluginlib export is unavailable")

    adapter_xml = os.path.join(
        get_package_share_directory("mavros_gnss_adapter"),
        "mavros_plugins.xml",
    )
    if not os.path.isfile(adapter_xml):
        raise RuntimeError("Mowgli GNSS adapter pluginlib export is unavailable")

    wheel_odom_config = os.path.join(
        get_package_share_directory("mavros_esc_wheel_odometry"),
        "config",
        "esc_wheel_odometry.yaml",
    )
    runtime_wheel_odom_config = "/ros2_ws/config/esc_wheel_odometry.yaml"
    battery_observer_config = os.path.join(
        get_package_share_directory("mavros_battery_observer"),
        "config",
        "battery_observer.yaml",
    )

    if not os.path.isfile(wheel_odom_config):
        raise RuntimeError(
            "ESC wheel odometry MAVROS plugin configuration is unavailable"
        )

    if not os.path.isfile(battery_observer_config):
        raise RuntimeError(
            "MAVROS battery observer configuration is unavailable"
        )

    wheel_odom_parameters = [wheel_odom_config]
    if os.path.isfile(runtime_wheel_odom_config):
        wheel_odom_parameters.append(runtime_wheel_odom_config)
    bridge_parameters = [bridge_params]
    runtime_bridge_config = "/ros2_ws/config/hardware_bridge.yaml"
    if os.path.isfile(runtime_bridge_config):
        bridge_parameters.append(runtime_bridge_config)

    # Universal GNSS owns the canonical /rtcm stream.
    #
    # GNSS_SOURCE=mavros:
    #   UG /rtcm -> MAVROS Universal GNSS plugin -> GPS_RTCM_DATA -> FCU
    #
    # GNSS_SOURCE=direct:
    #   UG /rtcm belongs to the receiver connected directly to the SoC.
    #   MAVROS must not consume or forward that correction stream.
    rtcm_input = (
        "/rtcm"
        if source_mode == "mavros"
        else "/mavros/universal_gnss/rtcm_disabled"
    )

    return [
        Node(
            package="mavros",
            executable="mavros_node",
            output="screen",
            additional_env={"GNSS_SOURCE": source_mode, "GNSS_MAVROS_SOURCE": source},
            parameters=[
                os.path.join(mavros_share, "launch", plugin_list),
                os.path.join(mavros_share, "launch", config),
                *wheel_odom_parameters,
                battery_observer_config,
                {
                    "fcu_url": fcu_url.perform(context),
                    "gcs_url": gcs_url.perform(context),
                    "system_id": int(system_id.perform(context)),
                    "tgt_system": int(tgt_system.perform(context)),
                    "tgt_component": int(tgt_component.perform(context)),
                },
            ],
            # Keep Universal GNSS MAVROS output on its private canonical
            # adapter topics in every mode. Never remap its GnssStatus type
            # onto Mowgli's /gps/status topic.
            remappings=[
                ("/rtcm", rtcm_input),
            ],
        ),
        Node(
            package="mowgli_mavros_bridge",
            executable="mavros_hardware_bridge_node",
            name="hardware_bridge",
            output="screen",
            additional_env={"MAVROS_RESOLVED_FIRMWARE": provider.name},
            parameters=bridge_parameters,
            remappings=hardware_bridge_remappings,
        ),
    ]


def generate_launch_description():
    bridge_share = get_package_share_directory("mowgli_mavros_bridge")
    mavros_share = get_package_share_directory("mavros")

    bridge_params = os.path.join(
        bridge_share,
        "config",
        "hardware_bridge_mavros.yaml",
    )

    hardware_bridge_remappings = [
        ("~/imu/data_raw", "/imu/data"),
        ("~/emergency", "/hardware_bridge/emergency"),
        ("~/status", "/hardware_bridge/status"),
        ("~/cmd_vel", "/cmd_vel"),
    ]

    mavros_firmware = EnvironmentVariable(
        "MAVROS_FIRMWARE",
        default_value="auto",
    )
    mavros_fcu_url = EnvironmentVariable(
        "MAVROS_FCU_URL",
        default_value="serial:///dev/mavros:921600",
    )
    mavros_gcs_url = EnvironmentVariable(
        "MAVROS_GCS_URL",
        default_value="",
    )
    mavros_system_id = EnvironmentVariable(
        "MAVROS_SYSTEM_ID",
        default_value="255",
    )
    mavros_tgt_system = EnvironmentVariable(
        "MAVROS_TGT_SYSTEM",
        default_value="1",
    )
    mavros_tgt_component = EnvironmentVariable(
        "MAVROS_TGT_COMPONENT",
        default_value="1",
    )

    gnss_source_mode = EnvironmentVariable(
        "GNSS_SOURCE",
        default_value="mavros",
    )
    gnss_mavros_source = EnvironmentVariable(
        "GNSS_MAVROS_SOURCE",
        default_value="gps1",
    )

    source_mode_default = os.environ.get(
        "GNSS_SOURCE",
        "mavros",
    ).strip().lower()

    if source_mode_default not in ("direct", "mavros"):
        raise RuntimeError("GNSS_SOURCE must be direct or mavros")

    mavros_source_default = os.environ.get(
        "GNSS_MAVROS_SOURCE",
        "gps1",
    ).strip().lower()

    if mavros_source_default not in ("gps1", "gps2"):
        raise RuntimeError("GNSS_MAVROS_SOURCE must be gps1 or gps2")

    # GNSS_SOURCE is now authoritative.
    #
    # The existing MAVROS_GPS1_CANONICAL variable remains accepted as a
    # deployment consistency guard while MowgliNext transitions to the new
    # GNSS_SOURCE contract.
    gps1_canonical_expected = (
        source_mode_default == "mavros"
        and mavros_source_default == "gps1"
    )

    if "MAVROS_GPS1_CANONICAL" in os.environ:
        warnings.warn(
            "MAVROS_GPS1_CANONICAL is deprecated; use GNSS_SOURCE/GNSS_MAVROS_SOURCE",
            FutureWarning,
        )
        configured = _as_bool(os.environ["MAVROS_GPS1_CANONICAL"])
        if configured != gps1_canonical_expected:
            raise RuntimeError(
                "MAVROS_GPS1_CANONICAL conflicts with "
                "GNSS_SOURCE/GNSS_MAVROS_SOURCE"
            )

    return LaunchDescription(
        [
            OpaqueFunction(
                function=_backend_nodes,
                args=[
                    mavros_share,
                    mavros_firmware,
                    mavros_fcu_url,
                    mavros_gcs_url,
                    mavros_system_id,
                    mavros_tgt_system,
                    mavros_tgt_component,
                    gnss_source_mode,
                    gnss_mavros_source,
                    bridge_params,
                    hardware_bridge_remappings,
                ],
            ),
        ]
    )
