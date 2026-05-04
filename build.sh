#!/usr/bin/env bash
set -euo pipefail

OWNER="${OWNER:-mowglifrenchtouch}"
REPO="${REPO:-mowglimavros}"
IMAGE_NAME="${IMAGE_NAME:-mowgli-mavros-sidecar}"
TAG="${TAG:-latest}"

DOCKERFILE="ros2/Dockerfile"
PLATFORMS="linux/amd64,linux/arm64"

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

  docker buildx build \
    --platform "${PLATFORMS}" \
    -f "${DOCKERFILE}" \
    -t "${FULL_IMAGE}" \
    --build-arg BUILDKIT_INLINE_CACHE=1 \
    --cache-from type=gha,scope=${IMAGE_NAME} \
    --cache-to type=gha,mode=max,scope=${IMAGE_NAME} \
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