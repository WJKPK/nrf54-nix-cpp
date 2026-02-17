{ mkShell
, zephyr
, west2nix
, cmake
, ninja
, dtc
, pkgs
}:
let
  west2nix-cli = west2nix.west2nix;
in
mkShell {
  packages = [
    (zephyr.sdk.override {
      targets = [
        "arm-zephyr-eabi"
      ];
    })
    zephyr.pythonEnv
    zephyr.hosttools-nix
    cmake
    ninja
    dtc
    west2nix-cli
    (pkgs.nrfutil.withExtensions [
      "nrfutil-device"
    ])
    pkgs.segger-jlink
  ];

  shellHook = ''
    echo "Zephyr dev shell for nRF54L15 DK (C++)"
    echo "  Board target: nrf54l15dk/nrf54l15/cpuapp"
    echo "Quick start:"
    echo "  west init -l app     # initialize west workspace"
    echo "  west update           # fetch all modules"
    echo "  west build -b nrf54l15dk/nrf54l15/cpuapp app"
    echo "  west flash"
    echo ""
    echo "To regenerate west2nix.toml:"
    echo "  west2nix"
  '';
}
