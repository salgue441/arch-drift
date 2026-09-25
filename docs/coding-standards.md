# Coding & documentation standards

Conventions for Archdrift C++ sources. Follow these on every new public type or function.

## Language & style

- **C++23**, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`).
- Prefer **RAII** and move-only types for OS/GPU handles.
- Prefer **`kart::Result` / `std::expected`** for recoverable failures; exceptions only for broken invariants.
- Mark pure observers and fallible factories with `[[nodiscard]]`.
- Use `noexcept` where the implementation cannot throw (and document if a callee later can).
- Namespaces: `kart` is the engine root (`kart::platform`, `kart::render`, …). The product name is **Archdrift**.

## What to document

Document **public headers** (and free functions meant for other layers):

| Document | Skip |
|----------|------|
| Type/class purpose and ownership | Getters that only return a field name |
| Thread-safety / which thread may call | Obvious one-line wrappers |
| Preconditions and lifetimes (who owns what) | Every private helper |
| Non-obvious error cases returned via `Result` | Restating the code in prose |
| Invariants and units (seconds, radians, NDC) | TODO noise in committed code |

Private implementation details in `.cpp` files get a short `@file` brief only, unless an algorithm needs a non-obvious note.

## Comment format (Doxygen)

Use `///` (or `/** … */` for multi-paragraph) so tools can generate API docs later.

```cpp
/// @file Window.hpp
/// @brief GLFW window RAII wrapper for Vulkan (no OpenGL context).

/// Creates a borderless-capable Vulkan window.
/// @param config Size and title; client API is forced to NO_API.
/// @return Owning window, or Error::WindowInitFailed / VulkanUnavailable.
/// @note Call attach_callbacks() after the Window lives at its final address.
[[nodiscard]] static Result<Window> create(const WindowConfig& config);
```

Useful tags: `@param`, `@return`, `@retval`, `@note`, `@warning`, `@pre`, `@post`, `@thread_safety`.

## Layer rules (brief)

- **`platform`**: OS/window only; no Vulkan types in public headers except where unavoidable.
- **`render`**: Vulkan submit stays on the **main** thread; no blocking net I/O.
- **`input` / `sim` / `net_host`**: document producer/consumer thread for any shared snapshot.

## Docs tree

| Path | Role |
|------|------|
| [architecture.md](architecture.md) | System design |
| [phases.md](phases.md) | Milestone map |
| [coding-standards.md](coding-standards.md) | This file |
| [api/phase-0.md](api/phase-0.md) | Phase 0 module map |

Keep commit messages and docs free of tooling/AI mentions; describe engineering decisions only.
