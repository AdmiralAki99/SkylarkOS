#!/bin/bash
set -e

cd "$(dirname "${BASH_SOURCE[0]}")/.."

# JETSON_HOST/JETSON_DIR live in scripts/jetson.env (gitignored, personal —
# not committed) so this script stays generic/shareable while the actual
# connection details never need to be tracked or synced across branches.
source scripts/jetson.env

# Mirror the repo layout the Dockerfile expects (ws/src, docker/, scripts/ all
# under one root) so `docker build` on the Jetson resolves its COPY paths the
# same way it does when built from the repo root locally.
ssh "$JETSON_HOST" "mkdir -p $JETSON_DIR/ws/src $JETSON_DIR/docker $JETSON_DIR/scripts"

rsync -avz --delete ws/src/    "$JETSON_HOST:$JETSON_DIR/ws/src/"
# --delete deliberately omitted here: docker/ort_gpu_files/ is gitignored and
# only ever exists on the Jetson (extracted from a known-good image, not
# tracked in this repo) — a --delete sync would wipe it since it's absent
# from the local source tree. --exclude belt-and-suspenders in case --delete
# is ever restored above.
rsync -avz --exclude 'ort_gpu_files/' docker/  "$JETSON_HOST:$JETSON_DIR/docker/"
rsync -avz --delete --exclude 'jetson.env' scripts/   "$JETSON_HOST:$JETSON_DIR/scripts/"

ssh "$JETSON_HOST" "cd $JETSON_DIR && \
  docker build -f docker/Dockerfile -t skylark . && \
  docker rm -f skylark 2>/dev/null; \
  bash scripts/run.sh"
