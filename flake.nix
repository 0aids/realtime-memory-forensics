{
  description = "A basic flake with a shell";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.systems.url = "github:nix-systems/default";
  inputs.flake-utils = {
    url = "github:numtide/flake-utils";
    inputs.systems.follows = "systems";
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = import nixpkgs {
          system = "x86_64-linux";
          config.allowUnfree = true;
          config.cudaSupport = true;
        };
        pythonEnv = pkgs.python313.withPackages (ps:
          with ps; [
              numpy
          ]);
      in {
        devShell = pkgs.mkShell {
          buildInputs = with pkgs; [
            gnumake
            ninja
            #clang_20
            cmake
            xxd
            clang-tools
            gdb
            pkg-config
            bear
            evtest
            rr
            sdl3
            SDL2
            sdl3-image
            sdl3-ttf
            libx11
            libxext
            libGL
            libGLU
            libei
            python313
            python313Packages.python-lsp-server
            vulkan-tools
            vulkan-headers
            gtest
          ];

          LD_LIBRARY_PATH = "${pkgs.lib.makeLibraryPath [
            pkgs.stdenv.cc.cc.lib
            pkgs.stdenv.cc.libc # For standard C library headers
            pkgs.zlib
            pkgs.glib.out
            pkgs.fontconfig
          ]}:$LD_LIBRARY_PATH";

          # UV_PYTHON = "${pythonEnv}/bin/python";
          # UV_PYTHON_PREFERENCE = "only-system";
          shellHook = ''
            export PYTHONWARNINGS="ignore"
          '';
        };
      }
    );
}
