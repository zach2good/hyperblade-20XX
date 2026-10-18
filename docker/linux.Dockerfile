# A clean Linux toolchain that builds, tests and packages the game without Windows. PRESET picks
# the compiler, linux-gcc or linux-clang, and the build lands in /build with its package in
# /build/packages.
#
#   docker build -t hyperblade-linux -f docker/linux.Dockerfile docker
#   docker run --rm -e PRESET=linux-gcc -v "$PWD:/src:ro" -v "$PWD/.cpm-cache-linux:/cpm" -v "$PWD/build-linux:/build" hyperblade-linux

FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates cmake ninja-build g++-14 clang-20 git pkg-config \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev \
        libwayland-dev libxkbcommon-dev wayland-protocols libegl-dev libgl-dev libdrm-dev libgbm-dev \
        libasound2-dev libpulse-dev libpipewire-0.3-dev libdecor-0-dev libudev-dev libdbus-1-dev \
    && rm -rf /var/lib/apt/lists/*

ENV PRESET=linux-gcc CPM_SOURCE_CACHE=/cpm

WORKDIR /src

# The install is only there for ldd to prove the image finds libSDL3 beside it.
CMD cmake --preset "$PRESET" -B /build \
    && cmake --build /build -- -k 0 \
    && ctest --test-dir /build --output-on-failure \
    && rm -rf /build/package \
    && cmake --install /build --prefix /build/package \
    && ! ldd /build/package/hyperblade_20XX | grep 'not found' \
    && cmake --build /build --target package
