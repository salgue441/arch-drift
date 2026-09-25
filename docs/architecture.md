# Architecture

**Archdrift** is a Linux-first arcade kart project built in C++23 on Vulkan. The game is a single process with clear layer boundaries so each subsystem stays testable and explainable.

## Layers

| Layer | Responsibility |
|-------|----------------|
| `platform` | Window, clock, filesystem paths |
| `render` | Vulkan device, swapchain, pipelines, GPU resources |
| `input` | Keyboard and phone pads unified as `KartInput` snapshots |
| `sim` | Arcade kart / track / race (no GPU) |
| `game` | Mode stack (boot, lobby, race) |
| `net_host` | Session token, HTTP static controller UI, WebSocket input |

Phone browsers never simulate the race: they only send input. Simulation authority stays on the host.

## Engineering standards

### Errors

- Recoverable failures use `std::expected` (see `kart::Result`).
- Exceptions are reserved for broken invariants where continuing would corrupt state.
- Vulkan `VkResult` values are mapped at API boundaries; callers never see raw codes without context.
- Do not swallow failures with empty `catch (...)`.

### Concurrency

- Main thread owns simulation ticks and Vulkan submit.
- Network I/O (HTTP + WebSocket) runs on a dedicated thread (Boost.Asio planned in Phase 3).
- Cross-thread input uses a snapshot queue: producers publish `KartInput`, the sim consumes once per tick.

### Memory

- Vulkan and OS handles live in move-only RAII types.
- GPU allocations go through a dedicated allocator (VMA) once mesh/buffer work begins.
- No owning raw `new` / `delete` in application code.
- Optional per-frame arenas may hold scratch CPU data later.

## Phone pairing (Phase 3)

Host creates a session with a cryptographically random token. The lobby QR encodes a LAN URL that includes that token. There is no open lobby browse: without the token the host rejects controller attachment.

## Dependencies

| Library | Role |
|---------|------|
| GLFW | Window + input |
| Vulkan loader / headers | Rendering |
| GLM | Math (Phase 1+) |
| glslangValidator | GLSL → SPIR-V at build |

Trusted libraries (Boost, VMA, GoogleTest, etc.) may be added later with a one-line rationale here.

## Documentation

Source comments follow [coding-standards.md](coding-standards.md) (Doxygen `///` on public APIs). Phase module maps live under [`docs/api/`](api/).
