# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**macOS:**
```sh
brew install sdl2 sdl2_image sdl2_mixer
make
./gigalomania
```

**Clean:**
```sh
make clean
```

**Windows:** Open `gigalomania.sln` in Visual Studio 2022, build Release/x64. Dependencies via vcpkg:
```bat
vcpkg install sdl2 "sdl2-image[libjpeg-turbo]" sdl2-mixer --triplet x64-windows
```

There are no automated tests.

## Architecture

Entry point: `main.cpp` → `playGame()` in `game.cpp`.

**Game state machine** (`gamestate.cpp`): States cycle through menu screens (choose game type → difficulty → player placement) into the play loop and end screens. Each state is a class with `run()` / `draw()` methods.

**Core simulation** (`sector.cpp`, ~6700 lines): The map is a 5×5 grid of sectors. Each sector owns population, buildings (tower/mine/factory/lab), armies, and particles. Combat, production, and AI all execute per-sector per game tick.

**Player & AI** (`player.cpp`): Human and AI players share the same `Player` class. AI decision logic lives in `sector.cpp` (AI chooses build targets and attacks based on epoch).

**Rendering pipeline**: `screen.cpp` manages the SDL2 window and renderer; `image.cpp` wraps SDL2_image for sprite loading/blitting; `panel.cpp` / `gui.cpp` implement the UI widget layer (buttons, panels, selection screens).

**Asset paths are hardcoded as relative** (`"gfx/"`, `"sound/"`, `"islands/"`). The binary must be run from the directory containing these folders, or — in the macOS `.app` bundle — the launcher script sets CWD to `Contents/Resources/` before exec-ing the binary.

**Save/load** uses TinyXML (embedded in `TinyXML/`) to write XML game state files (`autosave.sav`, `prefs`).

## CI

- `.github/workflows/build-macos.yml` — builds, creates `Gigalomania.app` bundle (launcher script + SDL2 dylibs via dylibbundler), uploads `gigalomania-macos.zip`
- `.github/workflows/build-windows.yml` — builds with MSBuild + vcpkg, uploads `gigalomania-windows.zip`

## Platform notes

- **SDL includes** are platform-gated in `stdafx.h` — add new platform branches there, not in individual files.
- SDL1 legacy code paths still exist behind `#ifdef` — the active codebase targets SDL2.
- `gigalomania.pro` (Qt project) and `android/` directory exist but are not maintained in this fork.
