param([string]$JarPath = "$env:APPDATA\.minecraft\versions\26.3\26.3.jar")
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$build=Join-Path $root 'build-experimental'
$output=Join-Path $root 'release\playable-experiment'
cmake -S $root -B $build -A x64 -DPARALLAX_BUILD_RETIRED_SANDBOX=ON
if($LASTEXITCODE -ne 0){throw 'Configuration failed'}
cmake --build $build --config Release --target ParallaxSandbox
if($LASTEXITCODE -ne 0){throw 'C compilation failed'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive=[IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $JarPath))
$assets=Join-Path $output 'assets'
New-Item -ItemType Directory -Force $assets | Out-Null
try {
    $reader=[IO.StreamReader]::new($archive.GetEntry('version.json').Open())
    try {$version=$reader.ReadToEnd()|ConvertFrom-Json}finally{$reader.Dispose()}
    if($version.id -ne '26.3'){throw 'The supplied JAR must be Minecraft 26.3'}
    $names=@('grass_block_top','grass_block_side','dirt','stone','sand','oak_log','oak_log_top','oak_leaves','oak_planks','coal_ore','iron_ore','diamond_ore','water_still','snow','bricks','glass','bedrock','glowstone')
    foreach($name in $names){
        $entry=$archive.GetEntry("assets/minecraft/textures/block/$name.png")
        if(-not $entry){throw "Missing texture: $name"}
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,(Join-Path $assets "$name.png"),$true)
    }
}finally{$archive.Dispose()}
$note=@'
PARALLAX — PLAYABLE EXPERIMENT

This is the earlier custom sandbox, NOT a faithful Minecraft 26.3 rewrite.
The separate release/Parallax.exe remains the incomplete reference build.

Run ParallaxSandbox.exe, choose Singleplayer Worlds, select an empty slot,
then Generate & Play. Keep the assets directory beside the executable.

WASD: move. Mouse: look. Space: jump. Ctrl: sprint. Left/right click: mine/place.
1-9 or wheel: block selection. E: inventory/crafting. Esc: pause/save.
F: experimental flight toggle. L: custom achievement list.

Worlds use this experiment's own finite generation and .pxw saves in saves/.
It has simplified mobs/recipes and 24 custom achievements. It cannot load
Minecraft worlds or provide the original game's mechanics and advancements.
Textures were extracted locally from your owned 26.3 JAR. No game classes
are used by this C executable. Do not upload the extracted assets to Git.
'@
[IO.File]::WriteAllText((Join-Path $output 'READ-ME-FIRST.txt'),$note,[Text.UTF8Encoding]::new($false))
$source=@{version=$version.id;jar_sha256=(Get-FileHash -LiteralPath $JarPath -Algorithm SHA256).Hash;texture_count=$names.Count;implementation='experimental sandbox, not Minecraft parity'}
$source|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $output 'asset-source.json')
Push-Location $output
try {
    $test=Start-Process -FilePath .\ParallaxSandbox.exe -ArgumentList '--self-test' -WorkingDirectory $output -WindowStyle Hidden -PassThru
    $test.WaitForExit()
    if($test.ExitCode -ne 0){throw 'Experimental world integrity checks failed'}
    $render=Start-Process -FilePath .\ParallaxSandbox.exe -ArgumentList '--smoke-test' -WorkingDirectory $output -WindowStyle Hidden -PassThru
    $render.WaitForExit()
    if($render.ExitCode -ne 0){throw 'Experimental render/menu checks failed'}
}finally{Pop-Location}
Write-Host "Experimental playable build: $(Join-Path $output 'ParallaxSandbox.exe')"
