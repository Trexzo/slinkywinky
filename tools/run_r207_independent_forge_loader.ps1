# R207: launch a reobfuscated first-party Forge 1.8.9 preview JAR in
# a new isolated game directory. No credential, save, or mod copying.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$MinecraftRoot,
 [Parameter(Mandatory=$true)][string]$Java8Home,
 [Parameter(Mandatory=$true)][string]$ReobfJar,
 [Parameter(Mandatory=$true)][string]$NativeJniDir,
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [switch]$Launch,
 [switch]$R208LocalWorldTest,
 [switch]$R209PlayerTickTest,
 [switch]$R213LocalActionAcceptance,
 [switch]$R214LocalNoSlowAcceptance,
 [switch]$R215VanillaInputTest,
 [switch]$R216DisplacementTest,
 [switch]$R217ItemCategoriesTest,
 [switch]$R224LocalStrafeReadOnlyTest,
 [switch]$R289RosterWorldTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
if($R224LocalStrafeReadOnlyTest -and
   ($R208LocalWorldTest -or $R209PlayerTickTest -or
    $R213LocalActionAcceptance -or $R214LocalNoSlowAcceptance -or
    $R215VanillaInputTest -or $R216DisplacementTest -or
    $R217ItemCategoriesTest -or $R289RosterWorldTest)){
  throw 'R224 read-only native Strafe must be the ONLY selected test mode'
}
if($R289RosterWorldTest -and
   ($R208LocalWorldTest -or $R209PlayerTickTest -or
    $R213LocalActionAcceptance -or $R214LocalNoSlowAcceptance -or
    $R215VanillaInputTest -or $R216DisplacementTest -or
    $R217ItemCategoriesTest -or $R224LocalStrafeReadOnlyTest)) {
  throw 'R289 real roster test must be the ONLY selected mode'
}
$expectedJar=if($R289RosterWorldTest){
  '37ed0d9ec4319ae7ffeefce19787394d6de1d75214c27f063b85c461ea62a807'
}elseif($R224LocalStrafeReadOnlyTest){
  'ba8759cec99148cf14b31ad4e4b2c7ec06abe387a9730e3a0bb8586d60b2d345'
}elseif($R217ItemCategoriesTest){
  '8f03d30da031aeb21d72eb0dbc5cd337b09aadfdc673988101bb64a28b29bcff'
}elseif($R216DisplacementTest){
  'a4ef40b697019fdc209e31ee7b6e6453eb6326e952add4533033d05ad70ae08d'
}elseif($R215VanillaInputTest){
  '6620805e5f26438560e72a2f96280ca95f3d351323edde385ede37ed9b5e3915'
}elseif($R214LocalNoSlowAcceptance){
  'aa7bb470c3c5996ed08fa16d8962abe4aed0e8ae3ca07c5909351904452d78d8'
}elseif($R213LocalActionAcceptance){
  '45cfc71bd182c05393f79b718e5a7c330cf23ad2bdb137df69586c9921774bd2'
}elseif($R209PlayerTickTest){
  '27b7f8e707f90b3e4a5f5d56a7d7aa1e02aab55829c46fad494eb8b3771f8de2'
}elseif($R208LocalWorldTest){
  '5084bfce416abc8527f7e0e61f104378e104496e0eee40d6828a2b37970a5c0f'
}else{
  'f331bb3367236b15846288b3afb3c4d054d0e1b5bab941290e78dff121769e87'
}
$offlineUsername=if($R289RosterWorldTest){'R278Offline'}elseif($R224LocalStrafeReadOnlyTest){'R224Offline'}elseif($R217ItemCategoriesTest){'R217Offline'}elseif($R216DisplacementTest){'R216Offline'}elseif($R215VanillaInputTest){'R215Offline'}elseif($R214LocalNoSlowAcceptance){'R214Offline'}elseif($R213LocalActionAcceptance){'R213Offline'}elseif($R209PlayerTickTest){'R209Offline'}elseif($R208LocalWorldTest){'R208Offline'}else{'R207Offline'}
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
if($R289RosterWorldTest -or $R208LocalWorldTest -or $R209PlayerTickTest -or $R213LocalActionAcceptance -or $R214LocalNoSlowAcceptance -or $R215VanillaInputTest -or $R216DisplacementTest -or $R217ItemCategoriesTest -or $R224LocalStrafeReadOnlyTest){
  $target=[IO.Path]::GetFullPath($InstanceRoot).Replace('\','/')
  $tmp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).Replace('\','/')
  $prefix=if($R289RosterWorldTest){
    '/slinkywinky-r278-independent-forge-[^/]+$'
  }elseif($R224LocalStrafeReadOnlyTest){
    '/slinkywinky-r224-independent-forge-[^/]+$'
  }elseif($R217ItemCategoriesTest){
    '/slinkywinky-r217-independent-forge-[^/]+$'
  }elseif($R216DisplacementTest){
    '/slinkywinky-r216-independent-forge-[^/]+$'
  }elseif($R215VanillaInputTest){
    '/slinkywinky-r215-independent-forge-[^/]+$'
  }elseif($R214LocalNoSlowAcceptance){
    '/slinkywinky-r214-independent-forge-[^/]+$'
  }elseif($R213LocalActionAcceptance){
    '/slinkywinky-r213-independent-forge-[^/]+$'
  }elseif($R209PlayerTickTest){
    '/slinkywinky-r209-independent-forge-[^/]+$'
  }else{
    '/slinkywinky-r208-independent-forge-[^/]+$'
  }
  if(-not $target.StartsWith($tmp,[StringComparison]::OrdinalIgnoreCase) -or
     $target -notmatch $prefix){
    throw 'R208/R209 local world test requires a fresh named TEMP instance'
  }
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
            if($entry.Length -gt 32000000){throw 'R207 oversized native DLL'}
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
 '--username',$offlineUsername,
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
if($R289RosterWorldTest){
  $argsList=@(
    '-Dslinkywinky.r289RosterLocalWorldTest=true',
    '-Dslinkywinky.r278RosterReadOnlyProbe=true',
    '-Dslinkywinky.r283NativeRosterReadOnlyProbe=true',
    '-Dslinkywinky.r283AntiBotUuidMode=0')+$argsList
}elseif($R224LocalStrafeReadOnlyTest){
  # Opt-in real in-world *read-only* JNI Strafe proposal: no Javaagent,
  # gameplay motion write, packet modification, or original input hooks.
  $argsList=@('-Dslinkywinky.r224LocalStrafeReadOnlyTest=true')+$argsList
}elseif($R217ItemCategoriesTest){
  # Separate sword and consumable native No Slow test; exact hash pin,
  # dummy profile and TEMP-only world. R216 remains reproducible.
  $argsList=@("-javaagent:$staged",
    '-Dslinkywinky.r214NoSlowAgent=true',
    '-Dslinkywinky.r217ItemCategoriesTest=true')+$argsList
}elseif($R216DisplacementTest){
  $argsList=@("-javaagent:$staged",
    '-Dslinkywinky.r214NoSlowAgent=true',
    '-Dslinkywinky.r216DisplacementTest=true')+$argsList
}elseif($R215VanillaInputTest){
  $argsList=@("-javaagent:$staged",
    '-Dslinkywinky.r214NoSlowAgent=true',
    '-Dslinkywinky.r215VanillaInputTest=true')+$argsList
}elseif($R214LocalNoSlowAcceptance){
  # Earlier R214 test remains unchanged and hash pinned.
  $argsList=@("-javaagent:$staged",
    '-Dslinkywinky.r214NoSlowAgent=true',
    '-Dslinkywinky.r214LocalNoSlowAcceptance=true')+$argsList
}elseif($R213LocalActionAcceptance){
  $argsList=@('-Dslinkywinky.r213LocalActionAcceptance=true')+$argsList
}elseif($R209PlayerTickTest){
  $argsList=@('-Dslinkywinky.r209PlayerTickTest=true')+$argsList
}elseif($R208LocalWorldTest){
  $argsList=@('-Dslinkywinky.r208LocalWorldTest=true')+$argsList
}
$evidence=Join-Path $isolation 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
@(
 'R207_INDEPENDENT_FORGE_PREFLIGHT_PASS'
 "REOBF_SHA256=$expectedJar"
 "ISOLATED_WORLD_TEST=$([bool]($R289RosterWorldTest -or $R208LocalWorldTest -or $R209PlayerTickTest -or $R224LocalStrafeReadOnlyTest))"
 "R289_ROSTER_WORLD_TEST=$([bool]$R289RosterWorldTest)"
 "R289_ROSTER_NATIVE_READONLY=$([bool]$R289RosterWorldTest)"
 "NATIVE_JNI_SHA256=$((Get-FileHash (Join-Path $jni 'slinkywinky_jni.dll') -Algorithm SHA256).Hash.ToLowerInvariant())"
 "R224_LOCAL_STRAFE_READONLY=$([bool]$R224LocalStrafeReadOnlyTest)"
 "PLAYER_TICK_TEST=$([bool]$R209PlayerTickTest)"
 "R213_LOCAL_ACTION_ACCEPTANCE=$([bool]$R213LocalActionAcceptance)"
 "R214_EXPLICIT_POST_SLOW_AGENT=$([bool]$R214LocalNoSlowAcceptance)"
 "R215_VANILLA_INPUT_TEST=$([bool]$R215VanillaInputTest)"
 "R216_DISPLACEMENT_TEST=$([bool]$R216DisplacementTest)"
 "R217_ITEM_CATEGORIES_TEST=$([bool]$R217ItemCategoriesTest)"
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
