# syntax=docker/dockerfile:1.7
FROM ros:kilted-ros-base AS build

ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_TYPE=Release
ARG TARGETARCH

RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-kilted-mavros \
    ros-kilted-mavros-msgs \
    ros-kilted-mavros-extras \
    ros-kilted-rmw-cyclonedds-cpp \
    ros-kilted-ament-cmake \
    ros-kilted-rosidl-default-generators \
    python3-colcon-common-extensions \
    build-essential \
    cmake \
    ccache \
    libboost-system-dev \
    wget \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

RUN wget -q https://raw.githubusercontent.com/mavlink/mavros/ros2/mavros/scripts/install_geographiclib_datasets.sh -O /tmp/install_geo.sh \
 && chmod +x /tmp/install_geo.sh \
 && /tmp/install_geo.sh \
 && rm /tmp/install_geo.sh

WORKDIR /ros2_ws
COPY ros2/src/ src/

RUN --mount=type=cache,target=/ccache/${TARGETARCH} \
    /bin/bash -lc "\
    source /opt/ros/kilted/setup.bash && \
    export CCACHE_DIR=/ccache/${TARGETARCH} && \
    export CC='ccache gcc' && \
    export CXX='ccache g++' && \
    ccache --max-size=2G && \
    ccache --zero-stats && \
    colcon build \
      --cmake-args -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTING=OFF \
      --parallel-workers \$(nproc) && \
    ccache --show-stats \
    "

FROM ros:kilted-ros-base AS runtime

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-kilted-mavros \
    ros-kilted-mavros-msgs \
    ros-kilted-mavros-extras \
    ros-kilted-rmw-cyclonedds-cpp \
    ros-kilted-rosidl-default-runtime \
    libboost-system1.83.0 \
    wget \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

RUN wget -q https://raw.githubusercontent.com/mavlink/mavros/ros2/mavros/scripts/install_geographiclib_datasets.sh -O /tmp/install_geo.sh \
 && chmod +x /tmp/install_geo.sh \
 && /tmp/install_geo.sh \
 && rm /tmp/install_geo.sh

WORKDIR /ros2_ws
COPY --from=build /ros2_ws/install/ /ros2_ws/install/
COPY ros2_entrypoint.sh /ros2_entrypoint.sh
RUN chmod +x /ros2_entrypoint.sh

ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

ENTRYPOINT ["/ros2_entrypoint.sh"]
CMD []