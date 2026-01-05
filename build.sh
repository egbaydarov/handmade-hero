#!/usr/bin/env bash
set -e

# Default compile flags (can be overridden by CLI args)
if [[ $# -gt 0 ]]; then
  CFLAGS=("$@")
else
  CFLAGS=(-O0 -gcodeview)
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/build"
echo "$SCRIPT_DIR"

rm -rf -- "$BUILD_DIR"
mkdir -p -- "$BUILD_DIR"

(
  cd "$BUILD_DIR"
  echo "== compile (bear, verbose) =="

  bear --append -- \
    "$CC" ../src/main.c \
      "${CFLAGS[@]}" \
      -ftime-report \
      -fuse-ld=lld \
      -no-pie \
      -o main.exe \
      -luser32 \
      -lgdi32
)

