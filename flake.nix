{
  description = "Flake utils demo";

  inputs.nixpkgs.url = "github:nixos/nixpkgs";
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        devShells.default = devShell;
        devShell = pkgs.mkShellNoCC {
          packages = [
            pkgs.pkgsCross.avr.buildPackages.gcc
            pkgs.gnumake
            pkgs.avrdude
            pkgs.picocom
            pkgs.bear
            pkgs.llvmPackages_22.clang-tools
            pkgs.usbutils
          ];
        };
      }
    );
}
