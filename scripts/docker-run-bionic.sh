#!/usr/bin/env bash
set -euo pipefail

image_tag="${IMAGE_TAG:-poulsbo:bionic}"
docker_host="${DOCKER_HOST:-}"
run_network="${DOCKER_RUN_NETWORK:-host}"

docker_args=()
if [[ -n "$docker_host" ]]; then
    docker_args+=(-H "$docker_host")
fi

cmd=(bash)
if [[ "$#" -gt 0 ]]; then
    cmd=("$@")
fi

docker "${docker_args[@]}" run --rm -it \
    --network "$run_network" \
    -v "$(pwd):/workspace" \
    -w /workspace \
    "$image_tag" \
    "${cmd[@]}"
