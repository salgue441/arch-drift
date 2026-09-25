# Phase 0 API map

Phase 0 proves the Vulkan boot path: window → device → swapchain → colored triangle.

## Module map

```text
src/
  main.cpp                 Application entry; Escape to quit
  core/Result.hpp          Error codes + Result alias (std::expected)
  platform/Window.hpp      GLFW window (Vulkan-ready, NO_API)
  render/Renderer.hpp      Vulkan renderer (triangle demo)
shaders/
  triangle.vert / .frag    Hard-coded colored triangle (SPIR-V at build)
```

## Ownership & lifetime

1. `Window::create` → move into stable storage → `attach_callbacks()`.
2. `Renderer::create(window, shader_dir)` borrows the window for the surface; **window must outlive renderer**.
3. Renderer is move-only; destructor waits idle and destroys Vulkan objects.

## Threading

All Phase 0 APIs are **main-thread only**. No background Vulkan or GLFW calls.

## Error flow

Factories and `draw_frame` return `kart::Result` / `VoidResult`. `main` prints `kart::to_string(error)` and exits on failure. Validation layers are optional: if missing in Debug, rendering continues with a stderr notice.

## Build artifacts

| Artifact | Meaning |
|----------|---------|
| `build/archdrift` | Game binary |
| `build/shaders/*.spv` | Compiled SPIR-V (CMake `shaders` target) |

`KART_SHADER_DIR` is injected at compile time to the build tree shader output directory.
