# A clean Linux toolchain that checks the game builds and its tests pass without Windows. GCC is
# the default; pass -e CC=clang-20 -e CXX=clang++-20 for Clang, with a build folder of its own.
#
#   docker build -t hyperblade-linux -f docker/linux.Dockerfile docker
#   docker run --rm -v "$PWD:/src:ro" -v "$PWD/.cpm-cache-linux:/cpm" -v "$PWD/build-linux:/build" hyperblade-linux

FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates cmake ninja-build g++-14 clang-20 git pkg-config \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev \
        libwayland-dev libxkbcommon-dev wayland-protocols libegl-dev libgl-dev libdrm-dev libgbm-dev \
        libasound2-dev libpulse-dev libpipewire-0.3-dev libdecor-0-dev libudev-dev libdbus-1-dev \
    && rm -rf /var/lib/apt/lists/*

ENV CC=gcc-14 CXX=g++-14 CPM_SOURCE_CACHE=/cpm

CMD cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && cmake --build /build -- -k 0 \
    && ctest --test-dir /build --output-on-failure \
    && rm -rf /build/package \
    && cmake --install /build --prefix /build/package \
    && ! ldd /build/package/hyperblade_20XX | grep 'not found'
