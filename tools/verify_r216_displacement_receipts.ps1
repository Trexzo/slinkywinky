# R216: independent receipt verification of real horizontal player
# displacement under three native No Slow states. TEMP singleplayer only.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][int]$TestProcessId,
 [switch]$StopTestClient
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expected='a4ef40b697019fdc209e31ee7b6e6453eb6326e952add4533033d05ad70ae08d'
$instance=(Resolve-Path -LiteralPath $InstanceRoot).Path
$normalized=$instance.Replace('\','/')
$temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).Replace('\','/')
if(-not $normalized.StartsWith($temp,[StringComparison]::OrdinalIgnoreCase) -or
   $normalized -notmatch '/slinkywinky-r216-independent-forge-[^/]+$'){
    throw 'R216 verifier refuses non-isolated TEMP game directory'
}
$jar=Join-Path $instance 'mods/slinkywinky-preview-r207-reobf.jar'
$log=Join-Path $instance 'r207-forge-stdout.log'
$stderr=Join-Path $instance 'r207-forge-stderr.log'
$preflight=Join-Path $instance 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
$world=Join-Path $instance 'saves/R216-local-only/level.dat'
foreach($file in @($jar,$log,$preflight,$world)){
    if(-not(Test-Path -LiteralPath $file)){
        throw "R216 missing required acceptance artifact: $file"
    }
}
$hash=(Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if($hash -ne $expected){throw 'R216 JAR fingerprint mismatch'}
$pre=Get-Content -LiteralPath $preflight -Raw
if(-not $pre.Contains("REOBF_SHA256=$expected") -or
   -not $pre.Contains('R216_DISPLACEMENT_TEST=True')){
    throw 'R216 wrong hash-pinned independent launcher mode'
}
$raw=Get-Content -LiteralPath $log -Raw
$ordered=@(
 'R214_LOCAL_AGENT_ARMED',
 'R214_POST_SLOW_PRE_CONSUME_ASM_PATCH_PASS',
 'R216_DISPLACEMENT_ISOLATION_GATE_PASS',
 'JNI_STARTUP_PROBE_PASS',
 'Forge Mod Loader has successfully loaded 4 mods',
 'R216_LOCAL_WORLD_REQUESTED',
 'R216Offline joined the game',
 'R216_REAL_MOVEMENT_INPUT_ATTACHED',
 'R214_REAL_ENTITYPLAYER_ONLIVINGUPDATE_HOOK_REACHED',
 'R216_GROUNDED_MEASUREMENT_STARTED',
 'R216_PHASE_DISPLACEMENT phase=enabled',
 'R216_PHASE_DISPLACEMENT phase=disabled',
 'R216_PHASE_DISPLACEMENT phase=reenabled',
 'R216_REAL_WORLD_DISPLACEMENT_ACCEPTANCE_PASS',
 'R216_DISPLACEMENT_AND_ACTION_CLEANUP_PASS'
)
$last=-1
foreach($marker in $ordered){
    $offset=$raw.IndexOf($marker,[StringComparison]::Ordinal)
    if($offset -le $last){throw "R216 missing/out-of-order evidence: $marker"}
    $last=$offset
    $count=$raw.Split([string[]]@($marker),[StringSplitOptions]::None).Length-1
    if($count -ne 1){throw "R216 duplicate/missing evidence: $marker count=$count"}
}
if($raw.Contains('R214_NO_SLOW_ACTION_FAIL') -or
   $raw.Contains('R214_TRANSFORM_REFUSED') -or
   $raw.Contains('R216 player left the ground')){
    throw 'R216 native action, exact transformation or ground test failed'
}
$pattern='R216_PHASE_DISPLACEMENT phase=(enabled|disabled|reenabled) samples=(\d+) horizontalBlocks=([0-9.]+) meanBlocksPerTick=([0-9.]+) moduleWrites=(\d+)'
$matches=[regex]::Matches($raw,$pattern)
if($matches.Count -ne 3){throw 'R216 expected exactly three actual game displacement phases'}
$phaseOrder=@('enabled','disabled','reenabled')
$means=New-Object 'System.Collections.Generic.List[double]'
$distances=New-Object 'System.Collections.Generic.List[double]'
for($i=0;$i -lt 3;$i++){
    $m=$matches[$i]
    if($m.Groups[1].Value -ne $phaseOrder[$i] -or
       [int]::Parse($m.Groups[2].Value) -ne 33){
        throw "R216 invalid module state or measurement sample count at phase $i"
    }
    $distance=[double]::Parse($m.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture)
    $mean=[double]::Parse($m.Groups[4].Value,[Globalization.CultureInfo]::InvariantCulture)
    if([double]::IsNaN($distance) -or [double]::IsInfinity($distance) -or
       [double]::IsNaN($mean) -or [double]::IsInfinity($mean) -or
       $distance -le 0 -or $mean -le 0 -or
       [Math]::Abs($distance / 33.0 - $mean) -gt 0.000001){
        throw "R216 inconsistent position-derived horizontal displacement phase $i"
    }
    $means.Add($mean)
    $distances.Add($distance)
}
$ratioEnabled=$means[0]/$means[1]
$ratioReenabled=$means[2]/$means[1]
$repeat=$means[2]/$means[0]
if($ratioEnabled -lt 1.75 -or $ratioReenabled -lt 1.75 -or
   $repeat -lt 0.55 -or $repeat -gt 1.45){
    throw 'R216 actual movement difference not reproducible across toggles'
}
$final='R216_REAL_WORLD_DISPLACEMENT_ACCEPTANCE_PASS samplesPerPhase=(\d+) callbacks=(\d+) ownedFieldWrites=(\d+) enabledMean=([0-9.]+) disabledMean=([0-9.]+) reenabledMean=([0-9.]+) enabledToDisabledRatio=([0-9.]+) reenabledToDisabledRatio=([0-9.]+) repeatRatio=([0-9.]+) inputClass=MovementInputFromOptions source=synthetic_local_keybind'
if($raw -notmatch $final){throw 'R216 final native movement decision receipt malformed'}
$callbackCount=[int]$Matches[2]
$writes=[int]$Matches[3]
if([int]$Matches[1] -ne 33 -or $callbackCount -lt 135 -or
   $callbackCount -gt 190 -or $writes -lt 90 -or $writes -gt 140){
    throw 'R216 invalid native callback or field-write counts'
}
for($i=0;$i -lt 3;$i++){
    $fromLog=[double]::Parse($Matches[$i+4],[Globalization.CultureInfo]::InvariantCulture)
    if([Math]::Abs($fromLog-$means[$i]) -gt 0.000001){
        throw 'R216 final movement means disagree with individual phases'
    }
}
foreach($pair in @(@($Matches[7],$ratioEnabled),@($Matches[8],$ratioReenabled),
                   @($Matches[9],$repeat))){
    if([Math]::Abs(([double]::Parse($pair[0],[Globalization.CultureInfo]::InvariantCulture))-$pair[1]) -gt 0.00001){
        throw 'R216 final speed ratio differs from independently recomputed ratio'
    }
}
$proc=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId"
if(-not $proc){throw 'R216 original isolated Java 8 test JVM missing'}
$cmdline=[string]$proc.CommandLine
if($proc.ExecutablePath -notmatch 'jdk-8' -or
   $cmdline -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $cmdline -notmatch 'R216Offline' -or
   $cmdline -notmatch 'slinkywinky[.]r216DisplacementTest=true' -or
   $cmdline -notmatch 'slinkywinky[.]r214NoSlowAgent=true' -or
   $cmdline -notmatch 'javaagent:' -or
   -not $cmdline.Contains($instance) -or
   $cmdline -match 'GradleMain|experimental[/\\]forge189[/\\]build[/\\]classes'){
    throw 'R216 process is not exact standalone test JVM'
}
$stderrBytes=if(Test-Path -LiteralPath $stderr){(Get-Item $stderr).Length}else{-1}
$state='RUNNING_AT_ACCEPTANCE'
if($StopTestClient){
    $p=Get-Process -Id $TestProcessId -ErrorAction Stop
    if($p.MainWindowHandle -ne [IntPtr]::Zero){
        [void]$p.CloseMainWindow()
        Start-Sleep -Seconds 4
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        Stop-Process -Id $TestProcessId -Force -ErrorAction Stop
        Start-Sleep -Seconds 1
        $state='VERIFIED_TEST_JVM_STOPPED_AFTER_WINDOW_CLOSE'
    }else{
        $state='GRACEFUL_EXIT'
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        throw 'R216 exact verified test JVM still running'
    }
}
$receipt=Join-Path $instance 'R216_REAL_DISPLACEMENT_ACCEPTANCE.txt'
@(
 'R216_REAL_WORLD_MOVEMENT_DISPLACEMENT_NATIVE_ACTION_PASS'
 "JAR_SHA256=$expected"
 "NATIVE_CALLBACKS=$callbackCount"
 "OWNED_INPUT_WRITES=$writes"
 'MEASURED_TICKS_PER_PHASE=33'
 "ENABLED_HORIZONTAL_BLOCKS=$($distances[0])"
 "DISABLED_HORIZONTAL_BLOCKS=$($distances[1])"
 "REENABLED_HORIZONTAL_BLOCKS=$($distances[2])"
 "ENABLED_MEAN_BLOCKS_PER_TICK=$($means[0])"
 "DISABLED_MEAN_BLOCKS_PER_TICK=$($means[1])"
 "REENABLED_MEAN_BLOCKS_PER_TICK=$($means[2])"
 "ENABLED_TO_DISABLED_RATIO=$ratioEnabled"
 "REENABLED_TO_DISABLED_RATIO=$ratioReenabled"
 "REPEAT_RATIO=$repeat"
 'GAME=TEMP_ONLY_LOCAL_CREATIVE_SINGLEPLAYER'
 'INPUT=VANILLA_MOVEMENT_INPUT_TEST_ONLY_SYNTHETIC_KEYBIND'
 'PHYSICAL_KEYBOARD_ORIGINAL_CLIENT_PARITY=NOT_VERIFIED'
 "STDERR_BYTES_AT_ACCEPTANCE=$stderrBytes"
 "JVM_STATE=$state"
) | Set-Content -LiteralPath $receipt -Encoding ASCII
Write-Output "R216_REAL_WORLD_MOVEMENT_DISPLACEMENT_NATIVE_ACTION_PASS callbacks=$callbackCount writes=$writes enabledRatio=$ratioEnabled reenabledRatio=$ratioReenabled state=$state"
Write-Output "R216_LOCAL_RECEIPT=$receipt"
