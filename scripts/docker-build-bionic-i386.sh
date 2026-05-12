#!/usr/bin/env bash
set -euo pipefail

image_tag="${IMAGE_TAG:-poulsbo:bionic-i386}"
docker_host="${DOCKER_HOST:-}"
build_network="${DOCKER_BUILD_NETWORK:-host}"

docker_args=()
if [[ -n "$docker_host" ]]; then
    docker_args+=(-H "$docker_host")
fi

docker "${docker_args[@]}" build \
    --network "$build_network" \
    -t "$image_tag" \
    -f docker/ubuntu-18.04-kbuild-i386/Dockerfile \
    .
