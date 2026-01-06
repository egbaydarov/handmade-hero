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

        wine = pkgs.wineWowPackages.waylandFull;
      in
      {
        devShells.default = pkgs.mkShell {
          packages = [
            pkgs.pkg-config
            pkgs.bear
            pkgs.clang-tools
            pkgs.llvmPackages.llvm
            pkgs.llvmPackages.lld

            # controller shit
            wine

            cross.stdenv.cc
            cross.llvmPackages.clang
            cross.llvmPackages.lld
          ];

          shellHook = ''
            export TARGET=x86_64-w64-mingw32

            export CC=$TARGET-gcc
            export CLANG=$TARGET-clang

            export WINEDEBUG=-all
            export WINEPREFIX="$PWD/.wine"

            WININC="$(ls -d /nix/store/*-mingw-w64-x86_64-w64-mingw32-*/include | head -n2)"

            # drop "-rpath <path>" pairs (avoid warning)
            NIX_LDFLAGS="$(printf "%s\n" "$NIX_LDFLAGS" | sed -E 's/-rpath[[:space:]]+[^[:space:]]+//g')"
            export NIX_LDFLAGS

            cat > .clangd <<EOF
            CompileFlags:
              Add: [
                "-isystem", "$WININC",
              ]
            EOF

            echo "Wine: $(command -v wine)"
            echo "MinGW target: $TARGET"
          '';
        };
      });
}

