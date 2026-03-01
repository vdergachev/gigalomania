# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**macOS:**
```sh
brew install sdl3 sdl3_image sdl2_mixer
make
./gigalomania
```

Note: SDL3_mixer is not yet in homebrew; SDL2_mixer is used as a temporary shim.

**Linux (Ubuntu 24.04):** SDL3 and SDL3_image are not in apt — build from source.
See `.claude/docs/` for detailed instructions (only read on explicit request).

**Windows:** Open `gigalomania.sln` in Visual Studio 2022, build Release/x64. Dependencies via vcpkg:
```bat
vcpkg install sdl3 sdl3-image[png,jpeg] sdl2-mixer --triplet x64-windows
```

Note: vcpkg does not ship `SDL3main.lib`. The project uses `SDL_MAIN_HANDLED` + custom `WinMain` in `main.cpp` to handle this.

**Clean:**
```sh
make clean
```

There are no automated tests.

## Architecture

Entry point: `main.cpp` → `playGame()` in `game.cpp`.

**Game state machine** (`gamestate.cpp`): States cycle through menu screens (choose game type → difficulty → player placement) into the play loop and end screens. Each state is a class with `run()` / `draw()` methods.

**Core simulation** (`sector.cpp`, ~6400 lines + `sector_combat.cpp`, `sector_ai.cpp`): The map is a 5×5 grid of sectors. Each sector owns population, buildings (tower/mine/factory/lab), armies, and particles. Combat logic lives in `sector_combat.cpp` (`doCombat`), AI decision logic in `sector_ai.cpp` (`doPlayer`); both are compiled as separate translation units that include `sector.h`.

**Player & AI** (`player.cpp`): Human and AI players share the same `Player` class.

**Rendering pipeline**: `screen.cpp` manages the SDL3 window and renderer; `image.cpp` wraps SDL3_image for sprite loading/blitting; `panel.cpp` / `gui.cpp` implement the UI widget layer (buttons, panels, selection screens).

**Input dispatch** (`panel.cpp`): `GamePanel::input()` delegates to per-page handler methods (`inputSectorControlPage`, etc.). `PanelPage::setOnClick(std::function<void(bool,bool)>)` fires callbacks on mouseOver and click events.

**Asset paths are hardcoded as relative** (`"gfx/"`, `"sound/"`, `"islands/"`). The binary must be run from the directory containing these folders, or — in the macOS `.app` bundle — the launcher script sets CWD to `Contents/Resources/` before exec-ing the binary.

**Save/load** uses TinyXML (embedded in `TinyXML/`) to write XML game state files (`autosave.sav`, `prefs`).

## SDL3 migration notes

The codebase was migrated from SDL2 to SDL3 on the `MMM-migrate-to-sdl3` branch:

- All SDL2 API calls updated to SDL3 equivalents
- `SDL_CreateWindowAndRenderer` returns `bool` in SDL3 — check with `!func()`, not `!= 0`
- SDL3_image: include path is `<SDL3_image/SDL_image.h>`
- SDL2_mixer kept as-is (SDL3_mixer not yet widely available); sound.cpp wraps it behind the existing sound abstraction
- Windows: `SDL_MAIN_HANDLED` defined in `stdafx.h`; custom `WinMain` calls `SDL_SetMainReady()` then `playGame()`
- Platform-specific SDL includes are gated in `stdafx.h` — add new platforms there

## CI

- `.github/workflows/build-macos.yml` — builds, creates `Gigalomania.app` bundle (launcher script + SDL3 dylibs via dylibbundler), uploads `gigalomania-macos.zip`
- `.github/workflows/build-windows.yml` — builds with MSBuild + vcpkg, uploads `gigalomania-windows.zip`
- `.github/workflows/build-linux.yml` — builds on `ubuntu-latest` (amd64); SDL3 and SDL3_image built from source with caching; uploads `gigalomania-linux.tar.gz`
- `.github/workflows/build-android.yml` — disabled (`if: false`); Android has not been migrated to SDL3 yet

## Project docs

Extended notes, research, and ideas live in `.claude/docs/`. These files may be outdated — read them only when explicitly asked.

## Platform notes

- **SDL includes** are platform-gated in `stdafx.h` — add new platform branches there, not in individual files.
- `android/` directory exists; `Android.mk` is kept up to date with the source file list but the Android build is not actively maintained (still on SDL2).
