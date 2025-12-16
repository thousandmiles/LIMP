# LIMP Build Script for Windows (PowerShell)
param(
    [string]$Preset = "release"
)

$ErrorActionPreference = "Stop"

$ConanBuildType = "Release"

# Map presets to Conan build types
switch ($Preset) {
    {$_ -in "debug", "test"} {
        $ConanBuildType = "Debug"
    }
}

Write-Host "==> Installing dependencies with Conan ($ConanBuildType)..." -ForegroundColor Cyan
conan install . --output-folder=build/$Preset --build=missing -s build_type=$ConanBuildType -o "*:shared=False"

Write-Host "==> Configuring with CMake (preset: $Preset)..." -ForegroundColor Cyan
cmake --preset $Preset

Write-Host "==> Building..." -ForegroundColor Cyan
cmake --build --preset $Preset

if ($Preset -eq "test") {
    Write-Host "==> Running tests..." -ForegroundColor Cyan
    ctest --preset test
}

Write-Host "==> Build complete! Output in build/$Preset/" -ForegroundColor Green
