# R213: source-owned verifier for the exact isolated Minecraft 1.8.9
# singleplayer cooldown field-write trial. Never trusts the log alone:
# checks the JAR digest, named TEMP gameDir and live JVM identity.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][int]$TestProcessId,
 [switch]$StopTestClient
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expected='45cfc71bd182c05393f79b718e5a7c330cf23ad2bdb137df69586c9921774bd2'
$instance=(Resolve-Path -LiteralPath $InstanceRoot).Path
$normalized=$instance.Replace('\','/')
$temporary=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).Replace('\','/')
if(-not $normalized.StartsWith($temporary,[StringComparison]::OrdinalIgnoreCase) -or
   $normalized -notmatch '/slinkywinky-r213-independent-forge-[^/]+$'){
    throw 'R213 refuses non-isolated TEMP profile'
}
$jar=Join-Path $instance 'mods/slinkywinky-preview-r207-reobf.jar'
$log=Join-Path $instance 'r207-forge-stdout.log'
$stderr=Join-Path $instance 'r207-forge-stderr.log'
$preflight=Join-Path $instance 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
$world=Join-Path $instance 'saves/R213-local-only/level.dat'
foreach($file in @($jar,$log,$preflight,$world)){
    if(-not (Test-Path -LiteralPath $file)){
        throw "R213 missing in-game evidence: $file"
    }
}
$sha=(Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if($sha -ne $expected){throw 'R213 test mod JAR digest mismatch'}
$pre=Get-Content -LiteralPath $preflight -Raw
if(-not $pre.Contains('R213_LOCAL_ACTION_ACCEPTANCE=True') -or
   -not $pre.Contains("REOBF_SHA256=$expected")){
    throw 'R213 independent Forge launcher preflight inconsistent'
}
$raw=Get-Content -LiteralPath $log -Raw
$markers=@(
    'Forge Mod Loader has successfully loaded 4 mods',
    '[SlinkyWinkyPreview] R213_ACTION_ISOLATION_GATE_PASS',
    '[SlinkyWinkyPreview] JNI_STARTUP_PROBE_PASS',
    '[SlinkyWinkyPreview] R213_LOCAL_WORLD_REQUESTED',
    'R213Offline joined the game',
    '[SlinkyWinkyPreview] R213_JUMP_ENABLED_ZERO_PASS',
    '[SlinkyWinkyPreview] R213_JUMP_DISABLED_NO_WRITE_PASS',
    '[SlinkyWinkyPreview] R213_JUMP_REENABLED_ZERO_PASS',
    '[SlinkyWinkyPreview] R213_LOCAL_JUMP_ACTION_ACCEPTANCE_PASS enabled_zero=1 disabled_preserved=1 reenabled_zero=1 no_slow_phase=blocked_late_tick'
)
foreach($marker in $markers){
    if(-not $raw.Contains($marker)){throw "R213 missing live field-effect marker: $marker"}
}
foreach($marker in @('R213_JUMP_ENABLED_ZERO_PASS','R213_JUMP_DISABLED_NO_WRITE_PASS',
                   'R213_JUMP_REENABLED_ZERO_PASS','R213_LOCAL_JUMP_ACTION_ACCEPTANCE_PASS')){
    if($raw.Split([string[]]@($marker),[StringSplitOptions]::None).Length -ne 2){
        throw "R213 duplicate/missing acceptance action marker: $marker"
    }
}
$process=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId"
if(-not $process){throw 'R213 isolated Minecraft test JVM unavailable'}
$cmd=[string]$process.CommandLine
if($process.ExecutablePath -notmatch 'jdk-8' -or
   $cmd -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $cmd -notmatch 'R213Offline' -or
   $cmd -notmatch 'slinkywinky[.]r213LocalActionAcceptance=true' -or
   $cmd -notmatch 'slinkywinky[.]sprintObservation=true' -or
   -not $cmd.Contains($instance) -or
   $cmd -match 'GradleMain|experimental[/\\]forge189[/\\]build[/\\]classes'){
    throw 'R213 refuses unrelated Java/process or development-classpath client'
}
$stderrBytes=if(Test-Path -LiteralPath $stderr){(Get-Item $stderr).Length}else{-1}
$state='RUNNING_AT_ACCEPTANCE'
if($StopTestClient) {
    $gp=Get-Process -Id $TestProcessId -ErrorAction Stop
    if($gp.MainWindowHandle -ne [IntPtr]::Zero){
        [void]$gp.CloseMainWindow()
        Start-Sleep -Seconds 5
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue) {
        # Only this already-verified dummy-account test JVM may be stopped.
        Stop-Process -Id $TestProcessId -Force -ErrorAction Stop
        Start-Sleep -Seconds 1
        $state='EXACT_TEST_JVM_STOPPED_AFTER_WINDOW_CLOSE'
    } else {
        $state='GRACEFUL_EXIT'
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        throw 'R213 test client failed to terminate'
    }
}
$receipt=Join-Path $instance 'R213_JUMP_ACTION_ACCEPTANCE.txt'
@(
 'R213_REAL_FORGE_JUMP_COOLDOWN_FIELD_ACTION_PASS'
 "REOBF_SHA256=$expected"
 'WORLD=SINGLEPLAYER_LOCAL_TEMP'
 'LOGIN=OFFLINE_DUMMY_R213'
 'JUMP_ENABLED_5_TO_0=PASS'
 'JUMP_DISABLED_5_REMAINS_5=PASS'
 'JUMP_REENABLED_5_TO_0=PASS'
 'NO_SLOW_LATE_PLAYER_TICK_WRITE=REFUSED'
 'JUMP_FIXTURE=ARTIFICIALLY_SET_COOLDOWN_5'
 'NATURAL_REPEATED_JUMP=NOT_YET_VERIFIED'
 'GAMEPLAY_PRECONSUMPTION_NO_SLOW=NOT_YET_IMPLEMENTED'
 "JVM_STDERR_BYTES_AT_ACCEPTANCE=$stderrBytes"
 "JVM_STATE=$state"
) | Set-Content -LiteralPath $receipt -Encoding ASCII
Write-Output "R213_REAL_FORGE_JUMP_COOLDOWN_FIELD_ACTION_PASS state=$state stderr_bytes=$stderrBytes"
Write-Output "R213_LOCAL_EVIDENCE=$receipt"
