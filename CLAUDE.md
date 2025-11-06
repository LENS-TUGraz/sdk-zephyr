# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Context

This is a **fork of Zephyr RTOS** from LENS-TUGraz (https://github.com/LENS-TUGraz/sdk-zephyr) with custom Bluetooth LE extensions for "grptlk" (Group Talk) functionality. The grptlk feature adds custom isochronous broadcast capabilities to the Bluetooth controller's Link Layer.

## Build System

Zephyr uses a **CMake-based build system** with **west** as the meta-tool. The architecture is **application-centric**: applications invoke Zephyr's build system rather than Zephyr controlling the build.

### Common Build Commands

```bash
# Build an application for a specific board
west build -b <BOARD> <app_path>

# Example: Build a sample for nRF52840
west build -b nrf52840dk_nrf52840 samples/bluetooth/beacon

# Clean build (force CMake rerun)
west build -t pristine

# Build with sysbuild (multi-image: bootloader + app)
west build --sysbuild -b <BOARD> <app_path>

# Flash to hardware
west flash

# Debug with GDB
west debug

# List available boards
west boards
```

### Testing with Twister

Twister is Zephyr's test runner that discovers tests via `testcase.yaml` files:

```bash
# Run all tests (builds/runs on configured platforms)
./scripts/twister

# Run tests for specific platform
./scripts/twister -p nrf52840dk_nrf52840

# Build-only (no execution)
./scripts/twister --build-only

# Filter by tags
./scripts/twister -T subsys/bluetooth/controller

# Verbose output
./scripts/twister -v

# Enable slow tests
./scripts/twister --enable-slow
```

### Configuration System

Zephyr uses **Kconfig** for build-time configuration and **Device Tree** for hardware description:

- **Kconfig**: Hierarchical configuration (`prj.conf`, `Kconfig.zephyr`)
  - Application configs: `prj.conf`, board-specific: `boards/<board>.conf`
  - Override via: `west build -- -DCONFIG_FOO=y`

- **Device Tree**: Hardware description (`.dts`, `.overlay`)
  - Application overlays: `app.overlay`, board-specific: `boards/<board>.overlay`
  - Compiled to: `build/zephyr/include/generated/devicetree.h`

## Architecture Overview

### Layered Design

```
Application Layer (samples/, tests/, external apps)
         ↓
Subsystems Layer (subsys/: bluetooth, networking, logging, shell, etc.)
         ↓
Drivers Layer (drivers/: gpio, uart, spi, i2c, sensor, etc.)
         ↓
Kernel Layer (kernel/: scheduler, threads, sync, memory)
         ↓
Architecture Layer (arch/: arm, arm64, risc-v, x86, xtensa)
         ↓
Hardware Abstraction (boards/, soc/, dts/)
```

### Key Components Interaction

1. **Boot Flow**: arch-specific startup → kernel init → device init → app main
2. **Device Framework**: All drivers register via `DEVICE_DEFINE()` or `DEVICE_DT_DEFINE()`
   - Devices are `struct device` with function pointer APIs
   - Dependencies auto-resolved during build (multi-stage linking)
3. **Interrupt Handling**: Arch-specific ISR tables generated at build time
4. **Memory Management**: Kernel heaps, memory slabs, MMU support, userspace isolation
5. **Scheduling**: Priority-based, cooperative/preemptive, SMP support

### Multi-Stage Linking

Zephyr uses 1-3 linking stages to handle circular dependencies:
- **zephyr_pre0**: Initial link to extract device dependencies
- **zephyr_pre1**: Re-link with ISR tables, kernel object hashes
- **zephyr_final**: Final binary with all generated tables

This enables:
- Device dependency ordering (`CONFIG_DEVICE_DEPS`)
- ISR table generation (`CONFIG_GEN_ISR_TABLES`)
- Kernel object hashes (`CONFIG_USERSPACE`)
- Memory domain generation

## Key Directories

| Directory | Purpose |
|-----------|---------|
| `kernel/` | Core RTOS: scheduler, threads, synchronization, memory management |
| `arch/` | Architecture-specific code: ARM, ARM64, x86, RISC-V, Xtensa, ARC |
| `drivers/` | Hardware drivers following the device framework |
| `subsys/` | Feature subsystems: Bluetooth, networking, USB, logging, shell, filesystems |
| `subsys/bluetooth/controller/` | Bluetooth LE controller (Link Layer, HCI) |
| `subsys/bluetooth/controller/ll_sw/` | Software Link Layer implementation |
| `boards/` | Board definitions: DTS, configs, documentation |
| `soc/` | SoC-specific initialization and configuration |
| `dts/` | Device Tree source files and bindings |
| `include/zephyr/` | Public API headers |
| `cmake/` | Build system: CMake modules, toolchain files, linker scripts |
| `lib/` | Standard libraries: C lib, POSIX, data structures |
| `samples/` | Example applications (all buildable) |
| `tests/` | Test suites (discovered by twister) |
| `scripts/` | Development tools: twister, west commands, CI tools |

## Bluetooth Controller Architecture

This repository contains **custom modifications to the Bluetooth LE controller** for grptlk (Group Talk) functionality.

### Bluetooth Stack Layers

```
Host Layer (subsys/bluetooth/host/: HCI, L2CAP, ATT, GATT, SMP)
         ↓
HCI Interface (subsys/bluetooth/controller/hci/)
         ↓
Link Layer (subsys/bluetooth/controller/ll_sw/)
  - Upper Link Layer (ULL): ull_*.c
  - Lower Link Layer (LLL): lll_*.c (platform-specific)
         ↓
Radio HAL (subsys/bluetooth/controller/ll_sw/nordic/hal/)
```

### Grptlk Implementation

The **grptlk** feature is a custom isochronous broadcast extension:

**Key Files:**
- `subsys/bluetooth/controller/ll_sw/ull_adv_grptlk.c` - ULL broadcaster
- `subsys/bluetooth/controller/ll_sw/ull_sync_grptlk.c` - ULL receiver
- `subsys/bluetooth/controller/ll_sw/lll_adv_grptlk.h` - LLL broadcaster header
- `subsys/bluetooth/controller/ll_sw/lll_sync_grptlk.h` - LLL receiver header
- `subsys/bluetooth/controller/ll_sw/nordic/lll/lll_adv_grptlk.c` - Nordic LLL broadcaster
- `subsys/bluetooth/controller/ll_sw/nordic/lll/lll_sync_grptlk.c` - Nordic LLL receiver

**Recent Development (see git log):**
- ISOAL (ISO Adaptation Layer) has been **skipped** for this implementation
- Direct LLL-to-ULL data path for lower latency
- Focus on Nordic radio HAL (nRF5x series)
- Custom PDU handling for group broadcast scenarios

### Bluetooth Controller Development

When modifying the Bluetooth controller:

1. **ULL (Upper Link Layer)**: High-level state machines, event scheduling
   - Located: `subsys/bluetooth/controller/ll_sw/ull_*.c`
   - Handles: Connection setup, advertising, scanning, ISO streams

2. **LLL (Lower Link Layer)**: Real-time radio operations
   - Located: `subsys/bluetooth/controller/ll_sw/nordic/lll/lll_*.c`
   - Platform-specific (Nordic nRF5x in this fork)
   - Handles: Radio timing, PHY, packet assembly

3. **HAL (Hardware Abstraction Layer)**: Direct hardware access
   - Located: `subsys/bluetooth/controller/ll_sw/nordic/hal/`
   - Radio, timer, CCM (crypto), RNG, etc.

4. **Scheduler**: Event-based scheduling for radio operations
   - Located: `subsys/bluetooth/controller/ll_sw/ull_sched.c`
   - Manages timing conflicts between connections, advertising, scanning

## CMake Build Structure

Key CMake concepts:

- **zephyr_interface**: Source-less library with global compiler flags
  - All other libraries link against this

- **zephyr**: Main catch-all library for most source files

- **kernel**: Separate library for kernel code (can be pre-built)

- **Conditional inclusion**: `add_subdirectory_ifdef(CONFIG_FOO dir)` pattern
  - Drivers/subsystems only included if Kconfig enables them

- **Application CMakeLists.txt** pattern:
  ```cmake
  cmake_minimum_required(VERSION 3.20.0)
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(my_app)

  target_sources(app PRIVATE src/main.c)
  ```

## Development Workflow

### Typical Development Cycle

```bash
# 1. Setup workspace (first time)
west init -m https://github.com/LENS-TUGraz/sdk-zephyr --mr try1 ~/zephyr-grptlk
cd ~/zephyr-grptlk
west update

# 2. Create/modify application
cd zephyr/samples/bluetooth/my_app
# Edit prj.conf, src/main.c, app.overlay

# 3. Build
west build -b nrf52840dk_nrf52840

# 4. Flash and test
west flash
# Monitor output via UART

# 5. Debug if needed
west debug
```

### Working with the Bluetooth Controller

For controller development (grptlk or other LL modifications):

1. **Enable controller build**: `CONFIG_BT_LL_SW_SPLIT=y` in `prj.conf`
2. **Modify ULL/LLL** files in `subsys/bluetooth/controller/ll_sw/`
3. **Test on hardware** (nRF52840 DK recommended for grptlk)
4. **Use twister** for regression: `./scripts/twister -T tests/bluetooth/controller`

### Nordic-Specific Notes

This fork targets **Nordic nRF5x** series (nRF52, nRF53):
- Primary test platform: **nRF52840 DK** (`nrf52840dk_nrf52840`)
- Radio HAL: Nordic-specific in `subsys/bluetooth/controller/ll_sw/nordic/`
- Consider nRF53 dual-core: network core runs controller, app core runs host

## Important Development Practices

- **Device Framework**: Use `DEVICE_DEFINE()` or `DEVICE_DT_DEFINE()` for all drivers
- **Kconfig**: All features must be Kconfig-gated (no hardcoded features)
- **Device Tree**: Hardware description belongs in DTS, not C code
- **API Stability**: Public APIs in `include/zephyr/` must maintain backward compatibility
- **Coding Style**: Follow Linux kernel style (enforced by `checkpatch.pl`)
- **Testing**: Add `testcase.yaml` for new features

## Reference Documentation

- Official docs: https://docs.zephyrproject.org
- Bluetooth controller: https://docs.zephyrproject.org/latest/connectivity/bluetooth/api/index.html
- Building apps: https://docs.zephyrproject.org/latest/develop/application/index.html
- Device Tree: https://docs.zephyrproject.org/latest/build/dts/index.html
- Twister: https://docs.zephyrproject.org/latest/develop/test/twister.html
