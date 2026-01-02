{
  description = "mingw-w64 cross compile + wine dev shell (fixed)";

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

            echo "MinGW target: $TARGET"
            echo "Build: $CXX -O2 -o app.exe main.cpp"
            echo "Run:   wine ./app.exe"
          '';
        };
      });
}

