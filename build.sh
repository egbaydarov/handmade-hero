set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/build"

rm -rf -- "$BUILD_DIR"
mkdir -p -- "$BUILD_DIR"

(
  cd "$BUILD_DIR"

  echo "== compile (bear, verbose) =="
  bear --append -- \
    "$CXX" ../src/main.cpp \
      -ftime-report \
      -g3 -gcodeview \
      -o main.exe \
      -luser32 -lgdi32
)

