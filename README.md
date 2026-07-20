# ygpu

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg)](https://isocpp.org/)
[![Vulkan](https://img.shields.io/badge/API-Vulkan-AC162C.svg)](https://www.vulkan.org/)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
![Status](https://img.shields.io/badge/status-in%20development-orange.svg)

**ygpu** is a modern C++20 GPU library that hides Vulkan boilerplate and provides a more convenient way to work with the GPU without taking native Vulkan access away from the user.

The **y** stands for **Yellowstone**—a geological counterpart to the name Vulkan.

> [!IMPORTANT]
> ygpu is under active development. It is already usable for experiments and small projects, but the public API is not yet stable and may change before the first stable release.

## Goals

Vulkan is explicit, powerful, and predictable, but even simple tasks require a significant amount of setup and resource-management code. ygpu aims to make common GPU operations shorter and easier to reason about while preserving the control expected from a Vulkan-based library.

The project focuses on:

- reducing repetitive Vulkan initialization and resource-management code;
- providing modern C++ configuration objects, presets, builders, and RAII wrappers;
- keeping native Vulkan handles available for advanced use cases;
- supporting graphics and compute workloads through the same modular API;
- avoiding a large external dependency tree.

ygpu is an abstraction layer, not a replacement graphics API and not a complete game engine.

## Features

- Vulkan instance, physical-device, and logical-device creation
- Configurable GPU selection and debug/validation presets
- GLFW windows, Vulkan surfaces, and swapchains
- Swapchain recreation on resize or out-of-date presentation
- Vertex, index, uniform, storage, and staging buffers
- Buffer upload helpers and mapped memory access
- Images, image views, samplers, and 2D textures
- Optional mipmap generation
- SPIR-V loading and GLSL compilation through `glslc`
- WebGPU-style bind groups and bind-group layouts
- Automatic descriptor-pool sizing from layouts
- Pipeline layouts and push-constant ranges
- Graphics and compute pipelines
- Command recording, copies, barriers, drawing, and compute dispatch
- One-time command submission helpers
- Move-only RAII wrappers for owned Vulkan resources
- Structured error values instead of hidden exceptions
- Graphics, compute, and device examples
- Unit and Vulkan integration tests

## Design

### Progressive disclosure

Most objects can be configured at several levels:

1. use a convenience overload or default preset;
2. start from a preset and override selected fields;
3. build a full configuration manually;
4. access the underlying Vulkan handle when lower-level control is required.

```cpp
auto config = yst::core::CreateConfig(yst::gpuc::DEFAULT_CONFIG);
config.PreferIntegratedGPU = false;
config.EnableDebug = true;

auto [device, error] = yst::core::CreateDevice(config);
```

### Fluent builders

More complex objects use builders to keep configuration readable.

```cpp
auto layoutConfig =
    yst::core::BindGroupLayoutBuilder(
        yst::core::BindGroupLayoutPreset::Empty)
        .AddUniformBuffer(
            0,
            yst::core::ShaderStageBits::Vertex)
        .AddCombinedTextureSampler(
            1,
            yst::core::ShaderStageBits::Fragment)
        .Build();
```

### RAII with native access

Owned resources such as buffers, images, samplers, shader modules, descriptor pools, and pipelines clean up their Vulkan objects automatically. They are move-only where ownership must remain unique.

The native handles remain accessible, so ygpu can be mixed with direct Vulkan calls when the abstraction does not cover a specific use case.

### Explicit errors

Creation functions generally return a resource and a `CustomError`:

```cpp
auto [device, error] = yst::core::CreateDevice(config);

if (error) {
    std::cerr << error.str() << '\n';
    return 1;
}
```

A specific error category can be checked with `error.Is(...)`.

> The current public namespace is `yst::`. The repository and exported Axle module are named `ygpu`.

## Requirements

### Required

- a C++20-capable toolchain;
- Axle build system;
- Vulkan headers and a Vulkan loader/implementation for the target platform;
- a Vulkan-capable GPU and driver for GPU integration tests and examples.

### Build and development tools

- `clang++` for the default Linux target;
- [Zig](https://ziglang.org/) for the current Windows and macOS cross-compilation targets;
- `glslc` for compiling the example shaders;
- Bash and common Unix tools used by the scripts: `cp`, `find`, `tar`, and `sha256sum`;
- `wget` and an `xz`-capable `tar` for installing the macOS SDK on a Linux build host;
- Valgrind when using the `valgrind` target directly.

The library itself intentionally avoids a large third-party package dependency tree. Apart from Vulkan and the platform libraries required by the selected target, no external C++ dependency manager is required.

## Supported build targets

| Target | Current compiler configuration | Example window output |
|---|---|---|
| Linux | `clang++` | `.modules/example_window/bin/main-linux` |
| Windows x86_64 | `zig c++ -target x86_64-windows` | `.modules/example_window/bin/main-win.exe` |
| macOS arm64 | `zig c++ -target aarch64-macos` | `.modules/example_window/bin/main-macos` |

The repository scripts are currently written for a Unix-like build host. Windows and macOS builds are cross-compiled with Zig.

The macOS target is designed to be built from Linux and does not require Xcode. It uses a local macOS 13.3 SDK at:

```text
.dev/MacOSX13.3.sdk
```

The SDK path is referenced by the macOS window configuration through `--sysroot` and `-F` framework search flags. The SDK is not stored in the repository.

## Building

Clone the repository and enter its root directory:

```bash
git clone https://github.com/striter-no/ygpu.git
cd ygpu
```

Compile the example shaders:

```bash
./scripts/comp
```

### Linux

```bash
./scripts/target linux
axle build .
```

### Windows x86_64

```bash
./scripts/target win
axle build .
```

### macOS arm64

Install the macOS 13.3 SDK once on the Linux build host:

```bash
./scripts/setup-macos-sdk
```

The setup script downloads and extracts:

```text
https://github.com/joseluisq/macosx-sdks/releases/download/13.3/MacOSX13.3.sdk.tar.xz
```

Equivalent manual installation:

```bash
mkdir -p .dev

wget   -O .dev/MacOSX13.3.sdk.tar.xz   https://github.com/joseluisq/macosx-sdks/releases/download/13.3/MacOSX13.3.sdk.tar.xz

tar -xJf .dev/MacOSX13.3.sdk.tar.xz -C .dev
rm .dev/MacOSX13.3.sdk.tar.xz
```

The resulting directory must be:

```text
.dev/MacOSX13.3.sdk
```

Then select and build the target:

```bash
./scripts/target macos
axle build .
```

`scripts/target macos` checks that the SDK directory exists before changing the active configuration. If it is missing, the script exits and points to `scripts/setup-macos-sdk`.

The target script replaces `defaults.json` and the platform-specific `example_window/module.json`, then removes stale object and static-library outputs before the next build.

### Full release loop

```bash
./scripts/loop
```

The release loop:

1. builds the Linux target;
2. runs the tests on the native build host;
3. builds the Windows target;
4. builds the macOS target;
5. creates platform archives in `.packages`;
6. prints SHA-256 checksums for the generated archives.

## Running the examples

Run examples from the repository root because shader and texture paths are currently relative to it.

### Device creation

```bash
./.modules/example_device/bin/main
```

Creates a Vulkan device and prints the selected GPU name and active Vulkan API version.

### Textured quad

Linux:

```bash
./.modules/example_window/bin/main-linux
```

Windows:

```powershell
.\.modules\example_window\bin\main-win.exe
```

macOS:

```bash
./.modules/example_window/bin/main-macos
```

Creates a window and swapchain, uploads vertex/index data and a texture, builds descriptors and a graphics pipeline, and renders a rotating textured quad.

### Compute pipeline

```bash
./.modules/example_compute/bin/main
```

Demonstrates:

- CPU-visible staging buffers;
- GPU-local storage buffers;
- transfer and compute barriers;
- compute dispatch;
- copying the result back to mapped CPU memory.

## Quick start

```cpp
#include <device/device.hpp>

#include <iostream>

int main()
{
    auto config =
        yst::core::CreateConfig(yst::gpuc::DEBUG_CONFIG);

    auto [device, error] =
        yst::core::CreateDevice(config);

    if (error) {
        std::cerr << error.str() << '\n';
        return 1;
    }

    std::cout
        << "GPU: " << device.GetDeviceName()
        << '\n';

    const auto api = device.GetActiveApiVersion();

    std::cout
        << "Vulkan API: "
        << api.Major << '.'
        << api.Minor << '.'
        << api.Patch << '\n';
}
```

For a complete graphics example, see `.modules/example_window/main.cpp`.  
For compute usage, see `.modules/example_compute/main.cpp`.

## Using ygpu from another Axle project

The repository exports its aggregate module under the name `ygpu`. A consuming module can declare it as a dependency after the repository has been resolved by the current Axle dependency mechanism:

```json
{
  "dependencies": {
    "ygpu": {
      "version": ">=0.0.0"
    }
  }
}
```

And as remote dependency:

```json
{
  "remote_deps": {
    "ygpu": {
      "version": "=0.0.0",
      "url": "https://github.com/striter-no/ygpu"
    }
  }
}
```

The exported module brings in the core modules and does not export the project tests.

Because Axle is developed alongside this project, use the dependency syntax supported by the Axle version you have installed when adding the remote repository.

## Modules

| Module | Responsibility |
|---|---|
| `errors` | Error codes and `CustomError` |
| `device` | Vulkan instance, device selection, queues, and allocator setup |
| `window` | GLFW window creation, events, resize state, and Vulkan surface |
| `swapchain` | Frame acquisition, render targets, presentation, and recreation |
| `buffer` | Buffer creation, allocation, presets, uploads, and mapped access |
| `image` | Images, image views, samplers, and image configuration |
| `texture` | Texture loading, upload, views, samplers, and mipmaps |
| `shader` | SPIR-V loading, shader modules, and GLSL compilation |
| `descriptor` | Bind groups, layouts, descriptor pools, and pipeline layouts |
| `pipeline` | Graphics and compute pipeline configuration and creation |
| `command` | Command pools, command lists, barriers, copies, draw, and dispatch |
| `_` | Aggregate Axle module that depends on the complete public library |

## Testing

Build and run the test suite with:

```bash
axle test
```

The suite contains both unit-style validation tests and Vulkan integration tests. Tests that require a real device attempt to create a headless Vulkan device and skip gracefully when no suitable Vulkan implementation is available.

Current coverage includes:

- error values;
- images and image views;
- samplers;
- textures and mipmap generation;
- shader modules and SPIR-V loading;
- bind-group layouts and descriptor-type mapping;
- descriptor-pool creation, reset, and automatic sizing;
- bind-group allocation, updates, and multi-binding regression cases.

## Project structure

```text
.
├── .modules/              # ygpu modules and examples
│   ├── _/                 # aggregate module
│   ├── buffer/
│   ├── command/
│   ├── descriptor/
│   ├── device/
│   ├── errors/
│   ├── image/
│   ├── pipeline/
│   ├── shader/
│   ├── swapchain/
│   ├── texture/
│   ├── window/
│   ├── example_device/
│   ├── example_compute/
│   └── example_window/
├── .tests/                # unit and integration tests
├── assets/                # shaders and textures used by examples
├── scripts/               # target, shader, release, and package scripts
├── defaults*.json         # platform/toolchain defaults for Axle
├── module.json            # root Axle module
└── .as.module.json        # exported Axle module configuration
```

## Build configurations

The project currently defines the following Axle targets:

| Target | Purpose |
|---|---|
| `release` | Optimized build with `-O3` |
| `debug` | Debug information, AddressSanitizer, and `-O1` |
| `valgrind` | Debug information and `-O1`, without AddressSanitizer |
| `debug-silent` | Debug build with warnings suppressed |

All primary targets compile as C++20 and define `VK_NO_PROTOTYPES`.

## Known limitations

- The project is pre-1.0 and API compatibility is not guaranteed.
- The public C++ namespace still uses the historical `yst::` name.
- The release and target scripts currently require Bash and Unix command-line tools.
- Runtime GLSL compilation currently uses Unix process APIs and is not portable to native Windows execution.
- The current Windows and macOS configurations are cross-compilation targets.
- The macOS configuration currently targets arm64 and expects macOS SDK 13.3 in `.dev/MacOSX13.3.sdk`.
- The SDK archive is obtained from an external project and is not bundled with ygpu.
- Some advanced Vulkan features still require direct access to native Vulkan handles.
- Documentation is still growing; the examples are currently the most complete usage reference.

## Contributing

Issues, bug reports, API discussions, tests, and pull requests are welcome.

Before submitting a change:

1. format C++ sources using the repository's `.clang-format`;
2. build the affected target;
3. run `axle test`;
4. keep platform-specific behavior isolated where possible;
5. include a focused test for bug fixes and new resource-management paths.

Since the API is still evolving, opening an issue before a large architectural change is recommended.

## License

ygpu is free and open-source software licensed under the **GNU General Public License v3.0**.

See [`LICENSE`](LICENSE) for the full license text.

Third-party components included in or used by the repository retain their respective licenses.
