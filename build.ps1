# Gigalomania Windows build script
# Run from repo root in Administrator PowerShell:
#   Set-ExecutionPolicy -Scope Process Bypass
#   .\build.ps1
#
# On first run: installs Build Tools for Visual Studio and vcpkg (~5 GB total).
# Subsequent runs: skips installation and goes straight to build.

param(
    [string]$VcpkgRoot = "C:\vcpkg",
    [string]$Config    = "Release",
    [string]$Platform  = "x64",
    [string]$DistDir   = "dist\gigalomania"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Step($msg) { Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Ok($msg)   { Write-Host "[ok] $msg"   -ForegroundColor Green }
function Fail($msg) { Write-Host "[x]  $msg"   -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------------
# Step 1: Build Tools for Visual Studio 2022
# ---------------------------------------------------------------------------
Step "Checking Build Tools"

$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
$hasBuildTools = $false
if (Test-Path $vswhere) {
    $inst = & $vswhere -latest -prerelease -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath 2>$null
    $hasBuildTools = ($inst -ne $null -and $inst -ne "")
}

if ($hasBuildTools) {
    Ok "Build Tools already installed"
} else {
    Step "Installing Build Tools for Visual Studio 2022 (this takes ~10-15 min)"
    $installer = "$env:TEMP\vs_buildtools.exe"
    Write-Host "Downloading installer..."
    Invoke-WebRequest -Uri "https://aka.ms/vs/17/release/vs_buildtools.exe" -OutFile $installer
    Write-Host "Running installer (silent)..."
    $proc = Start-Process $installer -ArgumentList @(
        "--quiet", "--wait", "--norestart",
        "--add", "Microsoft.VisualStudio.Workload.VCTools",
        "--includeRecommended"
    ) -Wait -PassThru
    if ($proc.ExitCode -notin @(0, 3010)) {
        Fail "Build Tools installer exited with code $($proc.ExitCode)"
    }
    Ok "Build Tools installed"
}

# ---------------------------------------------------------------------------
# Step 2: vcpkg
# ---------------------------------------------------------------------------
Step "Checking vcpkg at $VcpkgRoot"

if (-not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
    if (-not (Test-Path $VcpkgRoot)) {
        Write-Host "Cloning vcpkg..."
        git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
    }
    Write-Host "Bootstrapping vcpkg..."
    & "$VcpkgRoot\bootstrap-vcpkg.bat" -disableMetrics
    if ($LASTEXITCODE -ne 0) { Fail "vcpkg bootstrap failed" }
}
Ok "vcpkg ready"

# ---------------------------------------------------------------------------
# Step 3: SDL dependencies
# ---------------------------------------------------------------------------
Step "Installing SDL3 dependencies via vcpkg"

$packages = @("sdl3", "sdl3-image[jpeg,png]", "libjpeg-turbo", "sdl2-mixer")
foreach ($pkg in $packages) {
    Write-Host "Installing $pkg ..."
    & "$VcpkgRoot\vcpkg" install $pkg --triplet x64-windows
    if ($LASTEXITCODE -ne 0) { Fail "vcpkg install $pkg failed" }
}

& "$VcpkgRoot\vcpkg" integrate install
if ($LASTEXITCODE -ne 0) { Fail "vcpkg integrate install failed" }
Ok "SDL dependencies installed"

# ---------------------------------------------------------------------------
# Step 4: Find MSBuild
# ---------------------------------------------------------------------------
Step "Locating MSBuild"

$msbuild = & $vswhere -latest -prerelease -products * `
    -requires Microsoft.Component.MSBuild `
    -find "MSBuild\**\Bin\MSBuild.exe" 2>$null | Select-Object -First 1

if (-not $msbuild -or -not (Test-Path $msbuild)) {
    Fail "MSBuild not found. Make sure Build Tools installation completed."
}
Ok "MSBuild: $msbuild"

# ---------------------------------------------------------------------------
# Step 5: Build
# ---------------------------------------------------------------------------
Step "Building $Config|$Platform"

& $msbuild gigalomania.sln `
    /p:Configuration=$Config `
    /p:Platform=$Platform `
    /p:VcpkgEnabled=true `
    /m

if ($LASTEXITCODE -ne 0) { Fail "Build failed" }
Ok "Build succeeded: $Platform\$Config\gigalomania.exe"

# ---------------------------------------------------------------------------
# Step 6: Package
# ---------------------------------------------------------------------------
Step "Packaging into $DistDir"

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

Copy-Item "$Platform\$Config\gigalomania.exe" $DistDir
Ok "Copied gigalomania.exe"

$vcpkgBin = "$VcpkgRoot\installed\x64-windows\bin"
$dlls = Get-ChildItem "$vcpkgBin\*.dll"
$dlls | Copy-Item -Destination $DistDir
Ok "Copied $($dlls.Count) DLLs from vcpkg"

foreach ($asset in @("gfx", "islands", "music", "sound")) {
    if (Test-Path $asset) {
        cmd /c xcopy /E /I /Y $asset "$DistDir\$asset\" | Out-Null
        Ok "Copied $asset\"
    } else {
        Write-Host "[warn] $asset\ not found, skipping" -ForegroundColor Yellow
    }
}
if (Test-Path "readme.html") { Copy-Item readme.html $DistDir }

# ---------------------------------------------------------------------------
Step "Done"
Write-Host ""
Write-Host "Build ready at: $DistDir" -ForegroundColor Green
Write-Host "Run with:       .\$DistDir\gigalomania.exe"
Write-Host "Log at:         %APPDATA%\Gigalomania\log.txt"
