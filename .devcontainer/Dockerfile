# syntax=docker/dockerfile:1.7

ARG ROS_DISTRO=lyrical
FROM ros:${ROS_DISTRO}

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ARG DEBIAN_FRONTEND=noninteractive
ARG ROS_DISTRO
ARG USERNAME=ubuntu

ARG MAVROS_VERSION=2.16.0
ARG MAVROS_COMMIT=5c68b905ab30de6ce630822dc46c33467e8f23ea

ENV ROS_DISTRO=${ROS_DISTRO} \
    DEVCONTAINER_USER=${USERNAME} \
    GH_CONFIG_DIR=/home/${USERNAME}/.config/gh

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash-completion \
    build-essential \
    ccache \
    cmake \
    curl \
    docker-buildx \
    docker.io \
    geographiclib-tools \
    gh \
    git \
    jq \
    openssh-client \
    python3-colcon-common-extensions \
    python3-pip \
    python3-rosdep \
    python3-vcstool \
    sudo \
    wget \
 && rm -rf /var/lib/apt/lists/*

#
# Devcontainer user
#
RUN id "${USERNAME}" >/dev/null

RUN echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" \
    > "/etc/sudoers.d/${USERNAME}" \
 && chmod 0440 "/etc/sudoers.d/${USERNAME}"

RUN mkdir -p "/home/${USERNAME}/.config/gh" \
 && chown -R \
      "${USERNAME}:$(id --group --name "${USERNAME}")" \
      "/home/${USERNAME}"

#
# MAVROS 2.16.0 exact baseline
#
RUN mkdir -p /mavros_ws/src /opt/mowgli/mavros

RUN git clone \
      --filter=blob:none \
      --no-checkout \
      https://github.com/mavlink/mavros.git \
      /mavros_ws/src/mavros \
 && git -C /mavros_ws/src/mavros fetch \
      --depth 1 \
      origin \
      "${MAVROS_COMMIT}" \
 && test \
      "$(git -C /mavros_ws/src/mavros rev-parse FETCH_HEAD)" \
      = "${MAVROS_COMMIT}" \
 && git -C /mavros_ws/src/mavros checkout \
      --detach \
      "${MAVROS_COMMIT}" \
 && git -C /mavros_ws/src/mavros fetch \
      --depth 1 \
      origin \
      "refs/tags/${MAVROS_VERSION}:refs/tags/${MAVROS_VERSION}" \
 && test \
      "$(git -C /mavros_ws/src/mavros rev-list -n 1 "${MAVROS_VERSION}")" \
      = "${MAVROS_COMMIT}"

#
# MAVLink source dependency required on Lyrical
#
RUN git clone \
      --depth 1 \
      --branch release/rolling/mavlink \
      https://github.com/ros2-gbp/mavlink-gbp-release.git \
      /mavros_ws/src/mavlink

#
# Remaining MAVROS dependencies
#
RUN apt-get update \
 && rosdep update \
 && rosdep install \
      --from-paths /mavros_ws/src \
      --ignore-src \
      --rosdistro "${ROS_DISTRO}" \
      --skip-keys "ament_cmake_google_benchmark google-mock" \
      -r \
      -y \
 && rm -rf /var/lib/apt/lists/*

#
# Build MAVROS
#
WORKDIR /mavros_ws

RUN source "/opt/ros/${ROS_DISTRO}/setup.bash" \
 && colcon build \
      --packages-up-to mavros mavros_extras \
      --install-base /opt/mowgli/mavros \
      --cmake-args -DBUILD_TESTING=OFF \
 && test \
      "$(sed -n 's:.*<version>\(.*\)</version>.*:\1:p' \
        /mavros_ws/src/mavros/mavros/package.xml | head -n 1)" \
      = "${MAVROS_VERSION}"

#
# Shell environment
#
RUN printf '\nsource /opt/ros/%s/setup.bash\n' "${ROS_DISTRO}" \
      >> "/home/${USERNAME}/.bashrc" \
 && printf 'source /opt/mowgli/mavros/setup.bash\n' \
      >> "/home/${USERNAME}/.bashrc"

RUN chown -R \
      "${USERNAME}:$(id --group --name "${USERNAME}")" \
      /mavros_ws \
      /opt/mowgli \
      "/home/${USERNAME}"

COPY .devcontainer/init-devcontainer.sh /usr/local/bin/init-devcontainer

RUN chmod +x /usr/local/bin/init-devcontainer

WORKDIR /workspaces/mowglimavros

ENTRYPOINT ["/usr/local/bin/init-devcontainer"]
CMD ["sleep", "infinity"]