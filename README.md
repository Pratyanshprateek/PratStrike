# 🎮 PratStrike — 2D Action Shooter (C++ / SDL2)

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![SDL2](https://img.shields.io/badge/Engine-SDL2-green)
![Build](https://img.shields.io/badge/Build-CMake-orange)
![Status](https://img.shields.io/badge/Status-Active-success)
![Focus](https://img.shields.io/badge/Focus-Game%20Systems%20%26%20Feel-purple)

A **2D top-down action shooter** built in **C++ using SDL2**, focused heavily on **game feel, responsiveness, and systems design**.

---

---

## 🚀 Overview

PratStrike is designed as a **systems-first game project**, emphasizing:

- Tight, responsive controls  
- Deterministic simulation  
- Modular architecture (ECS)  
- Strong moment-to-moment game feel  

---

## 🎮 Controls

| Action | Input |
|---|---|
| Rotate left | `A` / ← |
| Rotate right | `D` / → |
| Move forward | `W` / ↑ |
| Reverse thrust | `S` / ↓ |
| Fire | Left Mouse / `Space` |
| Dash | `Left Shift` |
| Restart after death | `R` |
| Quit | `Esc` |

---

## ⚙️ Technical Features

- 🧩 Custom **ECS (Entity Component System)** with sparse-set storage  
- ⏱️ Fixed-timestep physics loop + render interpolation  
- 📦 **Quadtree-accelerated AABB collision detection**  
- 💥 Hitstop system (40–100ms based on impact weight)  
- 📳 Screenshake with intensity + decay  
- 🎯 Smooth 60 FPS rendering via interpolation  
- 🔊 Spatial audio panning (`SDL2_mixer`)  
- 👾 Multi-phase boss AI with state machine  
- 🚀 Object pool allocator for bullet entities  
- 🛡️ Per-hit invincibility frames (i-frames) with flicker rendering  

---

## 🧠 Design Decisions

### ECS Architecture
PratStrike uses a **data-oriented ECS** instead of inheritance-heavy design.  
This allows flexible composition of entities like enemies, bullets, particles, and boss states while maintaining **cache-friendly iteration** for performance-critical systems.

---

### Deterministic Simulation
A **fixed timestep loop** ensures:
- Consistent gameplay across machines  
- Stable physics and timing  
- Reliable tuning of mechanics like dash, fire rate, and collisions  

---

### Game Feel Systems
Game feel is treated as a **first-class system**:

- Hitstop implemented as a global state  
- Screenshake, particles, and audio decoupled from core logic  
- Visual + physical feedback tightly synchronized  

---

## 🏗️ Build Instructions

### macOS / Linux
```bash
cmake -B build
cmake --build build
```

### Windows
```
cmake -B build
cmake --build build --config Release
```

### Project Structure
```
PratStrike/
├── src/
├── assets/
├── CMakeLists.txt
└── README.md
```

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

## 🔮 Future Improvements

- 🛠️ Lightweight level editor for encounter design  
- ✨ GPU-driven particles & trails  
- 🌐 Deterministic rollback networking (co-op / PvP)  

---

## 💡 Highlights

- Built from scratch in **modern C++**  
- Focus on **systems design + performance**  
- Strong emphasis on **game feel mechanics**  
- Clean, modular architecture (ECS)  

---

## 👨‍💻 Author

**Pratyansh Prateek**  
🎓 B.Tech CSE | Game Systems | ML Engineer  

🔗 LinkedIn: https://www.linkedin.com/in/pratyansh-prateek/  
💻 GitHub: https://github.com/Pratyanshprateek  

---

⭐ If you like this project, consider giving it a star on GitHub!
