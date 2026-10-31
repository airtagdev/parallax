param([string]$JavaPath='', [string]$ReferenceTitle='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    cmake --build build --config Release
    if($LASTEXITCODE -ne 0){throw 'Build failed'}
    ctest --test-dir build -C Release --output-on-failure
    if($LASTEXITCODE -ne 0){throw 'C core/resource verification failed'}
    if($JavaPath){
        New-Item -ItemType Directory -Force .local\parity | Out-Null
        & $JavaPath tools\RandomOracle.java | Set-Content -Encoding utf8 .local\parity\java-random.tsv
        if($LASTEXITCODE -ne 0){throw 'Java reference harness failed'}
        & .\build\Release\ParallaxCoreTests.exe .local\parity\java-random.tsv
        if($LASTEXITCODE -ne 0){throw 'C and Java random streams differ'}
    }
    if($ReferenceTitle){
        $capture=Start-Process -FilePath .\release\Parallax.exe -ArgumentList '--capture-title' -WorkingDirectory $root -WindowStyle Hidden -PassThru
        $capture.WaitForExit()
        if($capture.ExitCode -ne 0){throw 'C title rendering failed'}
        & .\build\Release\ParallaxImageCompare.exe $ReferenceTitle rewrite-title.bmp | Tee-Object title-parity-results.txt
        if($LASTEXITCODE -ne 0){throw 'One or more of the seven pinned title-button regions differ'}
    }
} finally { Pop-Location }
