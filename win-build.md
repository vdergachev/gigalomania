# Windows Build Guide (without Visual Studio IDE)

Build Gigalomania on Windows 10/11 using only free CLI tools — no Visual Studio IDE required.

## What gets installed

| Tool | Size | Purpose |
|------|------|---------|
| Build Tools for Visual Studio 2022 | ~5 GB | MSVC compiler + MSBuild + Windows SDK |
| vcpkg | ~100 MB + deps | SDL3/SDL3_image/SDL2_mixer packages |
| Git | ~300 MB | Required by vcpkg bootstrap |

---

## Quick start (automated)

Run `build.ps1` from the repo root in an **Administrator PowerShell**:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build.ps1
```

The script installs Build Tools and vcpkg on first run, then builds the game and produces `dist\gigalomania\` ready to run.

---

## Manual step-by-step

### 1. Install Git

Download and install from https://git-scm.com/download/win (needed by vcpkg).

### 2. Install Build Tools for Visual Studio 2022

Download `vs_buildtools.exe` from:
https://visualstudio.microsoft.com/visual-cpp-build-tools/

Run a **silent install** from an Administrator PowerShell:

```powershell
.\vs_buildtools.exe --quiet --wait `
  --add Microsoft.VisualStudio.Workload.VCTools `
  --includeRecommended
```

This installs: MSVC compiler, MSBuild, Windows SDK, CMake. No IDE.

Or with `winget` (Windows 10 1709+):

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools `
  --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

### 3. Install vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat -disableMetrics
```

### 4. Install SDL dependencies

```powershell
C:\vcpkg\vcpkg install sdl3 "sdl3-image[jpeg,png]" libjpeg-turbo sdl2-mixer --triplet x64-windows
C:\vcpkg\vcpkg integrate install
```

The `[jpeg,png]` feature flags are required — without them SDL3_image is
compiled without JPEG/PNG support and the game crashes on startup with
"Unsupported image format".

### 5. Build

Find MSBuild and build the Release target:

```powershell
$msbuild = & "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -prerelease -products * `
    -requires Microsoft.Component.MSBuild `
    -find "MSBuild\**\Bin\MSBuild.exe"

& $msbuild gigalomania.sln /p:Configuration=Release /p:Platform=x64 /p:VcpkgEnabled=true
```

Output: `x64\Release\gigalomania.exe`

### 6. Package (copy assets + DLLs)

```powershell
$distDir = "dist\gigalomania"
New-Item -ItemType Directory -Force -Path $distDir

# Executable
Copy-Item "x64\Release\gigalomania.exe" $distDir

# SDL DLLs from vcpkg
$vcpkgBin = "C:\vcpkg\installed\x64-windows\bin"
Get-ChildItem "$vcpkgBin\*.dll" | Copy-Item -Destination $distDir

# Game assets
cmd /c xcopy /E /I /Y gfx     dist\gigalomania\gfx\
cmd /c xcopy /E /I /Y islands dist\gigalomania\islands\
cmd /c xcopy /E /I /Y music   dist\gigalomania\music\
cmd /c xcopy /E /I /Y sound   dist\gigalomania\sound\
Copy-Item readme.html $distDir
```

### 7. Run

```powershell
.\dist\gigalomania\gigalomania.exe
```

Log file is written to `%APPDATA%\Gigalomania\log.txt` on every run.

---

## Troubleshooting

**"MSBuild not found"**
Build Tools did not finish installing. Rerun step 2 and wait for completion (can take 10-15 minutes).

**"vcpkg is not recognized"**
Use the full path `C:\vcpkg\vcpkg` or add `C:\vcpkg` to `PATH`.

**"Unsupported image format" in log.txt**
SDL3_image was installed without JPEG/PNG features. Reinstall with:
```powershell
C:\vcpkg\vcpkg remove sdl3-image --triplet x64-windows
C:\vcpkg\vcpkg install "sdl3-image[jpeg,png]" --triplet x64-windows
```

**"Failed to open screen"**
SDL3.dll not found. Ensure all `.dll` files from `C:\vcpkg\installed\x64-windows\bin\` are in the same folder as `gigalomania.exe`.
