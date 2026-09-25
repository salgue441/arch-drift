# Qantara Kart

Linux arcade kart racer (Mario Kart–*inspired*, original content) built with **C++23** and **Vulkan**.

This repository is developed in explicit phases so the architecture and git history show deliberate progress: rendering first, then handling, then phone controllers, then race systems.

## Status

**Phase 0** — window + Vulkan triangle.

## Requirements

- Linux (Fedora and similar)
- CMake ≥ 3.28
- C++23 compiler (GCC 13+ / Clang 16+)
- Vulkan-capable GPU and `libvulkan`
- Network access on first configure (FetchContent pulls GLFW and Vulkan-Headers)
- Optional: `tools/glslang` (auto-downloaded) to recompile shaders

### Fedora packages (optional if you prefer system libs)

```bash
sudo dnf install glfw-devel vulkan-loader-devel vulkan-headers \
  vulkan-validation-layers glm-devel
```

The build also works without those devel packages by fetching headers/GLFW via CMake.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/qantara_kart
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

TBD.
