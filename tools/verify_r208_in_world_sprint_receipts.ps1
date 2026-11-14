# R208: validate a genuine in-world Sprint Java8/JNI observation receipt from
# the independently launched, reobfuscated Forge 1.8.9 preview.
# No automation of player input, no multiplayer or production game writes.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][int]$TestProcessId,
 [switch]$StopTestClient
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'

$expectedHash='5084bfce416abc8527f7e0e61f104378e104496e0eee40d6828a2b37970a5c0f'
$instance=(Resolve-Path -LiteralPath $InstanceRoot).Path
$normal=$instance.Replace('\','/')
$temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).Replace('\','/')
if(-not $normal.StartsWith($temp,[StringComparison]::OrdinalIgnoreCase) -or
   $normal -notmatch '/slinkywinky-r208-independent-forge-[^/]+$') {
    throw 'R208 refuses non-isolated TEMP gameDir'
}
$jar=Join-Path $instance 'mods/slinkywinky-preview-r207-reobf.jar'
$log=Join-Path $instance 'r207-forge-stdout.log'
$stderr=Join-Path $instance 'r207-forge-stderr.log'
$preflight=Join-Path $instance 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
$world=Join-Path $instance 'saves/R208-local-only/level.dat'
foreach($path in @($jar,$log,$preflight,$world)) {
    if(-not(Test-Path -LiteralPath $path)) {
        throw "R208 missing acceptance input: $path"
    }
}
$digest=(Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if($digest -ne $expectedHash){throw 'R208 staged JAR SHA256 mismatch'}
$pre=Get-Content -LiteralPath $preflight -Raw
if($pre -notmatch 'ISOLATED_WORLD_TEST=True' -or
   $pre -notmatch [regex]::Escape("REOBF_SHA256=$expectedHash")){
    throw 'R208 launcher did not validate the local world-test gate'
}
$logText=Get-Content -LiteralPath $log -Raw
$markers=@(
 'Forge Mod Loader has identified 4 mods to load',
 'FMLFileResourcePack:SlinkyWinky Preview',
 '[SlinkyWinkyPreview] R208_ISOLATION_GATE_PASS',
 '[SlinkyWinkyPreview] JNI_STARTUP_PROBE_PASS',
 '[SlinkyWinkyPreview] R208_LOCAL_WORLD_REQUESTED',
 'Loading dimension 0 (R208 local diagnostic)',
 'R208Offline joined the game',
 '[SlinkyWinkyPreview] R208_LOCAL_WORLD_PLAYER_READY_PASS',
 '[SlinkyWinkyPreview] R208_IN_WORLD_SPRINT_JNI_OBSERVATION_PASS'
)
foreach($marker in $markers) {
    if(-not $logText.Contains($marker)){
        throw "R208 missing genuine Forge world/JNI receipt: $marker"
    }
}
if($logText -notmatch 'R208_IN_WORLD_SPRINT_JNI_OBSERVATION_PASS renderSamples=(\d+) clientTicks=(\d+) elapsedMs=(\d+) latestKeyProposal=(true|false) latestSpeedProposal=(true|false)') {
    throw 'R208 in-world receipt has invalid structure'
}
$render=[int]$Matches[1]
$ticks=[int]$Matches[2]
$elapsed=[int]$Matches[3]
$key=[string]$Matches[4]
$speed=[string]$Matches[5]
if($render -lt 32 -or $ticks -lt 2 -or $elapsed -lt 1 -or
   $elapsed -gt 60000 -or
   $render -gt 1000) {
    throw 'R208 in-world sampled event counters are implausible'
}
if($logText.Split([string[]]@('R208_LOCAL_WORLD_REQUESTED'),
                  [StringSplitOptions]::None).Length -ne 2) {
    throw 'R208 unexpected duplicate local-world creation'
}
$serverJoin=$logText.Contains('Netty Local Client IO')
if(-not $serverJoin){throw 'R208 isolated test did not establish local singleplayer connection'}
$process=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId"
if(-not $process){throw 'R208 test JVM unavailable for provenance checking'}
$cmdline=[string]$process.CommandLine
if($process.ExecutablePath -notmatch 'jdk-8' -or
   -not $cmdline.Contains($instance) -or
   $cmdline -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $cmdline -notmatch 'R208Offline' -or
   $cmdline -notmatch 'slinkywinky[.]r208LocalWorldTest=true' -or
   $cmdline -notmatch 'slinkywinky[.]sprintObservation=true' -or
   $cmdline -match 'GradleMain|experimental[/\\]forge189[/\\]build[/\\]classes') {
    throw 'R208 not the expected independent read-only JNI Forge test process'
}
$stderrBytes=if(Test-Path $stderr){(Get-Item $stderr).Length}else{-1}
$state='RUNNING_AT_ACCEPTANCE'
if($StopTestClient) {
    $gp=Get-Process -Id $TestProcessId -ErrorAction Stop
    if($gp.MainWindowHandle -ne [IntPtr]::Zero) {
        [void]$gp.CloseMainWindow()
        Start-Sleep -Seconds 5
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue) {
        # Stop only the already verified isolated, offline Java8 test PID.
        Stop-Process -Id $TestProcessId -Force -ErrorAction Stop
        Start-Sleep -Seconds 1
        $state='VERIFIED_TEST_PID_STOPPED_AFTER_WINDOW_CLOSE'
    }else{
        $state='GRACEFUL_EXIT'
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue) {
        throw 'R208 exact isolated client did not shut down'
    }
}
$record=Join-Path $instance 'R208_IN_WORLD_SPRINT_ACCEPTANCE.txt'
@(
 'R208_REAL_WORLD_SPRINT_JNI_OBSERVATION_PASS'
 "REOBF_SHA256=$expectedHash"
 "IN_WORLD_RENDER_SAMPLES=$render"
 "IN_WORLD_CLIENT_TICKS=$ticks"
 "OBSERVATION_ELAPSED_MS=$elapsed"
 "LATEST_KEY_PROPOSAL=$key"
 "LATEST_SPEED_PROPOSAL=$speed"
 "SINGLEPLAYER_LOCAL_CONNECTION=$serverJoin"
 "JVM_STDERR_BYTES=$stderrBytes"
 'EXTERNAL_GAME_SERVER_CONNECTION=NOT_REQUESTED'
 'PLAYER_INPUT_OR_MOVEMENT_MUTATION=NONE_BY_PREVIEW'
 'NATIVE_EVENT17_21_PARITY=NOT_VERIFIED'
 'SPRINT_GAMEPLAY_ACTIONS=NOT_EXECUTED'
 "TEST_JVM_STATE=$state"
) | Set-Content -LiteralPath $record -Encoding ASCII
Write-Output "R208_REAL_WORLD_SPRINT_JNI_OBSERVATION_PASS render=$render ticks=$ticks elapsedMs=$elapsed state=$state"
Write-Output "R208_RECEIPT=$record"
