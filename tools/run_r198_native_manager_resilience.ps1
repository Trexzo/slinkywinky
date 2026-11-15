# R198: local-only native resilience acceptance for the pinned corrected manager
# COFF object. The public integration repo never stores third-party objects.
# No Minecraft process, JVM, full DLL load or injector is involved.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$CorrectedObject,
    [Parameter(Mandatory=$true)][string]$OriginalObject,
    [string]$OutputRoot = (Join-Path $env:TEMP 'slinkywinky-r198-manager-resilience')
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$expected = @{
    corrected = '632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae'
    original = '40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80'
}
foreach ($entry in @(
    @{Name='corrected';Path=$CorrectedObject},
    @{Name='original';Path=$OriginalObject}
)) {
    $obj=(Resolve-Path -LiteralPath $entry.Path).Path
    if ((Split-Path -Leaf $obj) -ne 'module_manager.cpp.obj') {
        throw "R198 unexpected COFF object filename"
    }
    $digest=(Get-FileHash -LiteralPath $obj -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($digest -ne $expected[$entry.Name]) {
        throw "R198 pinned $($entry.Name) compiled object changed"
    }
    if ($entry.Name -eq 'corrected') { $CorrectedObject=$obj }
    if ($entry.Name -eq 'original') { $OriginalObject=$obj }
}
if (Test-Path -LiteralPath $OutputRoot) {
    throw "R198 refuses to overwrite an existing acceptance directory"
}
$repo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$source=Join-Path $repo 'reference_compat\compiled_manager_harness'
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$good=Join-Path $OutputRoot 'corrected'
$bad=Join-Path $OutputRoot 'original-negative'

function Build-Harness([string]$obj,[string]$dir) {
    & cmake -S $source -B $dir "-DSLINKY_MANAGER_OBJECT=$obj"
    if ($LASTEXITCODE -ne 0) { throw "R198 CMake configure failed" }
    foreach ($cfg in @('Debug','Release')) {
        & cmake --build $dir --config $cfg --parallel 4
        if ($LASTEXITCODE -ne 0) { throw "R198 COFF harness compilation failed: $cfg" }
    }
}

Build-Harness $CorrectedObject $good
foreach ($cfg in @('Debug','Release')) {
    & ctest --test-dir $good -C $cfg -R '^r198_' --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "R198 corrected native resilience test failure: $cfg" }
    Write-Output "R198_CORRECTED_$($cfg)_2_OF_2_PASS"
}
Build-Harness $OriginalObject $bad
foreach ($cfg in @('Debug','Release')) {
    $exe=Join-Path $bad "$cfg\slinkywinky_r195_manager_acceptance.exe"
    foreach ($item in @(
        @{Name='allocator';Flag='--allocator-fault-matrix';Text='R198 baseline is not the corrected manager'},
        @{Name='lifecycle';Flag='--lifecycle-cycles';Text='R198 repeated constructor manager layout drift'}
    )) {
        $stdout=Join-Path $OutputRoot "$cfg-$($item.Name).stdout.txt"
        $stderr=Join-Path $OutputRoot "$cfg-$($item.Name).stderr.txt"
        $proc=Start-Process -FilePath $exe -ArgumentList $item.Flag -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        $err=Get-Content -LiteralPath $stderr -Raw
        if ($proc.ExitCode -ne 2 -or -not $err.Contains($item.Text)) {
            throw "R198 original 67-entry negative control failed: $cfg / $($item.Name)"
        }
        Write-Output "R198_ORIGINAL_$($cfg)_$($item.Name)_EXPECTED_REJECTION_PASS"
    }
}
Write-Output 'R198_COMPILED_NATIVE_MANAGER_RESILIENCE_ACCEPTANCE_PASS'
