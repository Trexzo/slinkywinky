# R294: verify the EXACT live, dummy-identity Minecraft 1.8.9 test JVM
# before accepting R291's SHA-backed read-only JNI roster receipt.
# READ ONLY: does not launch, kill, modify saves or change game inputs.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][string]$Java8Home,
 [Parameter(Mandatory=$true)][string]$NativeJniDir,
 [Parameter(Mandatory=$true)][string]$PythonExe,
 [Parameter(Mandatory=$true)][ValidateRange(1,2147483647)][int]$TestProcessId
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$instance=(Resolve-Path -LiteralPath $InstanceRoot -ErrorAction Stop).ProviderPath
$instance=[IO.Path]::GetFullPath($instance).TrimEnd('\')
$tmp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
if (-not [String]::Equals([IO.Path]::GetDirectoryName($instance),$tmp,
      [StringComparison]::OrdinalIgnoreCase) -or
    [IO.Path]::GetFileName($instance) -notmatch
      '^slinkywinky-r278-independent-forge-[A-Za-z0-9][A-Za-z0-9._-]*$') {
  throw 'R294 refuses a non-isolated or nested Minecraft gameDir'
}
$java=(Resolve-Path -LiteralPath (Join-Path $Java8Home 'bin\java.exe') -ErrorAction Stop).ProviderPath
$java=[IO.Path]::GetFullPath($java)
$native=(Resolve-Path -LiteralPath (Join-Path $NativeJniDir 'slinkywinky_jni.dll') -ErrorAction Stop).ProviderPath
$python=(Resolve-Path -LiteralPath $PythonExe -ErrorAction Stop).ProviderPath
$verifier=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\tests\r289_roster_world_receipt.py') -ErrorAction Stop).ProviderPath
$p=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId" -ErrorAction Stop
if($null -eq $p){throw 'R294 test JVM PID not running'}
$exe=[string]$p.ExecutablePath
$cmd=[string]$p.CommandLine
if(-not [String]::Equals([IO.Path]::GetFullPath($exe),$java,
       [StringComparison]::OrdinalIgnoreCase) -or
   $cmd.IndexOf($instance,[StringComparison]::OrdinalIgnoreCase) -lt 0 -or
   $cmd -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $cmd -notmatch '(^|\s)--username\s+"?R278Offline"?' -or
   $cmd -notmatch 'slinkywinky[.]r289RosterLocalWorldTest=true' -or
   $cmd -notmatch 'slinkywinky[.]r278RosterReadOnlyProbe=true' -or
   $cmd -notmatch 'slinkywinky[.]r283NativeRosterReadOnlyProbe=true' -or
   $cmd -match 'GradleMain|slinkywinky[.]r212ActionTest=true|javaagent:') {
  throw 'R294 refuses unrelated, modified or non-read-only Java process'
}
# R291, not this wrapper, authenticates JAR SHA, staged mod, source
# isolation, native DLL hash, world save, and ordered JNI markers.
$output=& $python $verifier --root $instance --native-jni $native 2>&1
$exitCode=$LASTEXITCODE
if($exitCode -ne 0) {
  throw "R294 native receipt verifier refused: $output"
}
$positive=($output | Where-Object {
  ([string]$_).Contains('R291_REAL_FORGE_ROSTER_RECEIPT_PROVENANCE_PASS')
})
if($null -eq $positive -or @($positive).Count -ne 1) {
  throw 'R294 refused absent/duplicate source verifier acceptance'
}
# PID could be recycled while Python is checking files. Require the
# same creation timestamp, executable and exact command line after.
$after=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId" -ErrorAction Stop
if($null -eq $after -or
   $after.CreationDate -ne $p.CreationDate -or
   -not [String]::Equals([string]$after.ExecutablePath,$exe,
       [StringComparison]::OrdinalIgnoreCase) -or
   -not [String]::Equals([string]$after.CommandLine,$cmd,
       [StringComparison]::Ordinal)) {
  throw 'R294 test JVM exited or identity changed while validating'
}
Write-Output $positive
Write-Output "R294_EXACT_TEST_JVM_REAL_ROSTER_JNI_PASS pid=$TestProcessId processAlive=1 source=Forge_PlayerTick_END_JNI gameWrites=0"
