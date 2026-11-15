# R214: independently verify exact Minecraft 1.8.9 local No Slow
# pre-consumption field writes and enabled/disabled/re-enabled native state.
# Reads one fresh TEMP-only game; never accesses normal user game data.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][int]$TestProcessId,
 [switch]$StopTestClient
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expected='aa7bb470c3c5996ed08fa16d8962abe4aed0e8ae3ca07c5909351904452d78d8'
$instance=(Resolve-Path -LiteralPath $InstanceRoot).Path
$normalized=$instance.Replace('\','/')
$temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).Replace('\','/')
if(-not $normalized.StartsWith($temp,[StringComparison]::OrdinalIgnoreCase) -or
   $normalized -notmatch '/slinkywinky-r214-independent-forge-[^/]+$'){
    throw 'R214 refuses non-isolated gameDir'
}
$jar=Join-Path $instance 'mods/slinkywinky-preview-r207-reobf.jar'
$log=Join-Path $instance 'r207-forge-stdout.log'
$stderr=Join-Path $instance 'r207-forge-stderr.log'
$pre=Join-Path $instance 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
$world=Join-Path $instance 'saves/R214-local-only/level.dat'
foreach($file in @($jar,$log,$pre,$world)){
    if(-not(Test-Path -LiteralPath $file)){throw "R214 missing evidence $file"}
}
$sha=(Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if($sha -ne $expected){throw 'R214 reobfuscated test JAR fingerprint mismatch'}
$launchEvidence=Get-Content -LiteralPath $pre -Raw
if(-not $launchEvidence.Contains("REOBF_SHA256=$expected") -or
   -not $launchEvidence.Contains('R214_EXPLICIT_POST_SLOW_AGENT=True')){
    throw 'R214 no pinned TEMP Javaagent launch evidence'
}
$raw=Get-Content -LiteralPath $log -Raw
$ordered=@(
 'R214_LOCAL_AGENT_ARMED',
 'R214_POST_SLOW_PRE_CONSUME_ASM_PATCH_PASS',
 'R214_ACTION_ISOLATION_GATE_PASS',
 'JNI_STARTUP_PROBE_PASS',
 'Forge Mod Loader has successfully loaded 4 mods',
 'R214_LOCAL_WORLD_REQUESTED',
 'R214Offline joined the game',
 'R214_LOCAL_INPUT_FIXTURE_ARMED',
 'R214_REAL_ENTITYPLAYER_ONLIVINGUPDATE_HOOK_REACHED',
 'R214_NO_SLOW_ENABLED_INPUT_0P2_TO_1P0_PASS',
 'R214_NO_SLOW_DISABLED_INPUT_REMAINS_0P2_PASS',
 'R214_NO_SLOW_REENABLED_INPUT_0P2_TO_1P0_PASS',
 'R214_IN_WORLD_NO_SLOW_PRECONSUME_PASS',
 'R214_INPUT_FIXTURE_AND_ACTION_CLEANUP_PASS'
)
$last=-1
foreach($marker in $ordered){
    $pos=$raw.IndexOf($marker,[StringComparison]::Ordinal)
    if($pos -le $last){throw "R214 marker missing or out of order: $marker"}
    $last=$pos
    $occurrences=$raw.Split([string[]]@($marker),
        [StringSplitOptions]::None).Length-1
    if($occurrences -ne 1){
        throw "R214 duplicated/missing marker: $marker count=$occurrences"
    }
}
if($raw.Contains('R214_NO_SLOW_ACTION_FAIL') -or
   $raw.Contains('R214_TRANSFORM_REFUSED')){
    throw 'R214 accepted despite native input or exact ASM failure'
}
$pattern='R214_IN_WORLD_NO_SLOW_PRECONSUME_PASS callbacks=(\d+) verifiedFieldWrites=(\d+) phases=enabled_disabled_reenabled beforeForward=([0-9.]+) afterForward=([0-9.]+) itemId=(\d+) elapsedMs=(\d+)'
if($raw -notmatch $pattern){throw 'R214 final in-world field write receipt malformed'}
$callbacks=[int]$Matches[1]
$writes=[int]$Matches[2]
$before=[double]::Parse($Matches[3],[Globalization.CultureInfo]::InvariantCulture)
$after=[double]::Parse($Matches[4],[Globalization.CultureInfo]::InvariantCulture)
$item=[int]$Matches[5]
$elapsed=[int]$Matches[6]
if($callbacks -ne 30 -or $writes -ne 20 -or
   [Math]::Abs($before-0.2) -gt 0.0001 -or
   [Math]::Abs($after-1.0) -gt 0.0001 -or
   $item -ne 261 -or $elapsed -lt 1 -or $elapsed -gt 60000){
    throw 'R214 invalid real hook/field/action count or item value'
}
$proc=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId"
if(-not $proc){throw 'R214 exact original Java8 test process missing'}
$cmdline=[string]$proc.CommandLine
if($proc.ExecutablePath -notmatch 'jdk-8' -or
   $cmdline -notmatch 'R214Offline' -or
   $cmdline -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $cmdline -notmatch 'slinkywinky[.]r214LocalNoSlowAcceptance=true' -or
   $cmdline -notmatch 'slinkywinky[.]r214NoSlowAgent=true' -or
   $cmdline -notmatch 'javaagent:' -or
   -not $cmdline.Contains($instance) -or
   $cmdline -match 'GradleMain|experimental[/\\]forge189[/\\]build[/\\]classes'){
    throw 'R214 test process did not match standalone local Forge Javaagent'
}
$stderrBytes=if(Test-Path -LiteralPath $stderr){(Get-Item $stderr).Length}else{-1}
$state='RUNNING_AT_ACCEPTANCE'
if($StopTestClient) {
    $p=Get-Process -Id $TestProcessId -ErrorAction Stop
    if($p.MainWindowHandle -ne [IntPtr]::Zero){
        [void]$p.CloseMainWindow()
        Start-Sleep -Seconds 4
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        # Only this identity-checked isolated, credential-free JVM.
        Stop-Process -Id $TestProcessId -Force -ErrorAction Stop
        Start-Sleep -Seconds 1
        $state='VERIFIED_TEST_PID_STOPPED_AFTER_WINDOW_CLOSE'
    }else{
        $state='GRACEFUL_EXIT'
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        throw 'R214 verified test JVM did not stop'
    }
}
$record=Join-Path $instance 'R214_NO_SLOW_ACTION_ACCEPTANCE.txt'
@(
 'R214_REAL_PRECONSUMPTION_NO_SLOW_NATIVE_ACTION_PASS'
 "REOBF_SHA256=$expected"
 "CALLBACKS=$callbacks"
 "VERIFIED_FIELD_WRITES=$writes"
 "BEFORE_VANILLA_SLOW_INPUT=$before"
 "AFTER_NATIVE_NO_SLOW_INPUT=$after"
 'ENABLED_10_OF_10=PASS'
 'DISABLED_10_OF_10=PASS'
 'REENABLED_10_OF_10=PASS'
 'HOOK=AFTER_VANILLA_0P2_BEFORE_INPUT_CONSUMPTION'
 'GAME=LOCAL_TEMP_SINGLEPLAYER_ONLY'
 'INPUT=DETERMINISTIC_TEST_ONLY_NOT_PHYSICAL_KEY'
 'PHYSICAL_MULTIPLAYER_ORIGINAL_PARITY=NOT_VERIFIED'
 'NO_SLOW_ACTION_FAILURES=ZERO'
 "STDERR_BYTES_AT_ACCEPTANCE=$stderrBytes"
 "JVM_STATE=$state"
) | Set-Content -LiteralPath $record -Encoding ASCII
Write-Output "R214_REAL_PRECONSUMPTION_NO_SLOW_NATIVE_ACTION_PASS callbacks=$callbacks writes=$writes elapsed=$elapsed state=$state"
Write-Output "R214_LOCAL_EVIDENCE=$record"
