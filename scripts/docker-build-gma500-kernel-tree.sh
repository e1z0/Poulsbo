#!/usr/bin/env bash
set -euo pipefail

image_tag="${IMAGE_TAG:-poulsbo:bionic}"
docker_host="${DOCKER_HOST:-}"
run_network="${DOCKER_RUN_NETWORK:-host}"
build_dir="${BUILD_DIR:-/workspace/build/linux-ubuntu-4.15-lab}"
src_dir="${SRC_DIR:-/workspace/third_party/linux-ubuntu-4.15}"

docker_args=()
if [[ -n "$docker_host" ]]; then
    docker_args+=(-H "$docker_host")
fi

docker "${docker_args[@]}" run --rm \
    --network "$run_network" \
    -e BUILD_DIR="$build_dir" \
    -e SRC_DIR="$src_dir" \
    -v "$(pwd):/workspace" \
    -w /workspace \
    "$image_tag" \
    bash --noprofile --norc -lc '
set -euo pipefail

cd "$SRC_DIR"

# Ubuntu Launchpad trees expect this helper to be staged by Debian rules.
cp debian/scripts/retpoline-extract-one scripts/ubuntu-retpoline-extract-one
chmod +x scripts/ubuntu-retpoline-extract-one

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

make O="$BUILD_DIR" ARCH=x86_64 x86_64_defconfig
./scripts/config --file "$BUILD_DIR/.config" \
    -m DRM \
    -m DRM_GMA500 \
    -e DRM_GMA600 \
    -d DRM_GMA3600 \
    -d DRM_MEDFIELD
make O="$BUILD_DIR" ARCH=x86_64 olddefconfig modules_prepare
make O="$BUILD_DIR" ARCH=x86_64 M=drivers/gpu/drm/gma500 -j"$(nproc)" modules

file "$BUILD_DIR/drivers/gpu/drm/gma500/gma500_gfx.ko"
'
