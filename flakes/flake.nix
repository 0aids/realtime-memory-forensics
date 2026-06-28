{
  description = "";
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
        };

        pythonVer = pkgs.python313Packages;
        pythonEnv = with pythonVer; [
          numpy
          nanobind
        ];
        llvm = pkgs.llvmPackages_22;
      in
      {
        devShell = llvm.stdenv.mkDerivation {
          name = "rmf shell";
          hardeningDisable = [ "all" ];
          nativeBuildInputs = with pkgs; [
            pythonEnv
            cmake
            ninja
            llvm.clang-tools
            llvm.bintools
            gtest
            pre-commit
            ruff
            gdb
          ];
        };
      }
    );
}
