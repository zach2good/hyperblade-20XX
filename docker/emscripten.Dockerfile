# The browser build: Emscripten on Ubuntu 24.04, whose glibc the build-time shader compiler (DXC)
# needs. It builds for wasm64, the project's 64-bit rule holding in the browser too, into /build.
#
#   docker build -t hyperblade-emscripten -f docker/emscripten.Dockerfile docker
#   docker run --rm -v "$PWD:/src:ro" -v "$PWD/.cpm-cache-linux:/cpm" -v "$PWD/build-wasm:/build" hyperblade-emscripten

FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates cmake ninja-build git python3 xz-utils bzip2 libatomic1 \
    && rm -rf /var/lib/apt/lists/*

ENV EMSDK_VERSION=4.0.8

RUN git clone --depth 1 https://github.com/emscripten-core/emsdk.git /emsdk \
    && /emsdk/emsdk install ${EMSDK_VERSION} \
    && /emsdk/emsdk activate ${EMSDK_VERSION}

ENV EMSDK=/emsdk PATH=/emsdk:/emsdk/upstream/emscripten:$PATH CPM_SOURCE_CACHE=/cpm

WORKDIR /src

CMD emcmake cmake -S /src -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS=-sMEMORY64=1 -DCMAKE_CXX_FLAGS=-sMEMORY64=1 -DCMAKE_EXE_LINKER_FLAGS=-sMEMORY64=1 \
    && cmake --build /build -- -k 0 \
    && ctest --test-dir /build -L "unit|lint" --output-on-failure
