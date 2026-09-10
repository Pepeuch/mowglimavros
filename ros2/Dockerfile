# syntax=docker/dockerfile:1.7
ARG ROS_DISTRO=kilted
ARG MAVROS_VERSION=2.15.1
ARG MAVROS_COMMIT=22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e
ARG UNIVERSAL_GNSS_REPOSITORY=https://github.com/Pepeuch/universal-gnss.git
ARG UNIVERSAL_GNSS_COMMIT=34afbf01e770dd0b4c3863f3822a122dc25e7b7b
ARG MAVLINK_VERSION=2026.8.8
ARG GEOGRAPHICLIB_DATASET_VERSION=geographiclib-datasets-v1

FROM ros:${ROS_DISTRO}-ros-base AS geographiclib-data

ARG DEBIAN_FRONTEND=noninteractive
ARG GEOGRAPHICLIB_DATASET_VERSION

RUN apt-get update && apt-get install -y --no-install-recommends \
    bzip2 \
    ca-certificates \
    curl \
 && rm -rf /var/lib/apt/lists/*

RUN set -eux; \
    release_url="https://github.com/Pepeuch/mowglimavros/releases/download/${GEOGRAPHICLIB_DATASET_VERSION}"; \
    mkdir -p /tmp/geographiclib /usr/share/GeographicLib; \
    cd /tmp/geographiclib; \
    curl --fail --location --retry 3 --output SHA256SUMS "${release_url}/SHA256SUMS"; \
    printf '%s  %s\n' \
      c46224f8f723dc915d97179f4e1580a98d6c742fe2b82cd8fef0ecaaad13e614 egm96-5.tar.bz2 \
      6fea4c6bd56ff8ac53dbdad8d5dd505c855471d0354c4abc5c5fe048bf8350c1 egm96.tar.bz2 \
      8e71a9704c5f2714bb65581df68e30f0d84d0ad17286d00efb782e7232334c3f emm2015.tar.bz2 \
      > expected-SHA256SUMS; \
    grep --fixed-strings --file=expected-SHA256SUMS SHA256SUMS; \
    while read -r checksum archive; do \
      curl --fail --location --retry 3 --output "${archive}" "${release_url}/${archive}"; \
    done < expected-SHA256SUMS; \
    sha256sum --check expected-SHA256SUMS; \
    tar -xjf egm96-5.tar.bz2 -C /usr/share/GeographicLib; \
    tar -xjf egm96.tar.bz2 -C /usr/share/GeographicLib; \
    tar -xjf emm2015.tar.bz2 -C /usr/share/GeographicLib; \
    test -s /usr/share/GeographicLib/geoids/egm96-5.pgm; \
    test -s /usr/share/GeographicLib/gravity/egm96.egm; \
    test -s /usr/share/GeographicLib/magnetic/emm2015.wmm

FROM ros:${ROS_DISTRO}-ros-base AS mavros-build

ARG DEBIAN_FRONTEND=noninteractive
ARG ROS_DISTRO
ARG MAVROS_VERSION
ARG MAVROS_COMMIT
ARG MAVLINK_VERSION
ARG TARGETARCH

RUN set -eux; \
    apt-get update; \
    boost_headers_package=libboost-dev; \
    apt-get install -y --no-install-recommends \
      "ros-${ROS_DISTRO}-angles" \
      "ros-${ROS_DISTRO}-diagnostic-updater" \
      "ros-${ROS_DISTRO}-diagnostic-msgs" \
      "ros-${ROS_DISTRO}-eigen-stl-containers" \
      "ros-${ROS_DISTRO}-geographic-msgs" \
      "ros-${ROS_DISTRO}-mavlink" \
      "ros-${ROS_DISTRO}-message-filters" \
      "ros-${ROS_DISTRO}-pluginlib" \
      "ros-${ROS_DISTRO}-rclcpp-components" \
      "ros-${ROS_DISTRO}-rosidl-default-generators" \
      "ros-${ROS_DISTRO}-tf2-eigen" \
      "ros-${ROS_DISTRO}-tf2-ros" \
      "ros-${ROS_DISTRO}-trajectory-msgs" \
      "ros-${ROS_DISTRO}-urdf" \
      "ros-${ROS_DISTRO}-visualization-msgs" \
      "ros-${ROS_DISTRO}-yaml-cpp-vendor" \
      build-essential \
      ca-certificates \
      ccache \
      cmake \
      geographiclib-tools \
      git \
      libasio-dev \
      libconsole-bridge-dev \
      libeigen3-dev \
      libgeographiclib-dev \
      libyaml-cpp-dev \
      python3-click \
      python3-colcon-common-extensions \
      python3-empy \
      "${boost_headers_package}"; \
    installed_mavlink_version="$(dpkg-query --show --showformat='${Version}' "ros-${ROS_DISTRO}-mavlink")"; \
    case "${installed_mavlink_version}" in "${MAVLINK_VERSION}"-*) ;; *) \
      echo "Expected MAVLink ${MAVLINK_VERSION}, got ${installed_mavlink_version}" >&2; exit 1 ;; esac; \
    rm -rf /var/lib/apt/lists/*

COPY --from=geographiclib-data /usr/share/GeographicLib /usr/share/GeographicLib

RUN set -eux; \
    git clone --filter=blob:none --no-checkout https://github.com/mavlink/mavros.git /mavros_ws/src/mavros; \
    git -C /mavros_ws/src/mavros fetch --depth 1 origin "${MAVROS_COMMIT}"; \
    test "$(git -C /mavros_ws/src/mavros rev-parse FETCH_HEAD)" = "${MAVROS_COMMIT}"; \
    git -C /mavros_ws/src/mavros checkout --detach "${MAVROS_COMMIT}"; \
    git -C /mavros_ws/src/mavros fetch --depth 1 origin "refs/tags/${MAVROS_VERSION}:refs/tags/${MAVROS_VERSION}"; \
    test "$(git -C /mavros_ws/src/mavros rev-list -n 1 "${MAVROS_VERSION}")" = "${MAVROS_COMMIT}"; \
    test "$(sed -n 's:.*<version>\(.*\)</version>.*:\1:p' /mavros_ws/src/mavros/mavros/package.xml | head -n 1)" = "${MAVROS_VERSION}"

WORKDIR /mavros_ws

RUN --mount=type=cache,id=ccache-${ROS_DISTRO}-${TARGETARCH}-${MAVROS_VERSION},target=/ccache \
    /bin/bash -lc "\
    source /opt/ros/${ROS_DISTRO}/setup.bash && \
    export CCACHE_DIR=/ccache && \
    export CCACHE_MAXSIZE=512M && \
    export CC='ccache gcc' && \
    export CXX='ccache g++' && \
    echo 'ccache MAVROS before:' && ccache --show-stats && du -sh \"\${CCACHE_DIR}\" && \
    colcon build --packages-up-to mavros mavros_extras \
      --install-base /opt/mowgli/mavros \
      --cmake-args -DBUILD_TESTING=OFF && \
    echo 'ccache MAVROS after:' && ccache --show-stats && du -sh \"\${CCACHE_DIR}\" && \
    printf '%s\\n' '${MAVROS_VERSION}' > /opt/mowgli/mavros/.mowgli-mavros-version \
    "

FROM mavros-build AS universal-gnss-build

ARG ROS_DISTRO
ARG MAVROS_VERSION
ARG UNIVERSAL_GNSS_REPOSITORY
ARG UNIVERSAL_GNSS_COMMIT

RUN set -eux; \
    git clone --filter=blob:none --no-checkout "${UNIVERSAL_GNSS_REPOSITORY}" /universal_gnss_ws/src/universal_gnss; \
    git -C /universal_gnss_ws/src/universal_gnss fetch --depth 1 origin "${UNIVERSAL_GNSS_COMMIT}"; \
    test "$(git -C /universal_gnss_ws/src/universal_gnss rev-parse FETCH_HEAD)" = "${UNIVERSAL_GNSS_COMMIT}"; \
    git -C /universal_gnss_ws/src/universal_gnss checkout --detach "${UNIVERSAL_GNSS_COMMIT}"; \
    test -f /universal_gnss_ws/src/universal_gnss/gnss_mavros/universal_gnss_mavros_plugins.xml; \
    grep --fixed-strings --quiet "${MAVROS_VERSION}" /universal_gnss_ws/src/universal_gnss/gnss_mavros/README.md

WORKDIR /universal_gnss_ws

RUN /bin/bash -lc "\
    source /opt/ros/${ROS_DISTRO}/setup.bash && \
    source /opt/mowgli/mavros/setup.bash && \
    colcon build --base-paths src/universal_gnss/gnss_ros2 src/universal_gnss/gnss_mavros \
      --packages-up-to universal_gnss_mavros \
      --install-base /opt/mowgli/universal_gnss \
      --cmake-args -DUNIVERSAL_GNSS_BUILD_MAVROS_PLUGIN=ON -DBUILD_TESTING=OFF && \
    test -f /opt/mowgli/universal_gnss/universal_gnss_mavros/share/universal_gnss_mavros/universal_gnss_mavros_plugins.xml && \
    printf '%s\n' '${UNIVERSAL_GNSS_COMMIT}' > /opt/mowgli/universal_gnss/.mowgli-universal-gnss-commit \
    "

FROM universal-gnss-build AS workspace-build

ARG ROS_DISTRO
ARG BUILD_TYPE=Release
ARG TARGETARCH

COPY ros2/src/ /ros2_ws/src/
COPY tools/ /ros2_ws/tools/
COPY ros2/Dockerfile /ros2_ws/Dockerfile
WORKDIR /ros2_ws

RUN --mount=type=cache,id=ccache-${ROS_DISTRO}-${TARGETARCH}-${MAVROS_VERSION},target=/ccache \
    /bin/bash -lc "\
    source /opt/ros/${ROS_DISTRO}/setup.bash && \
    source /opt/mowgli/mavros/setup.bash && \
    source /opt/mowgli/universal_gnss/setup.bash && \
    python3 tools/mowgli_interface_contract.py check --interface-root src/mowgli_interfaces && \
    python3 tools/test_external_backend_contract.py && \
    export CCACHE_DIR=/ccache && \
    export CCACHE_MAXSIZE=512M && \
    export CC='ccache gcc' && \
    export CXX='ccache g++' && \
    echo 'ccache workspace before:' && ccache --show-stats && du -sh \"\${CCACHE_DIR}\" && \
    colcon build \
      --cmake-args -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TESTING=OFF \
      --parallel-workers \$(nproc) && \
    echo 'ccache workspace after:' && ccache --show-stats && du -sh \"\${CCACHE_DIR}\" \
    "

FROM ros:${ROS_DISTRO}-ros-base AS runtime

ARG DEBIAN_FRONTEND=noninteractive
ARG ROS_DISTRO
ARG MAVROS_VERSION
ARG UNIVERSAL_GNSS_COMMIT

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
    "ros-${ROS_DISTRO}-angles" \
    "ros-${ROS_DISTRO}-diagnostic-updater" \
    "ros-${ROS_DISTRO}-diagnostic-msgs" \
    "ros-${ROS_DISTRO}-eigen-stl-containers" \
    "ros-${ROS_DISTRO}-geographic-msgs" \
    "ros-${ROS_DISTRO}-message-filters" \
    "ros-${ROS_DISTRO}-pluginlib" \
    "ros-${ROS_DISTRO}-rmw-cyclonedds-cpp" \
    "ros-${ROS_DISTRO}-rosidl-default-runtime" \
    "ros-${ROS_DISTRO}-tf2-eigen" \
    "ros-${ROS_DISTRO}-tf2-ros" \
    "ros-${ROS_DISTRO}-trajectory-msgs" \
    "ros-${ROS_DISTRO}-urdf" \
    "ros-${ROS_DISTRO}-visualization-msgs" \
    "ros-${ROS_DISTRO}-yaml-cpp-vendor" \
    geographiclib-tools \
 && rm -rf /var/lib/apt/lists/*

COPY --from=geographiclib-data /usr/share/GeographicLib /usr/share/GeographicLib
COPY --from=mavros-build /opt/mowgli/mavros /opt/mowgli/mavros
COPY --from=universal-gnss-build /opt/mowgli/universal_gnss /opt/mowgli/universal_gnss
COPY --from=workspace-build /ros2_ws/install/ /ros2_ws/install/
COPY ros2_entrypoint.sh /ros2_entrypoint.sh

RUN chmod +x /ros2_entrypoint.sh \
 && test "$(cat /opt/mowgli/mavros/.mowgli-mavros-version)" = "${MAVROS_VERSION}" \
 && test "$(cat /opt/mowgli/universal_gnss/.mowgli-universal-gnss-commit)" = "${UNIVERSAL_GNSS_COMMIT}" \
 && test -f /opt/mowgli/universal_gnss/universal_gnss_mavros/share/universal_gnss_mavros/universal_gnss_mavros_plugins.xml \
 && test -s /usr/share/GeographicLib/geoids/egm96-5.pgm \
 && test -s /usr/share/GeographicLib/gravity/egm96.egm \
 && test -s /usr/share/GeographicLib/magnetic/emm2015.wmm

ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

ENTRYPOINT ["/ros2_entrypoint.sh"]
CMD []
