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

## Building on Windows

### Option A: Download pre-built binary

Download the latest build from [Actions artifacts](https://github.com/vdergachev/gigalomania/actions) (requires GitHub login).

### Option B: Build yourself

1. Install [Visual Studio 2022 Community](https://visualstudio.microsoft.com/) with **Desktop development with C++**
2. Install [vcpkg](https://vcpkg.io/) and the SDL2 libraries:
   ```bat
   git clone https://github.com/microsoft/vcpkg
   .\vcpkg\bootstrap-vcpkg.bat
   .\vcpkg\vcpkg install sdl2 sdl2-image sdl2-mixer --triplet x64-windows
   .\vcpkg\vcpkg integrate install
   ```
3. Open `gigalomania.sln` in Visual Studio, select **Release / x64**, press **Build**.
4. Copy the SDL2 DLLs from `<vcpkg>\installed\x64-windows\bin\` next to the resulting `x64\Release\gigalomania.exe`.

## Controls

- Mouse / touchscreen — full control
- `P` — pause/unpause
- `Escape` — quit

## License

GPL v2 or later. See original readme.html for full details and credits.
