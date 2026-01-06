#!/usr/bin/env bash
set -e

# Default compile flags (can be overridden by CLI args)
if [[ $# -gt 0 ]]; then
  CFLAGS=("$@")
else
  CFLAGS=(-O0 -g)
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BUILD_DIR="$SCRIPT_DIR/build"
echo "$SCRIPT_DIR"

rm -rf -- "$BUILD_DIR"
mkdir -p -- "$BUILD_DIR"

(
  cd "$BUILD_DIR"
  echo "== compile linux (bear, verbose) =="

  # Generate wayland protocol headers if wayland-scanner is available
  PROTOCOL_SRC=""
  if command -v wayland-scanner >/dev/null 2>&1; then
    # Try to find xdg-shell.xml in common locations
    XDG_SHELL_PROTOCOL=""
    if [ -n "$XDG_SHELL_PROTOCOL" ] && [ -f "$XDG_SHELL_PROTOCOL" ]; then
      : # Use provided path
    elif pkg-config --exists wayland-protocols 2>/dev/null; then
      XDG_SHELL_PROTOCOL=$(pkg-config --variable=pkgdatadir wayland-protocols)/stable/xdg-shell/xdg-shell.xml
    else
      # Search in Nix store and common locations
      XDG_SHELL_PROTOCOL=$(find /nix/store -path "*/wayland-protocols*/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml" 2>/dev/null | head -1)
      if [ -z "$XDG_SHELL_PROTOCOL" ]; then
        XDG_SHELL_PROTOCOL=$(find /usr/share /usr/local/share -name "xdg-shell.xml" 2>/dev/null | head -1)
      fi
    fi
    
    if [ -n "$XDG_SHELL_PROTOCOL" ] && [ -f "$XDG_SHELL_PROTOCOL" ]; then
      echo "== generating wayland protocol headers from $XDG_SHELL_PROTOCOL =="
      wayland-scanner client-header "$XDG_SHELL_PROTOCOL" xdg-shell-client-protocol.h
      wayland-scanner private-code "$XDG_SHELL_PROTOCOL" xdg-shell-protocol.c
      PROTOCOL_SRC="xdg-shell-protocol.c"
    else
      echo "[WARNING] xdg-shell.xml not found, protocol headers not generated"
    fi
  fi

  # Use pkg-config to get proper flags for Wayland and ALSA
  WAYLAND_CFLAGS=$(pkg-config --cflags wayland-client 2>/dev/null || echo "")
  WAYLAND_LIBS=$(pkg-config --libs wayland-client 2>/dev/null || echo "-lwayland-client")
  ALSA_CFLAGS=$(pkg-config --cflags alsa 2>/dev/null || echo "")
  ALSA_LIBS=$(pkg-config --libs alsa 2>/dev/null || echo "-lasound")

  # Add generated protocol header directory if it exists
  if [ -f xdg-shell-client-protocol.h ]; then
    WAYLAND_CFLAGS="$WAYLAND_CFLAGS -I."
  fi

  # Find library paths for RPATH (NixOS)
  WAYLAND_LIBDIR=$(pkg-config --variable=libdir wayland-client 2>/dev/null || echo "")
  ALSA_LIBDIR=$(pkg-config --variable=libdir alsa 2>/dev/null || echo "")
  
  RPATH_FLAGS=""
  if [ -n "$WAYLAND_LIBDIR" ]; then
    RPATH_FLAGS="$RPATH_FLAGS -Wl,-rpath,$WAYLAND_LIBDIR"
  fi
  if [ -n "$ALSA_LIBDIR" ] && [ "$ALSA_LIBDIR" != "$WAYLAND_LIBDIR" ]; then
    RPATH_FLAGS="$RPATH_FLAGS -Wl,-rpath,$ALSA_LIBDIR"
  fi
  
  # If pkg-config didn't work, try to find libraries in Nix store
  if [ -z "$WAYLAND_LIBDIR" ]; then
    WAYLAND_LIBDIR=$(find /nix/store -name "libwayland-client.so*" 2>/dev/null | head -1 | xargs dirname 2>/dev/null || echo "")
    if [ -n "$WAYLAND_LIBDIR" ]; then
      RPATH_FLAGS="$RPATH_FLAGS -Wl,-rpath,$WAYLAND_LIBDIR"
    fi
  fi
  if [ -z "$ALSA_LIBDIR" ]; then
    ALSA_LIBDIR=$(find /nix/store -name "libasound.so*" 2>/dev/null | head -1 | xargs dirname 2>/dev/null || echo "")
    if [ -n "$ALSA_LIBDIR" ] && [ "$ALSA_LIBDIR" != "$WAYLAND_LIBDIR" ]; then
      RPATH_FLAGS="$RPATH_FLAGS -Wl,-rpath,$ALSA_LIBDIR"
    fi
  fi

  bear --append -- \
    ${CC_LINUX:-${CC:-clang}} ../src/linux_handmade.c ${PROTOCOL_SRC} \
      "${CFLAGS[@]}" \
      ${WAYLAND_CFLAGS} \
      ${ALSA_CFLAGS} \
      -ftime-report \
      -fuse-ld=lld \
      ${RPATH_FLAGS} \
      -o linux_handmade \
      ${WAYLAND_LIBS} \
      ${ALSA_LIBS} \
      -lm \
      -lpthread \
      -ldl
)

