# PratStrike

A 2D top-down action shooter built in C++ and SDL2, focused on game feel.

[GIF]

## How to build

### macOS
`cmake -B build`

`cmake --build build`

### Windows
`cmake -B build`

`cmake --build build --config Release`

### Linux
`cmake -B build`

`cmake --build build`

## Controls

| Action | Input |
| --- | --- |
| Rotate left | `A` / Left Arrow |
| Rotate right | `D` / Right Arrow |
| Accelerate | `W` / Up Arrow |
| Brake / reverse thrust | `S` / Down Arrow |
| Fire | Left Mouse / `Space` |
| Dash | Left Shift |
| Restart after death | `R` |
| Quit | `Esc` |

## Technical features

- Custom ECS with sparse-set component storage
- Fixed-timestep physics loop with render interpolation
- Quadtree-accelerated AABB collision detection
- Hitstop (physics freeze on impact, 40–100ms depending on hit weight)
- Screenshake with intensity and decay
- Render interpolation between fixed physics steps for smooth 60fps visuals
- Spatial audio panning via SDL2_mixer `Mix_SetPanning`
- Three-phase boss fight with state machine and distinct attack patterns per phase
- Object pool allocator for bullet entities
- Per-hit i-frames with flicker rendering

## Design decisions

I built PratStrike around an ECS rather than deep inheritance because the gameplay is driven by combinations of data: enemies, bullets, pickups, particles, hit flashes, and boss states all overlap in different ways. A sparse-set ECS keeps those combinations flexible while still giving me dense iteration for the hot systems like physics, AI, and rendering.

The simulation runs on a fixed timestep so tuning remains deterministic and consistent across machines. Movement, collision response, dash timing, fire cadence, and hitstop all feel noticeably better when they are evaluated at a stable 60 Hz instead of being tied directly to frame rate variability.

Hitstop is implemented as explicit world state rather than as an ad hoc pause flag scattered through unrelated systems. That keeps the freeze behavior readable and controllable: physics and AI can honor the same source of truth while rendering still updates particles, camera shake, and UI.

I also chose to keep game feel utilities as standalone helpers instead of baking them into a monolithic manager. Screenshake, particles, tint flashes, and spatial audio can be triggered from collisions, boss transitions, or deaths without introducing heavy coupling between systems.

## What I would add next

- A lightweight level editor for encounter authoring and wave pacing iteration
- GPU-driven particles and trails for denser combat feedback
- Deterministic rollback-friendly networking for co-op or versus experiments

## License

MIT
