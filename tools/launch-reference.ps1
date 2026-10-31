param(
    [string]$MinecraftDirectory="$env:APPDATA\.minecraft",
    [string]$JavaPath='',
    [switch]$Wait
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$reference=Join-Path $root '.local\reference-26.3'
New-Item -ItemType Directory -Force $reference,(Join-Path $reference 'tmp') | Out-Null
if(-not $JavaPath){
    $localDirectory=Join-Path ([IO.Directory]::GetParent($env:APPDATA).FullName) 'Local'
    $runtimeRoot=Join-Path $localDirectory 'Packages\Microsoft.4297127D64EC6_8wekyb3d8bbwe\LocalCache\Local\runtime\java-runtime-epsilon'
    $JavaPath=Get-ChildItem -LiteralPath $runtimeRoot -Filter java.exe -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
}
if(-not $JavaPath -or -not(Test-Path -LiteralPath $JavaPath)){throw 'Supply -JavaPath with the Java 25 runtime from your installation.'}
$manifest=Get-Content -LiteralPath (Join-Path $MinecraftDirectory 'versions\26.3\26.3.json') -Raw | ConvertFrom-Json
$classpath=[Collections.Generic.List[string]]::new()
foreach($library in $manifest.libraries){
    $allow=-not [bool]$library.rules
    foreach($rule in $library.rules){
        $matches=$true
        if($rule.os -and $rule.os.name -ne 'windows'){$matches=$false}
        if($rule.features){$matches=$false}
        if($matches){$allow=$rule.action -eq 'allow'}
    }
    if($allow -and $library.downloads.artifact.path){
        $file=Join-Path $MinecraftDirectory "libraries\$($library.downloads.artifact.path)"
        if(-not(Test-Path -LiteralPath $file)){throw "Missing reference library: $($library.name)"}
        $classpath.Add($file)
    }
}
$classpath.Add((Join-Path $MinecraftDirectory 'versions\26.3\26.3.jar'))
$arguments=@('-Xmx2G',"-Djava.io.tmpdir=$(Join-Path $reference 'tmp')",'--enable-native-access=ALL-UNNAMED','-cp',($classpath -join ';'),$manifest.mainClass,'--username','ParallaxReference','--version','26.3','--gameDir',$reference,'--assetsDir',(Join-Path $MinecraftDirectory 'assets'),'--assetIndex',$manifest.assetIndex.id,'--uuid','00000000000000000000000000000001','--accessToken','0','--userType','msa','--versionType','release','--width','1440','--height','900')
# Offline local reference only. No account files, password or tokens are read.
$quoted=@($arguments | ForEach-Object {'"'+$_.Replace('"','\"')+'"'})
$process=Start-Process -FilePath $JavaPath -ArgumentList ($quoted -join ' ') -WorkingDirectory $reference -WindowStyle Hidden -RedirectStandardOutput (Join-Path $reference 'stdout.log') -RedirectStandardError (Join-Path $reference 'stderr.log') -PassThru
Write-Output "Reference process: $($process.Id). Isolated game directory: $reference"
if($Wait){$process.WaitForExit();exit $process.ExitCode}
