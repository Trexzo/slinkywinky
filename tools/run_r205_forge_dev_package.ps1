# R205 reproducible *development-only* Forge 1.8.9 read-only preview JAR.
# Requires exact local cached mapped Forge/LWJGL/Guava JARs and JDK8.
# Does NOT run Gradle, reobfuscate Minecraft names or launch Minecraft.
# No third-party dependencies are copied into the resulting JAR.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Java8Home,
    [Parameter(Mandatory=$true)][string]$ForgeJar,
    [Parameter(Mandatory=$true)][string]$LwjglJar,
    [Parameter(Mandatory=$true)][string]$GuavaJar,
    [Parameter(Mandatory=$true)][string]$OutputRoot
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$known = @{
    ForgeJar = 'e9eff72c1684f87767e42cf9edf391b74c4a2c5cabc51f8be29f2a20c5f5892d'
    LwjglJar = '833e721817f70d1445eec13d8ce5a86e12af70efac5014356302e2bbc68b3fe2'
    GuavaJar = '8c36a80ea613d0b6b8040a17cf837c5bbe3677bc1b06a058a6c174fdb787ebbc'
}
$inputs = @{
    ForgeJar = $ForgeJar
    LwjglJar = $LwjglJar
    GuavaJar = $GuavaJar
}
foreach ($key in @('ForgeJar','LwjglJar','GuavaJar')) {
    $file = (Resolve-Path -LiteralPath $inputs[$key]).Path
    $digest = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($digest -ne $known[$key]) { throw "R205 pinned dependency SHA256 mismatch: $key" }
    $inputs[$key] = $file
}
$javaHome = (Resolve-Path -LiteralPath $Java8Home).Path
$javac = Join-Path $javaHome 'bin/javac.exe'
$jar = Join-Path $javaHome 'bin/jar.exe'
$javap = Join-Path $javaHome 'bin/javap.exe'
foreach ($exe in @($javac,$jar,$javap)) {
    if (-not (Test-Path -LiteralPath $exe)) {throw "R205 Java8 tool missing: $exe"}
}
# JDK8 writes its version to STDERR. ProcessStartInfo captures output
# without PowerShell wrapping the line as a nonterminating ErrorRecord.
$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $javac
$psi.Arguments = '-version'
$psi.UseShellExecute = $false
$psi.RedirectStandardError = $true
$psi.RedirectStandardOutput = $true
$versionProcess = [System.Diagnostics.Process]::Start($psi)
try {
    $versionText = ($versionProcess.StandardOutput.ReadToEnd() +
        $versionProcess.StandardError.ReadToEnd()).Trim()
    $versionProcess.WaitForExit()
    if ($versionProcess.ExitCode -ne 0 -or
        $versionText -notmatch '^javac 1[.]8[.]') {
        throw "R205 requires JDK8 compiler, found: $versionText"
    }
} finally { $versionProcess.Dispose() }
if (Test-Path -LiteralPath $OutputRoot) {
    throw 'Refusing to overwrite an existing R205 dev-only output directory'
}
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$sources = @(
    'java/dev/slinkywinky/GameHost.java',
    'java/dev/slinkywinky/HostSettings.java',
    'java/dev/slinkywinky/NativeEngine.java',
    'java/dev/slinkywinky/SprintObservationProbe.java',
    'experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java'
)
$sourcePaths = @()
foreach ($relative in $sources) {
    $source = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $source)) {
        throw "R205 required first-party source missing: $relative"
    }
    $sourcePaths += $source
}
$cp = ($inputs.ForgeJar, $inputs.LwjglJar, $inputs.GuavaJar) -join ';'
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$rootOutput = (Resolve-Path -LiteralPath $OutputRoot).Path
$classes = Join-Path $rootOutput 'classes'
New-Item -ItemType Directory -Path $classes | Out-Null
# javac writes benign deprecated-API notes to STDERR. Capture these,
# but still fail on every nonzero compiler exit code.
$preference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
try {
    $javacOutput = (& $javac '-source' '8' '-target' '8' '-cp' $cp '-d' $classes @sourcePaths 2>&1 | Out-String)
    $javacCode = $LASTEXITCODE
} finally { $ErrorActionPreference = $preference }
if ($javacCode -ne 0) { throw "R205 Forge 1.8.9 Java8 source compilation failed: $javacOutput" }

