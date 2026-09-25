# Archdrift

C++23 / Vulkan arcade kart racer with custom drift/boost, local races, and QR-paired phone controllers (web UI + gyro) on a closed LAN session.

Original content — Mario Kart–*inspired*, not affiliated with Nintendo. Developed in explicit phases so the architecture and git history stay portfolio-friendly.

## Status

**Phase 0** — window + Vulkan triangle.

## Requirements

- Linux (Fedora, Ubuntu, and similar)
- CMake ≥ 3.28
- C++23 compiler (GCC 13+ / Clang 16+)
- Vulkan-capable GPU and `libvulkan`
- Network access on first configure (FetchContent may pull GLFW / Vulkan-Headers)
- Optional: `tools/glslang` (or system `glslangValidator`) to recompile shaders

### Fedora packages (optional if you prefer system libs)

```bash
sudo dnf install glfw-devel vulkan-loader-devel vulkan-headers \
  vulkan-validation-layers glm-devel
```

### Ubuntu packages (also used by CI)

```bash
sudo apt install build-essential cmake ninja-build libvulkan-dev \
  glslang-tools libx11-dev libwayland-dev libxkbcommon-dev
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/archdrift
```

Close the window or press Escape to quit.

## Recompile shaders

```bash
cmake --build build --target shaders
```

## Docs

- [Architecture](docs/architecture.md) — layers, errors, concurrency, memory, phone trust model
- [Phases](docs/phases.md) — milestone map

## License

[MIT](LICENSE)
