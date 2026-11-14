# R199 native duplicate/reentrant toggle hazard + guarded-admission acceptance.
# Uses local pinned public OpenSlinky manager objects only. Never loads full
# DLL, invokes original gameplay callbacks, injects, or starts Minecraft.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$CorrectedObject,
    [Parameter(Mandatory=$true)][string]$OriginalObject,
    [string]$OutputRoot = (Join-Path $env:TEMP 'slinkywinky-r199-toggle-acceptance')
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$hashes=@{
    corrected='632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae'
    original='40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80'
}
foreach($item in @(
    @{Name='corrected';Path=$CorrectedObject},
    @{Name='original';Path=$OriginalObject}
)){
    $file=(Resolve-Path -LiteralPath $item.Path).Path
    if((Split-Path -Leaf $file) -ne 'module_manager.cpp.obj') {
        throw 'Unexpected R199 compiled native module manager object name'
    }
    $digest=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    if($digest -ne $hashes[$item.Name]) {
        throw "R199 pinned compiled object changed: $($item.Name)"
    }
    if($item.Name -eq 'corrected') {$CorrectedObject=$file}
    else {$OriginalObject=$file}
}
if(Test-Path -LiteralPath $OutputRoot) {
    throw 'Refusing to overwrite existing R199 native acceptance output'
}
$root=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$source=Join-Path $root 'reference_compat\compiled_manager_harness'
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$good=Join-Path $OutputRoot 'corrected'
$bad=Join-Path $OutputRoot 'original-negative'

function Build-Manager([string]$nativeObject, [string]$target) {
    & cmake -S $source -B $target "-DSLINKY_MANAGER_OBJECT=$nativeObject"
    if($LASTEXITCODE -ne 0) {throw 'R199 configure failed'}
    foreach($config in @('Debug','Release')) {
        & cmake --build $target --config $config --parallel 4
        if($LASTEXITCODE -ne 0) {throw "R199 build failed: $config"}
    }
}

Build-Manager $CorrectedObject $good
foreach($config in @('Debug','Release')){
    & ctest --test-dir $good -C $config -R '^r199_' --output-on-failure
    if($LASTEXITCODE -ne 0){throw "R199 actual compiled manager CTest failed: $config"}
    Write-Output "R199_CORRECTED_$($config)_2_OF_2_PASS"
}
Build-Manager $OriginalObject $bad
foreach($config in @('Debug','Release')){
    $exe=Join-Path $bad "$config\slinkywinky_r195_manager_acceptance.exe"
    foreach($control in @(
        @{Name='observation';Flag='--observe-native-toggle-hazards';Expected='R199 observation requires corrected manager'},
        @{Name='guarded';Flag='--guarded-toggle-acceptance';Expected='R199 guarded test requires corrected manager'}
    )){
        $stdout=Join-Path $OutputRoot "$config-$($control.Name).stdout"
        $stderr=Join-Path $OutputRoot "$config-$($control.Name).stderr"
        $proc=Start-Process -FilePath $exe -ArgumentList $control.Flag -PassThru -Wait -NoNewWindow -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        $diagnostic=Get-Content -LiteralPath $stderr -Raw
        if($proc.ExitCode -ne 2 -or -not $diagnostic.Contains($control.Expected)){
            throw "Original 67-entry negative control invalid: $config $($control.Name)"
        }
        Write-Output "R199_ORIGINAL_$($config)_$($control.Name)_EXPECTED_REJECTION_PASS"
    }
}
Write-Output 'R199_COMPILED_DUPLICATE_REENTRANT_TOGGLE_ACCEPTANCE_PASS'
