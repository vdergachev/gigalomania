# Linux Build Guide

Tested on Ubuntu 24.04 LTS (arm64 and amd64).

## Dependencies

SDL3 and SDL3_image are not yet in Ubuntu apt — build from source.
SDL2_mixer is available in apt.

```bash
sudo apt install build-essential pkg-config cmake git libpng-dev libjpeg-dev libsdl2-mixer-dev
```

### Build SDL3

```bash
git clone --depth=1 https://github.com/libsdl-org/SDL.git -b release-3.2.14
cd SDL
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=ON -DSDL_STATIC=OFF
cmake --build build -j$(nproc)
sudo cmake --install build
cd ..
```

### Build SDL3_image

```bash
git clone --depth=1 https://github.com/libsdl-org/SDL_image.git -b release-3.2.4
cd SDL_image
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
sudo ldconfig
cd ..
```

## Build the game

```bash
git clone <repo-url> gigalomania
cd gigalomania
make
./gigalomania
```

## Local VM (Apple Silicon)

For GUI testing on Apple Silicon Mac use UTM (https://getutm.app):

- New VM → **Virtualize** (not Emulate) for native arm64
- Ubuntu 24.04 Desktop arm64 ISO
- RAM: 4 GB, CPU: 4 cores, Storage: 30 GB
- Network: Shared Network (VM gets IP in 192.168.106.x or 192.168.107.x range)
- Display: VirtIO

After install, enable SSH in the VM:

```bash
sudo apt install openssh-server
sudo systemctl enable --now ssh
```

SSH config on Mac (`~/.ssh/config`):

```
Host gigalomania-linux
    HostName 192.168.107.3
    User user
    IdentityFile ~/.ssh/linux_gigalomania
    StrictHostKeyChecking no
```

Connect: `ssh gigalomania-linux`
