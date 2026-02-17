# nRF54L15 DK - C++ Zephyr Application

C++ application for the Nordic nRF54L15 Development Kit, built with Zephyr RTOS (4.4.0-dev) and Nix.

## Prerequisites

- [Nix](https://nixos.org/) with flakes enabled
- nRF54L15 DK (for flashing/debugging)

## Quick Start

```bash
# Enter the development shell
nix develop

# Initialize the west workspace (first time only)
west init -l app
west update

# Build
west build -b nrf54l15dk/nrf54l15/cpuapp app

# Flash (with DK connected via USB)
west flash
```

## Regenerating west2nix.toml

After `west init` and `west update` succeed, regenerate the Nix lockfile for reproducible builds:

```bash
west2nix
```

Then `nix build` will work for fully sandboxed, reproducible firmware builds.

## Project Structure

```
.
├── flake.nix          # Nix flake (Zephyr 4.4.0-dev, SDK 0.17.4)
├── shell.nix          # Dev shell with toolchain, west, cmake, ninja
├── default.nix        # Reproducible Nix build (requires west2nix.toml)
├── west2nix.toml      # Nix lockfile for west modules (regenerate with west2nix)
├── app/
│   ├── CMakeLists.txt # CMake project definition
│   ├── prj.conf       # Zephyr Kconfig (C++17, logging, GPIO, console)
│   ├── west.yml       # West manifest (modules: cmsis, hal_nordic, mbedtls)
│   └── src/
│       └── main.cpp   # Application entry point
```
