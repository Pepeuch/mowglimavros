# syntax=docker/dockerfile:1.7
ARG ROS_DISTRO=kilted
FROM ros:${ROS_DISTRO}-ros-base

ARG DEBIAN_FRONTEND=noninteractive
ARG ROS_DISTRO
ARG USERNAME=ubuntu

ENV ROS_DISTRO=${ROS_DISTRO} \
    DEVCONTAINER_USER=${USERNAME} \
    GH_CONFIG_DIR=/home/ubuntu/.config/gh

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash-completion \
    build-essential \
    cmake \
    ccache \
    curl \
    docker-buildx \
    docker.io \
    gh \
    git \
    jq \
    openssh-client \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-vcstool \
    "ros-${ROS_DISTRO}-ament-cmake" \
    "ros-${ROS_DISTRO}-rmw-cyclonedds-cpp" \
    "ros-${ROS_DISTRO}-rosidl-default-generators" \
    "ros-${ROS_DISTRO}-rosidl-default-runtime" \
    sudo \
    wget \
 && rm -rf /var/lib/apt/lists/*

RUN id "${USERNAME}" >/dev/null \
 && mkdir -p "/home/${USERNAME}/.config/gh" \
 && printf '\nsource /opt/ros/%s/setup.bash\n' "${ROS_DISTRO}" >> "/home/${USERNAME}/.bashrc" \
 && chown -R "${USERNAME}:$(id --group --name "${USERNAME}")" "/home/${USERNAME}"

COPY .devcontainer/init-devcontainer.sh /usr/local/bin/init-devcontainer
RUN chmod +x /usr/local/bin/init-devcontainer

ENTRYPOINT ["/usr/local/bin/init-devcontainer"]
CMD ["sleep", "infinity"]
