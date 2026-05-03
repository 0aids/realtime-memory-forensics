{
  description = "A basic flake with a shell (Clang + libc++)";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    systems.url = "github:nix-systems/default";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.systems.follows = "systems";
    };
  };

  outputs =
    { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
          config.cudaSupport = true;
          # overlays = [
          #   (final: prev: {
          #     root = prev.root.overrideAttrs (old: {
          #       src = pkgs.fetchFromGitHub {
          #         owner = "0aids";
          #         repo = "root";
          #         hash = "sha256-+ct2VggHEZlFHrm9gN3BKKf5WsUWW3SPjxIPZm5fd38=";
          #         rev = "b05a97247aad00946a45acd42c372a997d28cc22";
          #       };
          #       cmakeFlags = (old.cmakeFlags or [ ]) ++ [
          #         "-DCMAKE_CXX_STANDARD=23"
          #         "-DROOT_CXX_STANDARD=23"
          #       ];
          #       buildInputs = old.buildInputs ++ (with pkgs; [
          #           curl
          #           blas
          #       ]);
          #     });
          #   })
          # ];
        };

        llvm = pkgs.llvmPackages_20;
        pythonVer = pkgs.python313Packages;
        pythonEnv = with pythonVer; [
          numpy
        ];
      in
      {
        devShell = llvm.stdenv.mkDerivation {
          name = "llvm devshell";
          hardeningDisable = [ "all" ];
          nativeBuildInputs = with pkgs; [
            pythonEnv
            cmake
            ninja
            llvm.clang-tools
            # llvm.lldb
            gtest
            pre-commit
            ruff
            # root
          ];
        };
      }
    );
}
