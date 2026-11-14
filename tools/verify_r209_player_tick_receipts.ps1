# R209: read-only acceptance of genuine Forge client-player-tick JNI samples.
# This accepts only a fresh isolated TEMP world and the exact R209 test JAR.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][int]$TestProcessId,
 [switch]$StopTestClient
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expected='27b7f8e707f90b3e4a5f5d56a7d7aa1e02aab55829c46fad494eb8b3771f8de2'
$instance=(Resolve-Path -LiteralPath $InstanceRoot).Path
$normalized=$instance.Replace('\','/')
$temporary=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).Replace('\','/')
if (-not $normalized.StartsWith($temporary,[StringComparison]::OrdinalIgnoreCase) -or
    $normalized -notmatch '/slinkywinky-r209-independent-forge-[^/]+$') {
  throw 'R209 refuses to inspect or stop a non-isolated TEMP game'
}
$jar=Join-Path $instance 'mods/slinkywinky-preview-r207-reobf.jar'
$log=Join-Path $instance 'r207-forge-stdout.log'
$stderr=Join-Path $instance 'r207-forge-stderr.log'
$preflight=Join-Path $instance 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
$world=Join-Path $instance 'saves/R209-local-only/level.dat'
foreach($path in @($jar,$log,$preflight,$world)){
  if(-not(Test-Path -LiteralPath $path)){
    throw "R209 missing Forge in-world receipt input: $path"
  }
}
$sha=(Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if($sha -ne $expected){throw 'R209 staged Forge JAR SHA256 mismatch'}
$pre=Get-Content -LiteralPath $preflight -Raw
if($pre -notmatch 'PLAYER_TICK_TEST=True' -or
   $pre -notmatch 'ISOLATED_WORLD_TEST=True' -or
   -not $pre.Contains("REOBF_SHA256=$expected")){
  throw 'R209 pinned independent loader preflight mismatch'
}
$raw=Get-Content -LiteralPath $log -Raw
$markers=@(
 'Forge Mod Loader has identified 4 mods to load',
 'FMLFileResourcePack:SlinkyWinky Preview',
 '[SlinkyWinkyPreview] R209_ISOLATION_GATE_PASS',
 '[SlinkyWinkyPreview] JNI_STARTUP_PROBE_PASS',
 '[SlinkyWinkyPreview] RENDER_EVENT_PROBE_PASS frames=30',
 '[SlinkyWinkyPreview] R209_LOCAL_WORLD_REQUESTED',
 'Loading dimension 0 (Isolated Sprint diagnostic)',
 'R209Offline joined the game',
 '[SlinkyWinkyPreview] R209_LOCAL_WORLD_PLAYER_READY_PASS',
 '[SlinkyWinkyPreview] R209_PLAYER_TICK_SPRINT_JNI_PASS'
)
foreach($marker in $markers){
  if(-not $raw.Contains($marker)){
    throw "R209 missing real Forge player-tick receipt: $marker"
  }
}
$regex='R209_PLAYER_TICK_SPRINT_JNI_PASS playerTickSamples=(\d+) clientTicks=(\d+) elapsedMs=(\d+) minSampleGapMs=(\d+) maxSampleGapMs=(\d+) inputForward=(-?[0-9.]+) inputStrafe=(-?[0-9.]+) proposedKeyPressed=(true|false) proposedSpeed=(true|false) speedSource=getWalkSpeed_proxy'
if($raw -notmatch $regex){throw 'R209 player-tick receipt payload malformed'}
$samples=[int]$Matches[1]
$ticks=[int]$Matches[2]
$elapsed=[int]$Matches[3]
$min=[int]$Matches[4]
$max=[int]$Matches[5]
$forward=[double]::Parse($Matches[6],[Globalization.CultureInfo]::InvariantCulture)
$strafe=[double]::Parse($Matches[7],[Globalization.CultureInfo]::InvariantCulture)
$key=[string]$Matches[8]
$speed=[string]$Matches[9]
if($samples -lt 32 -or $samples -gt 128 -or $ticks -lt 30 -or
   [Math]::Abs($ticks-$samples) -gt 4 -or
   $elapsed -lt 900 -or $elapsed -gt 9000 -or
   $min -gt $max -or $max -gt 800 -or
   $key -ne 'true' -or $speed -ne 'false' -or
   $forward -ne 0 -or $strafe -ne 0){
  throw 'R209 real tick cadence or stationary Sprint proposal unexpected'
}
if($raw.Split([string[]]@('R209_LOCAL_WORLD_REQUESTED'),
  [StringSplitOptions]::None).Length -ne 2) {
  throw 'R209 local test world requested more than once'
}
$process=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId"
if(-not $process){throw 'R209 original test JVM has exited before identity verification'}
$cmdline=[string]$process.CommandLine
if($process.ExecutablePath -notmatch 'jdk-8' -or
   -not $cmdline.Contains($instance) -or
   $cmdline -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $cmdline -notmatch 'R209Offline' -or
   $cmdline -notmatch 'slinkywinky[.]r209PlayerTickTest=true' -or
   $cmdline -notmatch 'slinkywinky[.]sprintObservation=true' -or
   $cmdline -match 'GradleMain|experimental[/\\]forge189[/\\]build[/\\]classes'){
  throw 'R209 refuses an unrelated Minecraft/Java process'
}
$stderrBytes=if(Test-Path $stderr){(Get-Item $stderr).Length}else{-1}
$shutdown='RUNNING_AT_ACCEPTANCE'
if($StopTestClient) {
  $gp=Get-Process -Id $TestProcessId -ErrorAction Stop
  if($gp.MainWindowHandle -ne [IntPtr]::Zero){
    [void]$gp.CloseMainWindow()
    Start-Sleep -Seconds 5
  }
  if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
    Stop-Process -Id $TestProcessId -Force -ErrorAction Stop
    Start-Sleep -Seconds 1
    $shutdown='EXACT_TEST_PID_STOPPED_AFTER_WINDOW_CLOSE'
  }else{
    $shutdown='GRACEFUL_EXIT'
  }
  if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
    throw 'R209 isolated Java test did not shut down'
  }
}
$receipt=Join-Path $instance 'R209_PLAYER_TICK_ACCEPTANCE.txt'
@(
 'R209_REAL_FORGE_PLAYER_TICK_JNI_OBSERVATION_PASS'
 "REOBF_SHA256=$expected"
 "PLAYER_TICK_SAMPLES=$samples"
 "CLIENT_TICKS=$ticks"
 "ELAPSED_MS=$elapsed"
 "MIN_SAMPLE_GAP_MS=$min"
 "MAX_SAMPLE_GAP_MS=$max"
 "OBSERVED_FORWARD=$forward"
 "OBSERVED_STRAFE=$strafe"
 "PROPOSED_KEY_PRESSED=$key"
 "PROPOSED_SPEED=$speed"
 'OBSERVATION_HOOK=FORGE_PLAYER_TICK_END'
 'MOVEMENT_INPUT_SOURCE=ENTITYPLAYERSP_MOVEMENT_INPUT'
 'SPEED_SOURCE=GET_WALK_SPEED_DIAGNOSTIC_PROXY'
 'AUTHENTIC_NATIVE_EVENT21_TIMING=NOT_ESTABLISHED'
 'MINECRAFT_GAMEPLAY_WRITES_BY_SPRINT=NONE'
 "JVM_STDERR_BYTES=$stderrBytes"
 "TEST_JVM_STATE=$shutdown"
) | Set-Content -LiteralPath $receipt -Encoding ASCII
Write-Output "R209_REAL_FORGE_PLAYER_TICK_JNI_OBSERVATION_PASS samples=$samples clientTicks=$ticks elapsedMs=$elapsed min=$min max=$max state=$shutdown"
Write-Output "R209_ACCEPTANCE_RECORD=$receipt"
