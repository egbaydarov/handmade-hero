set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/build"
echo $SCRIPT_DIR

rm -rf -- "$BUILD_DIR"
mkdir -p -- "$BUILD_DIR"

(
  cd "$BUILD_DIR"
  echo "== compile (bear, verbose) =="
  #bear --append -- \
  #  "$CXX" ../src/main.cpp \
  #    -ftime-report \
  #    "-ffile-prefix-map=$SCRIPT_DIR=Z:\home\stuff\cstuff" \
  #    "-fdebug-prefix-map=$SCRIPT_DIR=Z:\home\stuff\cstuff" \
  #    -g3 -gcodeview \
  #    -o main.exe \
  #    -luser32 -lgdi32
  #3 -gdwarf64 -gstrict-dwarf -ginline-points -gstatement-frontiers -gvariable-location-views \
  bear --append -- \
    x86_64-w64-mingw32-gcc ../src/main.cpp \
      -g3 -gcodeview \
      -ftime-report \
      -O0 \
      -o main.exe \
      -luser32 \
      -lgdi32
)

