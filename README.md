# sdlprac

A 2D platformer written from scratch in C++ using SDL2 and OpenGL. Started as a way to get familiar with writing a game loop, ECS, and a batched sprite renderer without leaning on an engine. It's grown into something playable.

![Platform](https://img.shields.io/badge/platform-Linux-blue) ![Language](https://img.shields.io/badge/language-C%2B%2B17-orange) ![License](https://img.shields.io/badge/license-MIT-green)

---

## What it is

Side-scrolling platformer with a hand-rolled engine. No Unity, no Godot, no SFML game loop hiding the details. Every system — physics, rendering, input, state management, animation — is written directly.

The point was to understand what engines do under the hood, not to ship a game. That said, it runs, has a working player, tile collision, a HUD, and transitions between states.

---

## Building

**Dependencies:**

```
libsdl2-dev
libsdl2-mixer-dev
libglew-dev
libglm-dev
```

Install on Ubuntu/Debian:

```bash
sudo apt install libsdl2-dev libsdl2-mixer-dev libglew-dev libglm-dev
```

**Build:**

```bash
make
./app
```

---

## Controls

| Key | Action |
|-----|--------|
| A / D | Move left / right |
| Space | Jump |
| ESC | Back to title |

Coyote time and jump buffering are implemented, so jump timing feels forgiving rather than frame-perfect.

---

## Structure

```
src/          — all .cpp implementation files
include/      — headers
res/
  tiles.png   — 8×4 tileset (48px tiles)
  player.png  — 4×3 spritesheet (36×48 frames, idle/run/jump rows)
  font.png    — 16×6 bitmap glyph sheet
  audio/
    jump.wav
    land.wav
obj/          — compiled object files (generated)
```

The engine is split into mostly independent systems:

- **ECS** (`ECS.h`) — entity/component storage using unordered maps per type. Not cache-optimal but straightforward to use.
- **Renderer2D** — batched quad renderer. Builds a vertex buffer per frame, flushes on texture change or batch limit. One draw call per batch.
- **PhysicsWorld** — split-axis AABB resolution against a tilemap. X and Y passes are separate, which handles corner cases cleanly.
- **AnimationSystem** — UV-frame animation driven by clip definitions. Writes UV coords directly into SpriteComponent each frame.
- **GameStateMachine** — stack-based state machine. Transitions are deferred to the start of the next frame to avoid mid-update state changes.
- **ResourceManager** — cache-first asset loader for textures (stb_image), sound effects, and music (SDL2_mixer).
- **InputManager** — scancode + named action bindings, single-frame pressed/released flags, gamepad support.
- **Camera** — orthographic and perspective modes, FPS-style mouse look, view/projection matrix output.
- **HUD** — screen-space quad elements: health pips, panels, progress bars, bitmap font text.

---

## Known limitations

- No serialised level format. The tile grid is hardcoded in `GameplayState.cpp` as a flat array.
- Font rendering draws placeholder coloured quads unless you supply a proper ASCII bitmap font at `res/font.png` (16 cols × 6 rows, 16×16px glyphs, ASCII 32–127).
- One active level. The state machine supports stacking states but there's only one gameplay level.
- No entity pooling. The ECS uses `unordered_map` per component type — fine at this scale, slow if you spawn hundreds of entities per frame.

---

## Dependencies / credits

- [SDL2](https://www.libsdl.org/) — window, input, audio context
- [SDL2_mixer](https://wiki.libsdl.org/SDL2_mixer) — WAV/OGG playback
- [GLEW](https://glew.sourceforge.net/) — OpenGL extension loading
- [GLM](https://github.com/g-truc/glm) — math
- [stb_image](https://github.com/nothings/stb) — PNG/JPEG loading (single header, included)
