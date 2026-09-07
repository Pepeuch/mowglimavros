#!/usr/bin/env bash
set -eo pipefail

: "${ROS_DISTRO:=kilted}"
: "${DEVCONTAINER_USER:=ubuntu}"
source "/opt/ros/${ROS_DISTRO}/setup.bash"
set -u

if [[ -S /var/run/docker.sock ]]; then
  docker_socket_gid="$(stat --format='%g' /var/run/docker.sock)"
  docker_socket_group="$(getent group "${docker_socket_gid}" | cut --delimiter=: --fields=1 || true)"

  if [[ -z "${docker_socket_group}" ]]; then
    docker_socket_group=docker-host
    groupadd --gid "${docker_socket_gid}" "${docker_socket_group}"
  fi

  usermod --append --groups "${docker_socket_group}" "${DEVCONTAINER_USER}"
fi

user_home="$(getent passwd "${DEVCONTAINER_USER}" | cut --delimiter=: --fields=6)"
mkdir -p "${user_home}/.config/gh"
chown -R "${DEVCONTAINER_USER}:$(id --group --name "${DEVCONTAINER_USER}")" "${user_home}/.config/gh"

exec "$@"
