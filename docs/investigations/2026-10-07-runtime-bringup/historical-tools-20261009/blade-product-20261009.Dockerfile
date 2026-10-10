FROM mowgli-mavros-sidecar:off-only-20261008
COPY --chown=0:0 build/mavros_hardware_bridge_node /ros2_ws/install/mowgli_mavros_bridge/lib/mowgli_mavros_bridge/mavros_hardware_bridge_node
COPY --chown=0:0 mowgli_mavros_bridge/include/mavros_hardware_bridge_node.hpp /ros2_ws/install/mowgli_mavros_bridge/include/mavros_hardware_bridge_node.hpp
COPY --chown=0:0 mowgli_mavros_bridge/include/mowgli_mavros_bridge/blade_control.hpp /ros2_ws/install/mowgli_mavros_bridge/include/mowgli_mavros_bridge/blade_control.hpp
COPY --chown=0:0 mowgli_mavros_bridge/config/hardware_bridge_mavros.yaml /ros2_ws/install/mowgli_mavros_bridge/share/mowgli_mavros_bridge/config/hardware_bridge_mavros.yaml
