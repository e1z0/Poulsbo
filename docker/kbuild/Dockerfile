FROM i386/ubuntu:18.04
# Toolchain matching the Vaio (Ubuntu 18.04, gcc 7.5) so vermagic/modversions match.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential gcc make bc bison flex libssl-dev libelf-dev kmod && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /work
