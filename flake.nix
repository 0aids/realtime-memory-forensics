{
  description = "A basic flake with a shell";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.systems.url = "github:nix-systems/default";
  inputs.flake-utils = {
    url = "github:numtide/flake-utils";
    inputs.systems.follows = "systems";
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
          config.cudaSupport = true;
        };

        # Define the exact LLVM version we want to use
        llvm = pkgs.llvmPackages_22;

        pythonEnv = pkgs.python313.withPackages (
          ps: with ps; [
            numpy
          ]
        );
      in
      {
        devShell = pkgs.mkShell.override { stdenv = llvm.stdenv; } {
          packages = with pkgs; [
            gnumake
            ninja
            cmake
            gdb
            bear
            rr
            llvm.clang-tools
            pythonEnv
            pre-commit
            ruff

            # 1. Add the core LLVM package to get llvm-symbolizer
            llvm.llvm
            llvm.lldb

            # 2. Wrap gtest in enableDebugging to prevent Nix from stripping symbols
            (pkgs.enableDebugging gtest)
          ];

          LD_LIBRARY_PATH = "${pkgs.lib.makeLibraryPath [
            llvm.libcxx
            # pkgs.stdenv.cc.cc.lib
            # pkgs.stdenv.cc.libc
            # pkgs.zlib
            # pkgs.glib.out
            # pkgs.fontconfig
          ]}";

          shellHook = ''
            export PYTHONWARNINGS="ignore"
            export CXX=clang++
            export CC=clang

            # 3. Explicitly tell AddressSanitizer where the symbolizer is
            export ASAN_SYMBOLIZER_PATH="${llvm.llvm}/bin/llvm-symbolizer"

            # 4. Ensure ASan is configured to actually use it
            export ASAN_OPTIONS="symbolize=1"
          '';
        };
      }
    );
}
