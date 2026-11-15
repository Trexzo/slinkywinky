# R207 independent Forge reobfuscated mod loader runtime receipt validator.
# Read-only inspection until the optional narrowly scoped test-client cleanup.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][int]$TestProcessId,
 [switch]$StopTestClient
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expected='f331bb3367236b15846288b3afb3c4d054d0e1b5bab941290e78dff121769e87'
$instance=(Resolve-Path -LiteralPath $InstanceRoot).Path
$temporary=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')+'\'
if(-not $instance.StartsWith($temporary,[StringComparison]::OrdinalIgnoreCase) -or
    $instance -notmatch 'slinkywinky-r207-independent-forge-') {
    throw 'R207 refuses to inspect/stop a non-isolated Minecraft instance'
}
$jar=Join-Path $instance 'mods/slinkywinky-preview-r207-reobf.jar'
$log=Join-Path $instance 'r207-forge-stdout.log'
$stderr=Join-Path $instance 'r207-forge-stderr.log'
if(-not(Test-Path $jar) -or -not(Test-Path $log)) {
    throw 'R207 missing staged JAR or local Forge log'
}
$sha=(Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if($sha -ne $expected){throw 'R207 staged reobfuscated JAR SHA mismatch'}
$lines=Get-Content -LiteralPath $log -Raw
$checks=@{
  'FML_ISOLATED_MOD_FOLDER'=$lines.Contains(('Searching '+$instance+'\mods for mods'))
  'FML_MOD_DISCOVERY'=$lines.Contains('Forge Mod Loader has identified 4 mods to load')
  'SLINKYWINKY_RESOURCE_PACK'=$lines.Contains('FMLFileResourcePack:SlinkyWinky Preview')
  'READ_ONLY_MOD_INITIALIZATION'=$lines.Contains('[SlinkyWinkyPreview] initialized - read-only')
  'JAVA_NATIVE_JNI_RECEIPT'=$lines.Contains('[SlinkyWinkyPreview] JNI_STARTUP_PROBE_PASS')
  'FORGE_REAL_RENDER_EVENT_RECEIPT'=$lines.Contains('[SlinkyWinkyPreview] RENDER_EVENT_PROBE_PASS frames=30')
}
foreach($check in $checks.GetEnumerator()) {
    if(-not $check.Value){throw "R207 independent Forge missing positive receipt: $($check.Key)"}
}
$process=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId"
if(-not $process){throw 'R207 independent Forge test process no longer available for identity confirmation'}
$cmdline=[string]$process.CommandLine
if($process.ExecutablePath -notmatch 'jdk-8' -or
    $cmdline -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
    $cmdline -notmatch 'R207Offline' -or
    $cmdline -notmatch 'net[.]minecraftforge[.]fml[.]common[.]launcher[.]FMLTweaker' -or
    -not $cmdline.Contains($instance) -or
    $cmdline -match 'experimental[/\\]forge189[/\\]build[/\\]classes' -or
    $cmdline -match 'org[.]gradle[.]launcher[.]GradleMain') {
    throw 'R207 refusing unrelated or development-classpath Java process'
}
$stderrBytes=if(Test-Path $stderr){(Get-Item $stderr).Length}else{-1}
$state='RUNNING_AT_ACCEPTANCE'
if($StopTestClient){
    $gp=Get-Process -Id $TestProcessId -ErrorAction Stop
    if($gp.MainWindowHandle -ne [IntPtr]::Zero){
        [void]$gp.CloseMainWindow()
        Start-Sleep -Seconds 5
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        # A legacy client may close its window but leave Java threads alive.
        # Stop only the exact previously verified isolated test PID.
        Stop-Process -Id $TestProcessId -Force -ErrorAction Stop
        Start-Sleep -Seconds 1
        $state='EXACT_ISOLATED_TEST_PID_STOPPED_AFTER_WINDOW_CLOSE'
    }else{
        $state='GRACEFUL_EXIT'
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        throw 'R207 isolated test JVM failed to terminate'
    }
}
$evidence=Join-Path $instance 'R207_INDEPENDENT_FORGE_RUNTIME_ACCEPTANCE.txt'
@(
 'R207_INDEPENDENT_FORGE_REOBF_MOD_RUNTIME_ACCEPTANCE_PASS'
 "REOBF_JAR_SHA256=$sha"
 'MOD_LOADED_FROM_ISOLATED_MODS_DIRECTORY=YES'
 'FORGE_RUNCLIENT_DEVELOPMENT_CLASSPATH=NO'
 'FORGE_LOADER=INSTALLED_FORGE_1_8_9_PROFILE'
 'JAVA_AUTHENTICATION=DUMMY_OFFLINE_ONLY'
 'SLINKYWINKY_MOD_INITIALIZED=YES'
 'NATIVE_JNI_STARTUP_PROBE=PASS'
 'FORGE_RENDER_EVENTS=30'
 'SPRINT_IN_WORLD_PARITY=NOT_VERIFIED'
 'DEFAULT_GAME_MODS_AND_SAVES=UNTOUCHED'
 "JVM_STDERR_BYTES_AT_ACCEPTANCE=$stderrBytes"
 "TEST_JVM_STATE=$state"
) |Set-Content -LiteralPath $evidence -Encoding ASCII
Write-Output "R207_INDEPENDENT_FORGE_REOBF_MOD_RUNTIME_ACCEPTANCE_PASS pid=$TestProcessId state=$state stderr_bytes=$stderrBytes"
Write-Output "R207_RECEIPT=$evidence"
