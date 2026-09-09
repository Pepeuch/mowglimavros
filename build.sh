#!/usr/bin/env bash
set -euo pipefail

OWNER="${OWNER:-mowglifrenchtouch}"
REPO="${REPO:-mowglimavros}"
IMAGE_NAME="${IMAGE_NAME:-mowgli-mavros-sidecar}"
ROS_DISTRO="${ROS_DISTRO:-kilted}"
MAVROS_VERSION="${MAVROS_VERSION:-2.15.1}"
MAVROS_COMMIT="${MAVROS_COMMIT:-22ae5b7cc7cdb4cb9c2070a8213c72dae445a23e}"
UNIVERSAL_GNSS_REPOSITORY="${UNIVERSAL_GNSS_REPOSITORY:-https://github.com/Pepeuch/universal-gnss.git}"
UNIVERSAL_GNSS_COMMIT="${UNIVERSAL_GNSS_COMMIT:-34afbf01e770dd0b4c3863f3822a122dc25e7b7b}"
MAVLINK_VERSION="${MAVLINK_VERSION:-2026.8.8}"
GEOGRAPHICLIB_DATASET_VERSION="${GEOGRAPHICLIB_DATASET_VERSION:-geographiclib-datasets-v1}"

if [[ -v TAG ]]; then
  TAG="${TAG}"
elif [[ "${ROS_DISTRO}" == "kilted" ]]; then
  TAG="latest"
else
  TAG="${ROS_DISTRO}"
fi

DOCKERFILE="ros2/Dockerfile"
PLATFORMS="linux/amd64,linux/arm64"
CACHE_SCOPE="${CACHE_SCOPE:-${IMAGE_NAME}-${ROS_DISTRO}-${MAVROS_VERSION}}"

FULL_IMAGE="ghcr.io/${OWNER}/${REPO}/${IMAGE_NAME}:${TAG}"

info() { echo -e "\n[INFO] $*"; }
error() { echo -e "\n[ERROR] $*" >&2; }

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    error "Command not found: $1"
    exit 1
  }
}

ensure_repo_root() {
  [[ -d .git ]] || { error "Run from repo root"; exit 1; }
}

ensure_dockerfile() {
  [[ -f "$DOCKERFILE" ]] || {
    error "Dockerfile not found: $DOCKERFILE"
    exit 1
  }
}

ensure_buildx() {
  docker buildx inspect mowgli-builder >/dev/null 2>&1 || \
    docker buildx create --name mowgli-builder --use >/dev/null

  docker buildx inspect --bootstrap >/dev/null
}

build_image() {
  ensure_buildx

  info "Building & pushing ${FULL_IMAGE}"
  info "Platforms: ${PLATFORMS}"
  info "ROS distro: ${ROS_DISTRO}"
  info "MAVROS: ${MAVROS_VERSION} (${MAVROS_COMMIT})"
  info "Universal GNSS: ${UNIVERSAL_GNSS_COMMIT}"
  info "MAVLink: ${MAVLINK_VERSION}"

  docker buildx build \
    --platform "${PLATFORMS}" \
    -f "${DOCKERFILE}" \
    -t "${FULL_IMAGE}" \
    --build-arg BUILDKIT_INLINE_CACHE=1 \
    --build-arg ROS_DISTRO="${ROS_DISTRO}" \
    --build-arg MAVROS_VERSION="${MAVROS_VERSION}" \
    --build-arg MAVROS_COMMIT="${MAVROS_COMMIT}" \
    --build-arg UNIVERSAL_GNSS_REPOSITORY="${UNIVERSAL_GNSS_REPOSITORY}" \
    --build-arg UNIVERSAL_GNSS_COMMIT="${UNIVERSAL_GNSS_COMMIT}" \
    --build-arg MAVLINK_VERSION="${MAVLINK_VERSION}" \
    --build-arg GEOGRAPHICLIB_DATASET_VERSION="${GEOGRAPHICLIB_DATASET_VERSION}" \
    --cache-from type=gha,scope=${CACHE_SCOPE} \
    --cache-to type=gha,mode=max,scope=${CACHE_SCOPE} \
    --push \
    .
}

require_cmd docker
ensure_repo_root
ensure_dockerfile
build_image

info "Done 🚀"
echo "Image pushed:"
echo "  - ${FULL_IMAGE}"
