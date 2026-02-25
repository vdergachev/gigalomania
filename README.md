# Gigalomania

Open source 2D Real Time Strategy god game. Research and develop technology — from rocks and sticks to nuclear weapons and spaceships — across 10 ages and 28 maps.

Original project: https://sourceforge.net/p/gigalomania/

## macOS patch

This fork fixes the build on modern macOS (tested on macOS 14/15):

- Added `#elif defined(__APPLE__)` branch in `stdafx.h` with correct `<SDL.h>` include (case-sensitive path) and `<unistd.h>` (provides `access()` syscall missing in original).

## Building on macOS

Install dependencies via Homebrew:

```sh
brew install sdl2 sdl2_image sdl2_mixer
```

Then build:

```sh
make
./gigalomania
```

## Controls

- Mouse / touchscreen — full control
- `P` — pause/unpause
- `Escape` — quit

## License

GPL v2 or later. See original readme.html for full details and credits.
