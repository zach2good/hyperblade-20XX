#!/bin/sh
# Plays simulated matches under Valgrind memcheck, in the image linux.Dockerfile builds, after a
# linux-gcc build has packaged the game into build-linux/package. Uninitialised reads show up as
# matches that differ from run to run, so run this when they do.
#
#   docker run --rm -v "$PWD/build-linux:/build:ro" -v "<your Data folder>:/data:ro" \
#       -v "<folder with your HYPERX.EXE>:/hyperx:ro" -v "$PWD/build-linux/memcheck:/out" \
#       hyperblade-linux sh /src/docker/memcheck.sh [HOME AWAY]
#
# Mount the source at /src too, or copy this script into /out and run it from there.

set -e

HOME_TEAM=${1:-LOSAN}
AWAY_TEAM=${2:-SEATTLE}

apt-get update -qq >/dev/null
DEBIAN_FRONTEND=noninteractive apt-get install -y -qq valgrind >/dev/null

mkdir -p /run/game
cp -P /build/package/hyperblade_20XX /build/package/libSDL3.so* /build/package/20XX_data.pak /run/game/
cp /hyperx/HYPERX.EXE /run/game/
ln -s /data /run/game/Data
cd /run/game

export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy LD_LIBRARY_PATH=/run/game

valgrind --track-origins=yes --error-limit=no --num-callers=24 \
    ./hyperblade_20XX -s -sim "$HOME_TEAM" "$AWAY_TEAM" 1 3 fixed /tmp/sim.csv > /out/memcheck.log 2>&1 || true

grep 'ERROR SUMMARY' /out/memcheck.log
