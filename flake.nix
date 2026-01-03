{
  description = "mingw-w64 cross compile + wine dev shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        cross = pkgs.pkgsCross.mingwW64;
        wine = pkgs.wineWowPackages.stable;
      in
      {
        devShells.default = pkgs.mkShell {
          packages = [
            pkgs.cmake
            pkgs.ninja
            pkgs.gnumake
            pkgs.pkg-config
            pkgs.bear
            pkgs.clang-tools
            wine

            # MinGW-w64 cross toolchain (includes sysroot headers/libs)
            cross.stdenv.cc
          ];

          shellHook = ''
            export TARGET=x86_64-w64-mingw32

            export CC=$TARGET-gcc
            export CXX=$TARGET-g++
            export AR=$TARGET-ar
            export RANLIB=$TARGET-ranlib
            export STRIP=$TARGET-strip
            export WINDRES=$TARGET-windres

            export WINEDEBUG=-all
            export WINEPREFIX="$PWD/.wine"

            GPP="$(command -v x86_64-w64-mingw32-g++)"
            SYSROOT="$(dirname "$(dirname "$(readlink -f "$GPP")")")/x86_64-w64-mingw32"
            WININC="$(ls -d /nix/store/*-mingw-w64-x86_64-w64-mingw32-*/include | head -n1)"

            cat > .clangd <<EOF
            CompileFlags:
              Add: [
                "--target=x86_64-w64-windows-gnu",
                "--sysroot=$SYSROOT",
                "-nostdinc",
                "-nostdinc++",
                "-isystem", "$WININC",
                "-isystem", "$SYSROOT/include",
                "-isystem", "$SYSROOT/usr/include"
              ]
            EOF

            echo "MinGW target: $TARGET"
          '';
        };
      });
}

