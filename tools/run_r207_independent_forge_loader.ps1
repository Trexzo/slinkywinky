# R207: launch a reobfuscated first-party Forge 1.8.9 preview JAR in
# a new isolated game directory. No credential, save, or mod copying.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$MinecraftRoot,
 [Parameter(Mandatory=$true)][string]$Java8Home,
 [Parameter(Mandatory=$true)][string]$ReobfJar,
 [Parameter(Mandatory=$true)][string]$NativeJniDir,
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [switch]$Launch
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expectedJar='f331bb3367236b15846288b3afb3c4d054d0e1b5bab941290e78dff121769e87'
$expectedVanilla='14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298'
$forgeId='1.8.9-forge1.8.9-11.15.1.2318-1.8.9'
$root=(Resolve-Path -LiteralPath $MinecraftRoot).Path
$jdk=(Resolve-Path -LiteralPath $Java8Home).Path
$jar=(Resolve-Path -LiteralPath $ReobfJar).Path
$jni=(Resolve-Path -LiteralPath $NativeJniDir).Path
if ([IO.Path]::GetFullPath($InstanceRoot).TrimEnd('\') -eq $root.TrimEnd('\') -or
    [IO.Path]::GetFullPath($InstanceRoot).StartsWith($root.TrimEnd('\')+'\',
      [StringComparison]::OrdinalIgnoreCase)) {
    throw 'R207 refuses to write to original Minecraft root'
}
if (Test-Path -LiteralPath $InstanceRoot) {
    throw 'R207 refuses to overwrite a prior isolated instance'
}
$java=Join-Path $jdk 'bin/java.exe'
if (-not(Test-Path $java) -or -not(Test-Path (Join-Path $jni 'slinkywinky_jni.dll'))) {
    throw 'R207 missing existing JDK8 or first-party JNI DLL'
}
if ((Get-FileHash $jar -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedJar) {
    throw 'R207 reobfuscated preview JAR hash mismatch'
}
$forge=Get-Content -Raw (Join-Path $root "versions/$forgeId/$forgeId.json")|ConvertFrom-Json
$vanilla=Get-Content -Raw (Join-Path $root 'versions/1.8.9/1.8.9.json')|ConvertFrom-Json
if ($forge.inheritsFrom -ne '1.8.9' -or
    $forge.mainClass -ne 'net.minecraft.launchwrapper.Launch' -or
    $forge.id -ne $forgeId) {
    throw 'R207 unsupported installed Forge profile metadata'
}
$mcJar=Join-Path $root 'versions/1.8.9/1.8.9.jar'
if ((Get-FileHash $mcJar -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedVanilla) {
    throw 'R207 vanilla Minecraft 1.8.9 JAR hash mismatch'
}
if (-not(Test-Path (Join-Path $root 'assets/indexes/1.8.json'))) {
    throw 'R207 missing read-only asset index'
}
$libraryPaths=New-Object 'System.Collections.Generic.List[string]'
$nativePaths=New-Object 'System.Collections.Generic.List[string]'
$seen=@{}
foreach($lib in @($forge.libraries)+@($vanilla.libraries)) {
    $parts=$lib.name.Split(':')
    if($parts.Count -ne 3) {throw "R207 unknown Maven coordinate: $($lib.name)"}
    $id=$parts[0]+':'+$parts[1]
    if($seen.ContainsKey($id)){continue}
    $seen[$id]=$true
    $allowed=$true
    if($null -ne $lib.PSObject.Properties['rules'] -and $lib.rules) {
        $allowed=$false
        foreach($rule in $lib.rules) {
            if($null -eq $rule.PSObject.Properties['os'] -or $rule.os.name -eq 'windows') {
                $allowed=$rule.action -eq 'allow'
            }
        }
    }
    if(-not $allowed) {continue}
    $prefix=$parts[0].Replace('.','/')+'/'+$parts[1]+'/'+$parts[2]+'/'+$parts[1]+'-'+$parts[2]
    if($null -ne $lib.PSObject.Properties['natives'] -and $lib.natives) {
        $classifier=[string]$lib.natives.windows
        if(-not $classifier){throw "R207 missing native classifier: $id"}
        # Legacy Mojang metadata uses a Windows architecture placeholder.
        $classifier=$classifier.Replace('arch','64').Replace('$','').Replace('{','').Replace('}','')
        $path=Join-Path $root ('libraries/'+$prefix+'-'+$classifier+'.jar')
        if(-not(Test-Path $path)){throw "R207 native dependency missing: $id/$classifier"}
        $nativePaths.Add($path)
    } else {
        $path=Join-Path $root ('libraries/'+$prefix+'.jar')
        if(-not(Test-Path $path)){throw "R207 library missing: $id"}
        $libraryPaths.Add($path)
    }
}
if($libraryPaths.Count -lt 35 -or $nativePaths.Count -lt 3){
    throw 'R207 incomplete independent profile classpath'
}
$forgeLoader=Join-Path $root "versions/$forgeId/$forgeId.jar"
if(-not(Test-Path $forgeLoader)){throw 'R207 installed Forge loader JAR missing'}
New-Item -ItemType Directory -Path $InstanceRoot|Out-Null
$isolation=(Resolve-Path -LiteralPath $InstanceRoot).Path
$mods=Join-Path $isolation 'mods'
$natives=Join-Path $isolation 'natives'
New-Item -ItemType Directory -Path @($mods,$natives)|Out-Null
$staged=Join-Path $mods 'slinkywinky-preview-r207-reobf.jar'
Copy-Item -LiteralPath $jar -Destination $staged
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$nativeCount=0
foreach($archive in $nativePaths){
    $zip=[IO.Compression.ZipFile]::OpenRead($archive)
    try{
        foreach($entry in $zip.Entries){
            $name=[IO.Path]::GetFileName($entry.FullName)
            if(-not $name.EndsWith('.dll',[StringComparison]::OrdinalIgnoreCase)){continue}
            if($entry.Length -gt 20000000){throw 'R207 oversized native DLL'}
            $target=Join-Path $natives $name
            if(Test-Path $target){throw "R207 colliding DLL: $name"}
            $inputStream=$entry.Open()
            $outputStream=[IO.File]::Create($target)
            try{$inputStream.CopyTo($outputStream)}
            finally{$outputStream.Dispose();$inputStream.Dispose()}
            $nativeCount++
        }
    } finally {$zip.Dispose()}
}
if($nativeCount -lt 3){throw 'R207 required LWJGL/JInput natives not extracted'}
$classpath=(@($libraryPaths.ToArray()) + @($forgeLoader,$mcJar)) -join ';'
$assets=Join-Path $root 'assets'
$javaNatives=$natives+';'+$jni
$argsList=@(
 '-Xms384m','-Xmx2048m',
 "-Djava.library.path=$javaNatives",
 "-Dorg.lwjgl.librarypath=$natives",
 "-Dnet.java.games.input.librarypath=$natives",
 '-Dslinkywinky.renderProbe=true',
 '-Dslinkywinky.startupProbe=true',
 '-Dslinkywinky.sprintObservation=true',
 '-Dslinkywinky.sprint.allowBackwards=true',
 '-cp',$classpath,
 'net.minecraft.launchwrapper.Launch',
 '--username','R207Offline',
 '--version',$forgeId,
 '--gameDir',$isolation,
 '--assetsDir',$assets,
 '--assetIndex','1.8',
 '--uuid','00000000000000000000000000000000',
 '--accessToken','0',
 '--userType','legacy',
 '--versionType','Forge',
 '--tweakClass','net.minecraftforge.fml.common.launcher.FMLTweaker',
 '--width','960','--height','540'
)
$evidence=Join-Path $isolation 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
@(
 'R207_INDEPENDENT_FORGE_PREFLIGHT_PASS'
 "REOBF_SHA256=$expectedJar"
 "MC_SHA256=$expectedVanilla"
 "LIBRARY_COUNT=$($libraryPaths.Count)"
 "NATIVE_ARCHIVES=$($nativePaths.Count)"
 "DLL_COUNT=$nativeCount"
 "STAGED_MOD=$staged"
 'PROFILE=ISOLATED_TEMPORARY'
 'LOGIN=OFFLINE_DUMMY'
 'ORIGINAL_MINECRAFT_LIBRARIES_AND_ASSETS=READ_ONLY'
) | Set-Content -LiteralPath $evidence -Encoding ASCII
Write-Output "R207_INDEPENDENT_FORGE_PREFLIGHT_PASS libraries=$($libraryPaths.Count) natives=$nativeCount"
Write-Output "R207_STAGED_MOD=$staged"
if(-not $Launch){exit 0}
$quoted=($argsList|ForEach-Object{'"'+[string]$_+'" '}) -join ''
$log=Join-Path $isolation 'r207-forge-stdout.log'
$err=Join-Path $isolation 'r207-forge-stderr.log'
$p=Start-Process -FilePath $java -ArgumentList $quoted -PassThru -RedirectStandardOutput $log -RedirectStandardError $err -WorkingDirectory $isolation
Write-Output "R207_INDEPENDENT_FORGE_STARTED_PID=$($p.Id)"
Write-Output "R207_LOG=$log"
