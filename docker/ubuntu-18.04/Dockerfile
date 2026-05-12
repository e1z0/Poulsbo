FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    bc \
    bison \
    build-essential \
    ca-certificates \
    cpio \
    file \
    flex \
    git \
    kmod \
    libdrm-dev \
    libegl1-mesa-dev \
    libelf-dev \
    libgles2-mesa-dev \
    libssl-dev \
    libx11-dev \
    libxdamage-dev \
    libxext-dev \
    libxfixes-dev \
    libxrender-dev \
    libxshmfence-dev \
    libxcb-dri2-0-dev \
    libxcb-dri3-dev \
    libxcb-present-dev \
    libxcb1-dev \
    linux-headers-generic \
    meson \
    ninja-build \
    pkg-config \
    python3 \
    python3-jinja2 \
    python3-mako \
    python3-setuptools \
    rsync \
    xorg-dev \
    xserver-xorg-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