$packageFile = Join-Path $rootOutput 'slinkywinky-forge189-r205-DEV-ONLY.jar'
Push-Location $classes
try {
    & $jar cf $packageFile 'dev'
    if ($LASTEXITCODE -ne 0) { throw 'R205 Java8 JAR packaging failed' }
} finally { Pop-Location }

$entries = @(& $jar tf $packageFile)
if ($LASTEXITCODE -ne 0) { throw 'R205 packaged Java8 JAR is unreadable' }
$required = @(
    'dev/slinkywinky/forge/ForgePreview.class',
    'dev/slinkywinky/GameHost.class',
    'dev/slinkywinky/HostSettings.class',
    'dev/slinkywinky/NativeEngine.class',
    'dev/slinkywinky/SprintObservationProbe.class'
)
foreach ($entry in $required) {
    if ($entries -notcontains $entry) { throw "R205 missing packaged Java class: $entry" }
}
foreach ($entry in $entries) {
    if (($entry -notlike 'META-INF/*') -and
        ($entry -notlike 'dev/*')) {
        throw "R205 unexpected third-party or unscoped JAR entry: $entry"
    }
}
$metadata = (& $javap -classpath $packageFile -verbose 'dev.slinkywinky.forge.ForgePreview' | Out-String)
if ($LASTEXITCODE -ne 0 -or
    $metadata -notmatch 'major version: 52' -or
    $metadata -notmatch 'Lnet/minecraftforge/fml/common/Mod;') {
    throw 'R205 Java8 classfile version or Forge @Mod annotation was not verified'
}
$classesPacked = @($entries | Where-Object { $_ -like '*.class' })
foreach ($entry in $classesPacked) {
    $class = $entry.Replace('/', '.').Replace('.class','')
    $ops = (& $javap -classpath $packageFile -c -p $class | Out-String)
    if ($LASTEXITCODE -ne 0) { throw "R205 javap failed: $class" }
    # Verify all packaged first-party classes remain free of common
    # Minecraft game-state setters and synthetic key-press actions.
    if ($ops -match '// Method .*setKeyBindState' -or
        $ops -match '// Method .*setSprinting' -or
        $ops -match '// Method .*setPlayerWalkSpeed' -or
        $ops -match '// Method .*setVelocity' -or
        $ops -match '// Method .*sendPacket') {
        throw "R205 unexpected game-write call in read-only dev JAR: $class"
    }
}
$sha = (Get-FileHash -LiteralPath $packageFile -Algorithm SHA256).Hash.ToLowerInvariant()
$manifest = Join-Path $rootOutput 'R205_LOCAL_DEV_PACKAGE_EVIDENCE.txt'
@(
    'R205_FORGE189_DEV_PACKAGE_PASS'
    "JAVA8=$versionText"
    "FORGE_SHA256=$($known.ForgeJar)"
    "LWJGL_SHA256=$($known.LwjglJar)"
    "GUAVA_SHA256=$($known.GuavaJar)"
    "CLASS_COUNT=$($classesPacked.Count)"
    "DEV_JAR_SHA256=$sha"
    'GAME_WRITES=NO_COMPILED_SETTER_CALLS_DETECTED'
    'FULL_FORGEGRADLE_REOBF=NOT_RUN'
    'GAME_LAUNCH=NOT_RUN'
    'USE=DEV_ONLY_NOT_INSTALLABLE_PRODUCTION_JAR'
) | Set-Content -LiteralPath $manifest -Encoding ASCII
Write-Output "R205_FORGE189_DEV_PACKAGE_PASS classes=$($classesPacked.Count) sha256=$sha"
Write-Output "DEV_ONLY_JAR=$packageFile"
Write-Output "EVIDENCE=$manifest"
