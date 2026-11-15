# R206: ForgeGradle 2.1 + Gradle 4.10.3 verified build of the
# read-only Forge 1.8.9 preview. No Minecraft launch or mod installation.
# Uses only first-party repository Java sources and the official Gradle zip.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Java8Home,
    [Parameter(Mandatory=$true)][string]$GradleZip,
    [Parameter(Mandatory=$true)][string]$OutputRoot,
    [switch]$Offline
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$gradleSha = '8626cbf206b4e201ade7b87779090690447054bc93f052954c78480fa6ed186e'
$sourceRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$forgeProject = Join-Path $sourceRoot 'experimental/forge189'
$javaHome = (Resolve-Path -LiteralPath $Java8Home).Path
$archive = (Resolve-Path -LiteralPath $GradleZip).Path

if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $gradleSha) {
    throw 'R206 official Gradle 4.10.3 archive SHA256 mismatch'
}
foreach ($tool in @('java.exe','javac.exe','javap.exe')) {
    if (-not (Test-Path -LiteralPath (Join-Path $javaHome "bin/$tool"))) {
        throw "R206 missing JDK tool $tool"
    }
}
if (-not (Test-Path -LiteralPath (Join-Path $forgeProject 'build.gradle'))) {
    throw 'R206 ForgeGradle project missing'
}
if (Test-Path -LiteralPath $OutputRoot) {
    throw 'R206 refuses to overwrite an existing acceptance directory'
}
if (-not (Test-Path -LiteralPath $forgeProject)) {
    throw 'R206 project directory missing'
}
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$dest = (Resolve-Path -LiteralPath $OutputRoot).Path
$distribution = Join-Path $dest 'gradle-distribution'
New-Item -ItemType Directory -Path $distribution | Out-Null
Expand-Archive -LiteralPath $archive -DestinationPath $distribution
$gradle = Join-Path $distribution 'gradle-4.10.3/bin/gradle.bat'
if (-not (Test-Path -LiteralPath $gradle)) {
    throw 'R206 Gradle executable missing after verified archive extraction'
}
$oldJavaHome = $env:JAVA_HOME
$oldPath = $env:PATH
$oldPreference = $ErrorActionPreference
try {
    $env:JAVA_HOME = $javaHome
    $env:PATH = (Join-Path $javaHome 'bin') + ';' + $oldPath
    $ErrorActionPreference = 'Continue'
    $gradleVersion = (& $gradle --version 2>&1 | Out-String)
    $versionCode = $LASTEXITCODE
    if ($versionCode -ne 0 -or
        $gradleVersion -notmatch 'Gradle 4[.]10[.]3' -or
        $gradleVersion -notmatch 'JVM:\s+1[.]8[.]') {
        throw 'R206 Gradle 4.10.3 / JDK8 toolchain version gate failed'
    }
    $log = Join-Path $dest 'forgegradle-build.log'
    $args = @('--no-daemon', '--console=plain')
    if ($Offline) { $args += '--offline' }
    $args += @('--rerun-tasks','build')
    Push-Location $forgeProject
    try {
        & $gradle @args *> $log
        $code = $LASTEXITCODE
    } finally { Pop-Location }
    $ErrorActionPreference = $oldPreference
    if ($code -ne 0) {
        $last = (Get-Content -LiteralPath $log -Tail 65 | Out-String)
        throw "R206 ForgeGradle 2.1 reobfuscated build failed (exit $code): $last"
    }
    $rawLog = Get-Content -LiteralPath $log -Raw
    if ($rawLog -notmatch 'BUILD SUCCESSFUL' -or
        $rawLog -notmatch ':reobfJar') {
        throw 'R206 build log does not prove successful ForgeGradle reobfJar task'
    }
    $jar = Join-Path $forgeProject 'build/libs/slinkywinky-preview-0.1.0-preview.jar'
    if (-not (Test-Path -LiteralPath $jar) -or
        (Get-Item -LiteralPath $jar).Length -lt 5000) {
        throw 'R206 no substantial reobfuscated preview JAR was produced'
    }
    $javap = Join-Path $javaHome 'bin/javap.exe'
    $jarTool = Join-Path $javaHome 'bin/jar.exe'
    $entries = @(& $jarTool tf $jar)
    if ($LASTEXITCODE -ne 0) { throw 'R206 jar tf validation failed' }
    $mustContain = @(
        'dev/slinkywinky/forge/ForgePreview.class',
        'dev/slinkywinky/GameHost.class',
        'dev/slinkywinky/NativeEngine.class',
        'dev/slinkywinky/HostSettings.class',
        'dev/slinkywinky/SprintObservationProbe.class'
    )
    foreach ($entry in $mustContain) {
        if ($entries -notcontains $entry) {
            throw "R206 missing first-party packaged class: $entry"
        }
    }
    foreach ($entry in $entries) {
        if ($entry -notlike 'META-INF/*' -and $entry -notlike 'dev/*') {
            throw "R206 unexpected third-party packaged file: $entry"
        }
    }
    $previewClass = (& $javap -classpath $jar -verbose 'dev.slinkywinky.forge.ForgePreview' | Out-String)
    if ($LASTEXITCODE -ne 0 -or
        $previewClass -notmatch 'major version: 52' -or
        $previewClass -notmatch 'Lnet/minecraftforge/fml/common/Mod;' -or
        $previewClass -notmatch 'func_71410_x') {
        throw 'R206 Forge @Mod, Java8 classfile or SRG-reobfuscated Minecraft reference missing'
    }
    $classes = @($entries | Where-Object { $_ -like '*.class' })
    foreach ($entry in $classes) {
        $name = $entry.Replace('/','.').Replace('.class','')
        $ops = (& $javap -classpath $jar -c -p $name | Out-String)
        if ($LASTEXITCODE -ne 0) { throw "R206 javap failed for $name" }
        if ($ops -match '// Method .*setKeyBindState' -or
            $ops -match '// Method .*setSprinting' -or
            $ops -match '// Method .*setPlayerWalkSpeed' -or
            $ops -match '// Method .*setVelocity' -or
            $ops -match '// Method .*sendPacket') {
            throw "R206 unexpected direct gameplay setter in read-only preview: $name"
        }
    }
    $artifact = Join-Path $dest 'slinkywinky-forge189-r206-REOBF-TEST.jar'
    Copy-Item -LiteralPath $jar -Destination $artifact
    $hash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash.ToLowerInvariant()
    $evidence = Join-Path $dest 'R206_FORGEGRADLE_ACCEPTANCE.txt'
    @(
        'R206_FORGEGRADLE_REOBF_ACCEPTANCE_PASS'
        "GRADLE_ZIP_SHA256=$gradleSha"
        'GRADLE_VERSION=4.10.3'
        'JAVA_MAJOR=8'
        'FORGEGRADLE_VERSION=2.1-SNAPSHOT'
        "CLASS_COUNT=$($classes.Count)"
        "ARTIFACT_SHA256=$hash"
        'REOBF_TASK=PASSED'
        'MINECRAFT_METHOD_REMAP=func_71410_x'
        'BYTECODE_MINECRAFT_ACTION_SETTERS=NOT_DETECTED_IN_FIRSTPARTY_CLASSES'
        'ACTUAL_MINECRAFT_LAUNCH=NOT_RUN'
        'LIVE_GAMEPLAY_PARITY=NOT_ESTABLISHED'
        'GAME_INSTALLATION=UNCHANGED'
    ) | Set-Content -LiteralPath $evidence -Encoding ASCII
    Write-Output "R206_FORGEGRADLE_REOBF_ACCEPTANCE_PASS classes=$($classes.Count) sha256=$hash"
    Write-Output "R206_ARTIFACT=$artifact"
    Write-Output "R206_EVIDENCE=$evidence"
} finally {
    $ErrorActionPreference = $oldPreference
    $env:JAVA_HOME = $oldJavaHome
    $env:PATH = $oldPath
}
