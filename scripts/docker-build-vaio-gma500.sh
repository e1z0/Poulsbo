#!/usr/bin/env bash
set -euo pipefail

image_tag="${IMAGE_TAG:-poulsbo:bionic-i386}"
docker_host="${DOCKER_HOST:-}"
run_network="${DOCKER_RUN_NETWORK:-host}"

headers_dir="${HEADERS_DIR:-/workspace/third_party/devnull-vaio-headers/linux-headers-5.4.0-150-generic}"
module_src_in="${MODULE_SRC_IN:-/workspace/third_party/linux-ubuntu-5.4/drivers/gpu/drm/gma500}"
module_src_out="${MODULE_SRC_OUT:-/workspace/build/devnull-vaio-gma500-src}"

docker_args=()
if [[ -n "$docker_host" ]]; then
    docker_args+=(-H "$docker_host")
fi

docker "${docker_args[@]}" run --rm \
    --network "$run_network" \
    -e HEADERS_DIR="$headers_dir" \
    -e MODULE_SRC_IN="$module_src_in" \
    -e MODULE_SRC_OUT="$module_src_out" \
    -v "$(pwd):/workspace" \
    -w /workspace \
    "$image_tag" \
    bash --noprofile --norc -lc '
set -euo pipefail

rm -rf "$MODULE_SRC_OUT"
mkdir -p "$MODULE_SRC_OUT"
rsync -a --delete "$MODULE_SRC_IN"/ "$MODULE_SRC_OUT"/

make -C "$HEADERS_DIR" M="$MODULE_SRC_OUT" ARCH=x86 modules

file "$MODULE_SRC_OUT/gma500_gfx.ko"
'
