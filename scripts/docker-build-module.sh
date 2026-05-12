#!/usr/bin/env bash
set -euo pipefail

image_tag="${IMAGE_TAG:-poulsbo:bionic}"
docker_host="${DOCKER_HOST:-}"
run_network="${DOCKER_RUN_NETWORK:-host}"
kver="${KVER:-4.15.0-213-generic}"
kdir="${KDIR:-}"

docker_args=()
if [[ -n "$docker_host" ]]; then
    docker_args+=(-H "$docker_host")
fi

make_cmd="make -C module clean all KVER=${kver}"
if [[ -n "$kdir" ]]; then
    make_cmd="make -C module clean all KDIR=${kdir}"
fi

docker "${docker_args[@]}" run --rm \
    --network "$run_network" \
    -v "$(pwd):/workspace" \
    -w /workspace \
    "$image_tag" \
    bash -lc "$make_cmd"
