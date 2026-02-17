{
  description = "C++ Zephyr application for nRF54L15 DK";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    zephyr.url = "github:zephyrproject-rtos/zephyr/main";
    zephyr.flake = false;

    zephyr-nix.url = "github:nix-community/zephyr-nix";
    zephyr-nix.inputs.nixpkgs.follows = "nixpkgs";
    zephyr-nix.inputs.zephyr.follows = "zephyr";

    west2nix.url = "github:adisbladis/west2nix";
    west2nix.inputs.nixpkgs.follows = "nixpkgs";
    west2nix.inputs.zephyr-nix.follows = "zephyr-nix";
  };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      mkPkgs = system: import nixpkgs {
        inherit system;
        config = {
          allowUnfree = true;
          segger-jlink.acceptLicense = true;
          permittedInsecurePackages = [
            "segger-jlink-qt4-874"
          ];
        };
      };

    in
    {
      packages = forAllSystems (system:
        let
          pkgs = mkPkgs system;

          callPackage = pkgs.newScope (pkgs // {
            zephyr = inputs.zephyr-nix.packages.${system};
            west2nix =
              pkgs.callPackage inputs.west2nix.lib.mkWest2nix { };
          });
        in
        {
          default = callPackage ./default.nix { };
        }
      );

      devShells = forAllSystems (system:
        let
          pkgs = mkPkgs system;

          callPackage = pkgs.newScope (pkgs // {
            zephyr = inputs.zephyr-nix.packages.${system};
            west2nix =
              pkgs.callPackage inputs.west2nix.lib.mkWest2nix { };
          });
        in
        {
          default = callPackage ./shell.nix { };
        }
      );
    };
}
