# Phase 1 API map

Phase 1 adds a driveable placeholder race: oval track, box kart, chase camera, keyboard input.

## Modules

```text
input/KartInput.hpp         Control snapshot
input/KeyboardSource.*      WASD / arrows / Shift / Space
sim/Tunables.*              Handling constants (+ optional JSON)
sim/Kart.*                  Fixed-step arcade drive
sim/Track.*                 Procedural oval mesh + ground sample
sim/Camera.*                Chase view / projection
render/MeshData.*           CPU mesh builders
render/Renderer.*           Depth + mesh draws via dynamic UBO
shaders/mesh.*              Lit vertex-color mesh pipeline
```

## Controls

| Input | Action |
|-------|--------|
| W / Up | Throttle |
| S / Down | Brake / reverse |
| A/D or arrows | Steer |
| Escape | Quit |

## Sim / render contract

- Simulation uses fixed `dt = 1/60`.
- `Track::sample` supplies ground height (flat in Phase 1).
- `Renderer::draw_frame(FrameScene)` draws up to 16 meshes per frame with per-draw dynamic UBO offsets.
- Window must outlive Renderer; GPU meshes must be destroyed before the Renderer.

## Build note

GLM is required (`find_package` or FetchContent). Compile definitions force radians and Vulkan depth range `[0, 1]`.
