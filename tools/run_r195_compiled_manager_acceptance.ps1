# R195: first-party compiled-manager harness against local, pinned COFF objects.
# Never loads the full DLL or a Minecraft JVM. No native object is uploaded.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$CorrectedObject,
    [Parameter(Mandatory=$true)][string]$OriginalObject,
    [string]$OutputRoot = (Join-Path $env:TEMP 'slinkywinky-r195-manager-recheck')
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$expected = @{
    corrected = '632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae'
    original = '40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80'
}
foreach ($item in @(
    @{ Name='corrected'; Path=$CorrectedObject },
    @{ Name='original'; Path=$OriginalObject }
)) {
    $file = (Resolve-Path -LiteralPath $item.Path).Path
    if ((Split-Path -Leaf $file) -ne 'module_manager.cpp.obj') {
        throw "Unexpected native object filename: $file"
    }
    $actual = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $expected[$item.Name]) {
        throw "$($item.Name) native COFF fingerprint mismatch: $actual"
    }
    if ($item.Name -eq 'corrected') { $CorrectedObject = $file }
    if ($item.Name -eq 'original') { $OriginalObject = $file }
}
if (Test-Path -LiteralPath $OutputRoot) {
    throw "Refusing to overwrite existing isolated folder: $OutputRoot"
}
$source = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\reference_compat\compiled_manager_harness')).Path
New-Item -ItemType Directory -Path $OutputRoot | Out-Null

function Build-NativeHarness([string]$obj, [string]$path) {
    & cmake -S $source -B $path "-DSLINKY_MANAGER_OBJECT=$obj"
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $path" }
    foreach ($config in @('Debug', 'Release')) {
        & cmake --build $path --config $config --parallel 4
        if ($LASTEXITCODE -ne 0) { throw "Harness build failed: $config" }
    }
}
$good = Join-Path $OutputRoot 'corrected'
$bad = Join-Path $OutputRoot 'original-negative'
Build-NativeHarness $CorrectedObject $good
foreach ($config in @('Debug', 'Release')) {
    & ctest --test-dir $good -C $config --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Corrected manager CTest FAILED: $config" }
    Write-Output "R195_CORRECTED_$($config)_PASS"
}
Build-NativeHarness $OriginalObject $bad
foreach ($config in @('Debug', 'Release')) {
    $exe = Join-Path $bad "$config\slinkywinky_r195_manager_acceptance.exe"
    $out = Join-Path $OutputRoot "negative-$config.out.txt"
    $err = Join-Path $OutputRoot "negative-$config.err.txt"
    $proc = Start-Process -FilePath $exe -NoNewWindow -Wait -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
    $failure = Get-Content -LiteralPath $err -Raw
    if ($proc.ExitCode -ne 2 -or -not $failure.Contains('R195_COMPILED_MANAGER_FAIL compiled constructor is not 66-entry')) {
        throw "Original 67-entry negative control did not reject: $config (exit $($proc.ExitCode))"
    }
    Write-Output "R195_ORIGINAL_$($config)_NEGATIVE_CONTROL_PASS"
}
Write-Output "R195_ISOLATED_ACTUAL_COFF_ACCEPTANCE_PASS"
