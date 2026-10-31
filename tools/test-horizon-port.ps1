$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Push-Location $root
try {
    New-Item -ItemType Directory -Force .local\horizon-parity | Out-Null
    $lock=Get-Content horizon.lock.json -Raw | ConvertFrom-Json
    foreach($source in $lock.reference_files){
        $actualHash=git hash-object --no-filters (Join-Path 'third_party\horizon' $source.path)
        if($LASTEXITCODE -ne 0 -or $actualHash -ne $source.git_blob_sha){throw "Pinned upstream reference changed: $($source.path)"}
    }
    # Isolate unchanged arithmetic from its winit-specific quit-shortcut adapter.
    $upstream=[IO.File]::ReadAllText((Join-Path $root 'third_party\horizon\crates\horizon-client-input\src\input.rs'))
    $begin=$upstream.IndexOf('fn length(')
    $end=$upstream.IndexOf('/// `InputQuirks.isQuitShortcutDown`')
    if($begin -lt 0 -or $end -le $begin){throw 'Pinned input source no longer matches the extraction boundary'}
    [IO.File]::WriteAllText((Join-Path $root '.local\horizon-input-arithmetic.rs'),$upstream.Substring($begin,$end-$begin),[Text.UTF8Encoding]::new($false))
    rustc --edition=2024 -C opt-level=2 -C overflow-checks=off tools\horizon_input_probe.rs -o .local\horizon-parity\rust-probe.exe
    if($LASTEXITCODE -ne 0){throw 'Rust reference compilation failed'}
    cmake --build build --config Release --target ParallaxHorizonInputProbe
    if($LASTEXITCODE -ne 0){throw 'C translation compilation failed'}
    & .\.local\horizon-parity\rust-probe.exe | Set-Content -Encoding ascii .local\horizon-parity\rust.txt
    if($LASTEXITCODE -ne 0){throw 'Rust reference probe failed'}
    & .\build\Release\ParallaxHorizonInputProbe.exe | Set-Content -Encoding ascii .local\horizon-parity\c.txt
    if($LASTEXITCODE -ne 0){throw 'C translation probe failed'}
    $expected=[IO.File]::ReadAllLines((Join-Path $root '.local\horizon-parity\rust.txt'))
    $actual=[IO.File]::ReadAllLines((Join-Path $root '.local\horizon-parity\c.txt'))
    if($expected.Count -ne $actual.Count){throw 'Probe row counts differ'}
    for($i=0;$i -lt $expected.Count;$i++){if($expected[$i] -cne $actual[$i]){throw "C/Rust mismatch at row ${i}:`nRust: $($expected[$i])`nC:    $($actual[$i])"}}
    "PASS: $($expected.Count) C/Rust rows match exactly (floating-point bit patterns included)." | Tee-Object .local\horizon-parity\results.txt
}finally{Pop-Location}
