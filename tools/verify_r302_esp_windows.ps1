# R302 original Forge 1.8.9 JNI ESP in-world audit; read-only verifier.
# Does not launch, kill, change world saves, invoke JNI, or alter inputs.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][string]$Java8Home,
 [Parameter(Mandatory=$true)][string]$NativeJniDir,
 [Parameter(Mandatory=$true)][string]$PythonExe,
 [Parameter(Mandatory=$true)][string]$ExpectedReobfJarSha256,
 [Parameter(Mandatory=$true)][ValidateRange(1,2147483647)][int]$TestProcessId
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if($ExpectedReobfJarSha256 -notmatch '^[0-9a-fA-F]{64}$'){
  throw 'R302 verifier requires an exact reviewed Forge JAR SHA256'
}
$instance=(Resolve-Path -LiteralPath $InstanceRoot -ErrorAction Stop).ProviderPath
$instance=[IO.Path]::GetFullPath($instance).TrimEnd('\')
$tmp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
if(-not [String]::Equals([IO.Path]::GetDirectoryName($instance),$tmp,
      [StringComparison]::OrdinalIgnoreCase) -or
   [IO.Path]::GetFileName($instance) -notmatch
      '^slinkywinky-r302-independent-forge-[A-Za-z0-9][A-Za-z0-9._-]*$'){
  throw 'R302 refuses non-TEMP, nested or unrelated test worlds'
}
$java=(Resolve-Path -LiteralPath (Join-Path $Java8Home 'bin\java.exe')).ProviderPath
$java=[IO.Path]::GetFullPath($java)
$native=(Resolve-Path -LiteralPath (Join-Path $NativeJniDir 'slinkywinky_jni.dll')).ProviderPath
$python=(Resolve-Path -LiteralPath $PythonExe).ProviderPath
$checker=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\tests\r302_esp_world_receipt.py')).ProviderPath
$p=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId" -ErrorAction Stop
if($null -eq $p){throw 'R302 exact Java 8 test process not running'}
$exe=[string]$p.ExecutablePath
$cmd=[string]$p.CommandLine
if(-not [String]::Equals([IO.Path]::GetFullPath($exe),$java,
     [StringComparison]::OrdinalIgnoreCase) -or
   $cmd.IndexOf($instance,[StringComparison]::OrdinalIgnoreCase) -lt 0 -or
   $cmd -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $cmd -notmatch '(^|\s)--username\s+"?R302Offline"?' -or
   $cmd -notmatch 'slinkywinky[.]sprintObservation=true' -or
   $cmd -notmatch 'slinkywinky[.]r300EspReadOnlyProbe=true' -or
   $cmd -notmatch 'slinkywinky[.]r301NativeEspReadOnlyProbe=true' -or
   $cmd -notmatch 'slinkywinky[.]r302EspLocalWorldTest=true' -or
   $cmd -match 'GradleMain|javaagent:|slinkywinky[.]r212ActionTest=true'){
  throw 'R302 refuses unverified/modified/non-read-only Java process'
}
$proof=& $python $checker --root $instance --native-jni $native --expected-jar-sha256 $ExpectedReobfJarSha256.ToLowerInvariant() 2>&1
if($LASTEXITCODE -ne 0){
  throw "R302 world receipt provenance refused: $proof"
}
$positive=@($proof | Where-Object {
  ([string]$_).Contains('R302_REAL_FORGE_ESP_RECEIPT_PROVENANCE_PASS')
})
if($positive.Count -ne 1){throw 'R302 requires one verified live world receipt'}
$after=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId" -ErrorAction Stop
if($null -eq $after -or $after.CreationDate -ne $p.CreationDate -or
   -not [String]::Equals([string]$after.ExecutablePath,$exe,
     [StringComparison]::OrdinalIgnoreCase) -or
   -not [String]::Equals([string]$after.CommandLine,$cmd,
     [StringComparison]::Ordinal)){
  throw 'R302 Java process changed while validating receipt'
}
Write-Output $positive
Write-Output "R302_EXACT_JVM_REAL_ESP_JNI_PASS pid=$TestProcessId source=Forge_PlayerTick_END_JNI gameWrites=0"
