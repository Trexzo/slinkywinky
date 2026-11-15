# R197: controlled local-only execution of the public compiled COFF manager
# with private linked PE .data records. Never load/inject the full DLL.
# Private pointer-bearing fixture must remain OUTSIDE the Git checkout.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$UpstreamRoot,
    [Parameter(Mandatory=$true)][string]$CorrectedBuild,
    [Parameter(Mandatory=$true)][string]$OriginalBuild,
    [string]$OutputRoot = (Join-Path $env:TEMP 'slinkywinky-r197-private-record-acceptance')
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$upstream=(Resolve-Path $UpstreamRoot).Path
$corrected=(Resolve-Path $CorrectedBuild).Path
$original=(Resolve-Path $OriginalBuild).Path
$relativeObject='CMakeFiles\slinky.dir\src\engine\jni\module_manager.cpp.obj'
$goodObj=Join-Path $corrected $relativeObject
$badObj=Join-Path $original $relativeObject
$inputs=@(
    @{Name='corrected DLL';Path=(Join-Path $corrected 'slinky.dll');Hash='3613a466433fb1fa83d2d9e96815ece719c166f881c78642dc18d3e13ae2aa14'},
    @{Name='corrected COFF';Path=$goodObj;Hash='632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae'},
    @{Name='original COFF';Path=$badObj;Hash='40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80'}
)
foreach($item in $inputs) {
    if(-not (Test-Path -LiteralPath $item.Path -PathType Leaf)) {
        throw "R197 missing pinned input: $($item.Name)"
    }
    $observed=(Get-FileHash -LiteralPath $item.Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if($observed -ne $item.Hash) {throw "R197 unexpected native fingerprint: $($item.Name)"}
}
$fullOutput=[IO.Path]::GetFullPath($OutputRoot)
if($fullOutput.StartsWith($repo,[StringComparison]::OrdinalIgnoreCase) -or
   $fullOutput.StartsWith($upstream,[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Refusing private native-record export inside a source checkout'
}
if(Test-Path -LiteralPath $fullOutput) {throw 'Refusing to overwrite R197 acceptance evidence folder'}
New-Item -ItemType Directory -Path $fullOutput | Out-Null
$report=Join-Path $fullOutput 'linked_descriptor_report.json'
$manifest=Join-Path $fullOutput 'linked_descriptor_metadata.txt'
$native=Join-Path $fullOutput 'private_linked_native_records.bin'
& python (Join-Path $repo 'tools\extract_r196_linked_descriptor_manifest.py') --upstream $upstream --dll (Join-Path $corrected 'slinky.dll') --map (Join-Path $corrected 'slinky.map') --json-out $report --fixture-out $manifest
if($LASTEXITCODE -ne 0) {throw 'R197 linked metadata export failed'}
& python (Join-Path $repo 'tools\export_r197_private_native_records.py') --upstream $upstream --dll (Join-Path $corrected 'slinky.dll') --map (Join-Path $corrected 'slinky.map') --private-out $native
if($LASTEXITCODE -ne 0) {throw 'R197 raw native records export failed'}
$expectedFixture='9b0904f558f9fd3ff3043f1dd381511d69bfdbb8738f85c6cff0a69a8abc3a04'
$observedFixture=(Get-FileHash -LiteralPath $native -Algorithm SHA256).Hash.ToLowerInvariant()
if($observedFixture -ne $expectedFixture) {throw 'R197 private record fixture SHA256 drift'}
Write-Output "R197_PINNED_RECORD_FIXTURE_SHA256=$observedFixture"

$source=Join-Path $repo 'reference_compat\compiled_manager_harness'
$good=Join-Path $fullOutput 'corrected'
& cmake -S $source -B $good "-DSLINKY_MANAGER_OBJECT=$goodObj" "-DSLINKY_DESCRIPTOR_FIXTURE=$manifest" "-DSLINKY_NATIVE_RECORD_FIXTURE=$native"
if($LASTEXITCODE -ne 0) {throw 'R197 corrected harness CMake configure failed'}
foreach($config in @('Debug','Release')) {
    & cmake --build $good --config $config --parallel 4
    if($LASTEXITCODE -ne 0) {throw "R197 corrected build failed $config"}
    & ctest --test-dir $good -C $config --output-on-failure
    if($LASTEXITCODE -ne 0) {throw "R197 compiled native-record tests failed $config"}
    Write-Output "R197_ACTUAL_COFF_$($config)_3_OF_3_PASS"
}

$bad=Join-Path $fullOutput 'original-negative'
& cmake -S $source -B $bad "-DSLINKY_MANAGER_OBJECT=$badObj"
if($LASTEXITCODE -ne 0) {throw 'R197 unmodified negative-control configure failed'}
function Invoke-Negative([string]$exe,[string]$record,[string]$manifestPath,[string]$name,[string]$expected) {
    $stdoutPath=Join-Path $fullOutput "$name.stdout.txt"
    $stderrPath=Join-Path $fullOutput "$name.stderr.txt"
    $arguments=@('--native-records',('"'+$record+'"'),('"'+$manifestPath+'"'))
    $process=Start-Process -FilePath $exe -ArgumentList $arguments -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdoutPath -RedirectStandardError $stderrPath
    $diagnostic=Get-Content -LiteralPath $stderrPath -Raw
    if($process.ExitCode -ne 2 -or -not $diagnostic.Contains($expected)) {
        throw "R197 negative control did not reject correctly: $name exit=$($process.ExitCode)"
    }
    Write-Output "R197_NEGATIVE_$($name)_PASS"
}
foreach($config in @('Debug','Release')) {
    & cmake --build $bad --config $config --parallel 4
    if($LASTEXITCODE -ne 0) {throw "R197 unmodified COFF build failed $config"}
    Invoke-Negative (Join-Path $bad "$config\slinkywinky_r195_manager_acceptance.exe") $native $manifest "original-$config" 'real-metadata manager count not 66'
}

$correctedExe=Join-Path $good 'Release\slinkywinky_r195_manager_acceptance.exe'
$negativeDir=Join-Path $fullOutput 'corruption-controls'
New-Item -ItemType Directory -Path $negativeDir | Out-Null
$originalBytes=[IO.File]::ReadAllBytes($native)
$meta=Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
$firstEvent=[int]$meta.entries[0].handlers[0].event
$base=48+4
$mutations=@(
    @{Name='bad-header';Kind='flip';Offset=0;Error='header or DLL fingerprint invalid'},
    @{Name='wrong-source-index';Kind='flip';Offset=48;Error='source order changed'},
    @{Name='wrong-category';Kind='flip';Offset=($base+0x10);Error='category/default differs'},
    @{Name='wrong-priority';Kind='flip';Offset=($base+0x50+16*$firstEvent);Error='priority differs'},
    @{Name='missing-handler';Kind='zero8';Offset=($base+0x48+16*$firstEvent);Error='callback-presence mismatch'},
    @{Name='truncated';Kind='truncate';Offset=0;Error='truncated private linked native record bytes'},
    @{Name='extra-trailing-byte';Kind='append';Offset=0;Error='trailing bytes'}
)
foreach($mutation in $mutations) {
    $bytes=[byte[]]$originalBytes.Clone()
    switch($mutation.Kind) {
        'flip' {$bytes[$mutation.Offset]=$bytes[$mutation.Offset] -bxor 1}
        'zero8' {[Array]::Clear($bytes,$mutation.Offset,8)}
        'truncate' {$bytes=$bytes[0..($bytes.Length-2)]}
        'append' {$bytes += [byte]1}
        default {throw 'Unknown R197 private fixture mutation'}
    }
    $name=$mutation.Name
    $path=Join-Path $negativeDir "$name.bin"
    [IO.File]::WriteAllBytes($path,$bytes)
    Invoke-Negative $correctedExe $path $manifest $name $mutation.Error
}
Write-Output 'R197_ALL_7_CORRUPTION_CONTROLS_PASS'
Write-Output 'R197_PINNED_NATIVE_RECORD_RUNTIME_ACCEPTANCE_PASS'
