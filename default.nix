{ stdenv
, zephyr
, callPackage
, cmake
, ninja
, west2nix
, gitMinimal
, dtc
, lib
}:

let
  west2nixHook = west2nix.mkWest2nixHook {
    manifest = ./west2nix.toml;
  };

in
stdenv.mkDerivation {
  name = "nrf54l15-cpp-app";

  nativeBuildInputs = [
    (zephyr.sdk.override {
      targets = [
        "arm-zephyr-eabi"
      ];
    })
    west2nixHook
    zephyr.pythonEnv
    zephyr.hosttools-nix
    gitMinimal
    cmake
    ninja
    dtc
  ];

  # west drives cmake, not nix
  dontUseCmakeConfigure = true;

  src = ./.;

  westBuildFlags = [
    "-b"
    "nrf54l15dk/nrf54l15/cpuapp"
  ];

  installPhase = ''
    mkdir $out
    cp ./build/zephyr/zephyr.elf $out/
    cp ./build/zephyr/zephyr.hex $out/ || true
    cp ./build/zephyr/zephyr.bin $out/ || true
  '';
}
