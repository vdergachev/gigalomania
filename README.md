# Gigalomania

Open source 2D real-time strategy game. Research and develop technology — from rocks and sticks to nuclear weapons and spaceships — across 10 ages and 28 maps.

Original project: https://sourceforge.net/p/gigalomania/

## Building

### macOS

```sh
brew install sdl3 sdl3_image sdl2_mixer
make
./gigalomania
```

Note: SDL3_mixer is not yet in Homebrew; SDL2_mixer is used as a temporary shim.

### Linux (Ubuntu 24.04)

SDL3 and SDL3_image are not in apt — build from source. See `.claude/docs/` for instructions.

```sh
make
./gigalomania
```

### Windows

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with **Desktop development with C++**
2. Install [vcpkg](https://vcpkg.io/) and the SDL3 libraries:
   ```bat
   vcpkg install sdl3 sdl3-image[png,jpeg] sdl2-mixer --triplet x64-windows
   ```
3. Open `gigalomania.sln`, select **Release / x64**, press **Build**.

## Controls

- Mouse / touchscreen — full control
- `P` — pause/unpause
- `Escape` — quit

## Logs

Each run creates a timestamped log file. The last 2 sessions are kept.

| Platform | Location |
|---|---|
| macOS   | `~/Library/Application Support/Gigalomania/Gigalomania/logs/` |
| Linux   | `~/.local/share/Gigalomania/Gigalomania/logs/` |
| Windows | `%APPDATA%\Gigalomania\Gigalomania\logs\` |

Run with `verbose` to enable debug-level output, or `--log-level=warn` to suppress everything below warnings:

```sh
./gigalomania verbose
./gigalomania --log-level=warn
```

## License

GPL v2 or later. See `readme.html` for full details and credits.
