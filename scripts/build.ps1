param([switch]$SkipAssets)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
cmake -S $root -B (Join-Path $root 'build') -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
cmake --build (Join-Path $root 'build') --config Release
if ($LASTEXITCODE -ne 0) { throw 'C build failed' }
if (-not $SkipAssets) { & (Join-Path $PSScriptRoot 'import-assets.ps1') }
ctest --test-dir (Join-Path $root 'build') -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'World integrity tests failed' }
Write-Host "Ready: $(Join-Path $root 'release\Parallax.exe')"
