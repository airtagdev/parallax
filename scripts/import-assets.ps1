param(
    [string]$MinecraftDirectory = "$env:APPDATA\.minecraft",
    [string]$Version = '26.3',
    [string]$JarPath = ''
)
$ErrorActionPreference = 'Stop'
if ($Version -ne '26.3') { throw 'This rewrite is pinned to Minecraft Java Edition 26.3.' }
Add-Type -AssemblyName System.IO.Compression.FileSystem
if (-not $JarPath) { $JarPath = Join-Path $MinecraftDirectory "versions\$Version\$Version.jar" }
$root = Split-Path $PSScriptRoot -Parent
$destination = [IO.Path]::GetFullPath((Join-Path $root 'release\resources'))
New-Item -ItemType Directory -Force $destination | Out-Null
$jarHash = (Get-FileHash -LiteralPath $JarPath -Algorithm SHA256).Hash.ToLowerInvariant()
$archive = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $JarPath))
$counts = @{ assets=0; data=0; advancements=0; recipes=0; cache=0; missing_cache=0; excluded_code=0 }
$utf8 = [Text.UTF8Encoding]::new($false)
function Read-JarText([string]$path) {
    $entry = $archive.GetEntry($path)
    if (-not $entry) { throw "Missing required resource: $path" }
    $reader = [IO.StreamReader]::new($entry.Open(), [Text.Encoding]::UTF8)
    try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
}
try {
    $versionData = (Read-JarText 'version.json') | ConvertFrom-Json
    if ($versionData.id -ne '26.3') { throw 'The supplied JAR is not version 26.3.' }
    foreach ($entry in $archive.Entries) {
        $name = $entry.FullName
        if ($name.EndsWith('/')) { continue }
        if ($name -ne 'version.json' -and -not $name.StartsWith('assets/') -and -not $name.StartsWith('data/')) { continue }
        # Import resources and declarative data, never game classes or shader source.
        if ($name.StartsWith('assets/minecraft/shaders/') -or [IO.Path]::GetExtension($name) -notin @('.json','.png','.mcmeta','.txt','.ogg','.zip','.hex','.bin')) {
            $counts.excluded_code++; continue
        }
        $target = [IO.Path]::GetFullPath((Join-Path $destination $name))
        if (-not $target.StartsWith($destination + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw "Unsafe archive entry: $name" }
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,$target,$true)
        if($name.StartsWith('assets/')){$counts.assets++}
        if($name.StartsWith('data/')){$counts.data++}
        if($name -match '^data/minecraft/advancement/.+\.json$'){$counts.advancements++}
        if($name -match '^data/minecraft/recipe/.+\.json$'){$counts.recipes++}
    }
    $meta = Join-Path $destination 'meta'
    [IO.Directory]::CreateDirectory($meta) | Out-Null
    $font = (Read-JarText 'assets/minecraft/font/include/default.json') | ConvertFrom-Json
    $atlasLines = [Collections.Generic.List[string]]::new()
    $glyphLines = [Collections.Generic.List[string]]::new()
    $seen = [Collections.Generic.HashSet[int]]::new()
    $atlas = 0
    foreach($provider in $font.providers) {
        if($provider.type -ne 'bitmap'){continue}
        $height = if($provider.height){[int]$provider.height}else{8}
        $ascent = [int]$provider.ascent
        $columns = @($provider.chars[0].EnumerateRunes()).Count
        $file = $provider.file.Replace('minecraft:', 'assets/minecraft/textures/')
        $atlasLines.Add("$atlas`t$file`t$columns`t$($provider.chars.Count)`t$height`t$ascent")
        $cell = 0
        foreach($row in $provider.chars){
            foreach($rune in $row.EnumerateRunes()){
                if($rune.Value -ne 0 -and $seen.Add($rune.Value)){$glyphLines.Add("$($rune.Value)`t$atlas`t$cell")}
                $cell++
            }
        }
        $atlas++
    }
    [IO.File]::WriteAllLines((Join-Path $meta 'font-atlases.tsv'),$atlasLines,$utf8)
    [IO.File]::WriteAllLines((Join-Path $meta 'font-glyphs.tsv'),$glyphLines,$utf8)
} finally { $archive.Dispose() }

# Actual panoramas and sounds live in the local content-addressed asset cache.
$versionManifest = Join-Path $MinecraftDirectory "versions\26.3\26.3.json"
$indexId = ''
if(Test-Path -LiteralPath $versionManifest){
    $manifest = Get-Content -LiteralPath $versionManifest -Raw | ConvertFrom-Json
    $indexId = $manifest.assetIndex.id
    $indexPath = Join-Path $MinecraftDirectory "assets\indexes\$indexId.json"
    if(Test-Path -LiteralPath $indexPath){
        if((Get-FileHash -LiteralPath $indexPath -Algorithm SHA1).Hash -ine $manifest.assetIndex.sha1){throw 'Asset index checksum mismatch'}
        $index = Get-Content -LiteralPath $indexPath -Raw | ConvertFrom-Json -AsHashtable
        foreach($name in $index.objects.Keys){
            if(-not $name.StartsWith('minecraft/')){continue}
            if([IO.Path]::GetExtension($name) -notin @('.json','.png','.mcmeta','.txt','.ogg','.zip','.hex','.bin')){continue}
            if($name.StartsWith('minecraft/shaders/')){continue}
            $hash=$index.objects[$name].hash
            $source=Join-Path $MinecraftDirectory "assets\objects\$($hash.Substring(0,2))\$hash"
            if(-not(Test-Path -LiteralPath $source)){$counts.missing_cache++;continue}
            if((Get-FileHash -LiteralPath $source -Algorithm SHA1).Hash -ine $hash){throw "Cached asset checksum mismatch: $name"}
            $target=[IO.Path]::GetFullPath((Join-Path $destination "assets\$name"))
            if(-not $target.StartsWith($destination + '\',[StringComparison]::OrdinalIgnoreCase)){throw 'Unsafe asset index path'}
            [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
            Copy-Item -LiteralPath $source -Destination $target -Force
            $counts.cache++
        }
    }
}
$result = @{target='26.3'; jar_sha256=$jarHash; data_version=$versionData.world_version; protocol_version=$versionData.protocol_version; asset_index=$indexId; counts=$counts; imported_at=[DateTime]::UtcNow.ToString('o'); executable_game_code_imported=$false}
[IO.File]::WriteAllText((Join-Path $destination 'meta\import.json'),($result|ConvertTo-Json -Depth 5),$utf8)
$result | ConvertTo-Json -Depth 5
