param(
    [string]$MinecraftDirectory = "$env:APPDATA\.minecraft",
    [string]$Version = '26.3',
    [string]$JarPath = ''
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
if (-not $JarPath) { $JarPath = Join-Path $MinecraftDirectory "versions\$Version\$Version.jar" }
if (-not (Test-Path -LiteralPath $JarPath)) { throw "Minecraft JAR not found: $JarPath. Launch your owned copy once, or supply -JarPath." }
$destination = Join-Path $PSScriptRoot '..\release\assets'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
$names = @('grass_block_top','grass_block_side','dirt','stone','sand','oak_log','oak_log_top','oak_leaves','oak_planks','coal_ore','iron_ore','diamond_ore','water_still','snow','bricks','glass','bedrock','glowstone')
$archive = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $JarPath))
try {
    foreach ($name in $names) {
        $entry = $archive.GetEntry("assets/minecraft/textures/block/$name.png")
        if (-not $entry) { throw "Missing expected block texture: $name" }
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, (Join-Path $destination "$name.png"), $true)
    }
} finally { $archive.Dispose() }
Write-Host "Imported $($names.Count) local block textures. Assets stay on this computer."
