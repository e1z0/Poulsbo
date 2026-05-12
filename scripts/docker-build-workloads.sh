#!/usr/bin/env bash
set -euo pipefail

image_tag="${IMAGE_TAG:-poulsbo:bionic}"
docker_host="${DOCKER_HOST:-}"
run_network="${DOCKER_RUN_NETWORK:-host}"

docker_args=()
if [[ -n "$docker_host" ]]; then
    docker_args+=(-H "$docker_host")
fi

docker "${docker_args[@]}" run --rm \
    --network "$run_network" \
    ${CC:+-e CC="$CC"} \
    ${CFLAGS:+-e CFLAGS="$CFLAGS"} \
    ${LDFLAGS:+-e LDFLAGS="$LDFLAGS"} \
    ${PKG_CONFIG:+-e PKG_CONFIG="$PKG_CONFIG"} \
    -v "$(pwd):/workspace" \
    -w /workspace \
    "$image_tag" \
    bash -lc "make -C workloads clean all"
