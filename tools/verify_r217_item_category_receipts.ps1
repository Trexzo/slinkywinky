# R217: independently verify six real Minecraft 1.8.9 player-position
# displacement windows for No Slow sword and consumable JNI settings.
# No user Minecraft files or production processes are modified.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$InstanceRoot,
 [Parameter(Mandatory=$true)][int]$TestProcessId,
 [switch]$StopTestClient
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$expected='8f03d30da031aeb21d72eb0dbc5cd337b09aadfdc673988101bb64a28b29bcff'
$instance=(Resolve-Path -LiteralPath $InstanceRoot).Path
$normalized=$instance.Replace('\','/')
$temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).Replace('\','/')
if(-not $normalized.StartsWith($temp,[StringComparison]::OrdinalIgnoreCase) -or
   $normalized -notmatch '/slinkywinky-r217-independent-forge-[^/]+$'){
    throw 'R217 refuses non-isolated TEMP gameDir'
}
$jar=Join-Path $instance 'mods/slinkywinky-preview-r207-reobf.jar'
$log=Join-Path $instance 'r207-forge-stdout.log'
$stderr=Join-Path $instance 'r207-forge-stderr.log'
$preflight=Join-Path $instance 'R207_INDEPENDENT_FORGE_PREFLIGHT.txt'
$world=Join-Path $instance 'saves/R217-local-only/level.dat'
foreach($file in @($jar,$log,$preflight,$world)){
    if(-not(Test-Path -LiteralPath $file)){
        throw "R217 missing test evidence: $file"
    }
}
$sha=(Get-FileHash -LiteralPath $jar -Algorithm SHA256).Hash.ToLowerInvariant()
if($sha -ne $expected){throw 'R217 independent Forge JAR digest mismatch'}
$pre=Get-Content -LiteralPath $preflight -Raw
if(-not $pre.Contains("REOBF_SHA256=$expected") -or
   -not $pre.Contains('R217_ITEM_CATEGORIES_TEST=True')){
    throw 'R217 incorrect hash-pinned launcher mode'
}
$raw=Get-Content -LiteralPath $log -Raw
$ordered=@(
 'R214_LOCAL_AGENT_ARMED',
 'R214_POST_SLOW_PRE_CONSUME_ASM_PATCH_PASS',
 'R217_CATEGORIES_ISOLATION_GATE_PASS',
 'JNI_STARTUP_PROBE_PASS',
 'Forge Mod Loader has successfully loaded 4 mods',
 'R217_LOCAL_WORLD_REQUESTED',
 'R217Offline joined the game',
 'R217_REAL_MOVEMENT_INPUT_ATTACHED',
 'R214_REAL_ENTITYPLAYER_ONLIVINGUPDATE_HOOK_REACHED',
 'R217_CATEGORY_DISPLACEMENT item=sword phase=enabled',
 'R217_CATEGORY_DISPLACEMENT item=sword phase=disabled',
 'R217_CATEGORY_DISPLACEMENT item=sword phase=reconfigured',
 'R217_CATEGORY_DISPLACEMENT item=apple phase=enabled',
 'R217_CATEGORY_DISPLACEMENT item=apple phase=disabled',
 'R217_CATEGORY_DISPLACEMENT item=apple phase=reconfigured',
 'R217_REAL_SWORD_CONSUMABLE_DISPLACEMENT_PASS',
 'R217_CATEGORY_ACTION_CLEANUP_PASS'
)
$prior=-1
foreach($marker in $ordered){
    $at=$raw.IndexOf($marker,[StringComparison]::Ordinal)
    if($at -le $prior){throw "R217 evidence absent or out of order: $marker"}
    $prior=$at
    $count=$raw.Split([string[]]@($marker),[StringSplitOptions]::None).Length-1
    if($count -ne 1){throw "R217 duplicated/missing evidence $marker count=$count"}
}
if($raw.Contains('R214_NO_SLOW_ACTION_FAIL') -or
   $raw.Contains('R214_TRANSFORM_REFUSED') -or
   $raw.Contains('[Client thread/FATAL]')){
    throw 'R217 game log contains a native failure, class refusal or fatal error'
}
$pattern='R217_CATEGORY_DISPLACEMENT item=(sword|apple) phase=(enabled|disabled|reconfigured) multiplier=([0-9.]+) itemId=(\d+) samples=(\d+) horizontalBlocks=([0-9.]+) meanBlocksPerTick=([0-9.]+)'
$records=[regex]::Matches($raw,$pattern)
if($records.Count -ne 6){throw 'R217 did not produce exactly six measured item phases'}
$names=@('sword','sword','sword','apple','apple','apple')
$phases=@('enabled','disabled','reconfigured','enabled','disabled','reconfigured')
$multipliers=@([double]0.65,0.2,0.4,0.45,0.2,0.8)
$ids=@(276,276,276,260,260,260)
$means=New-Object 'System.Collections.Generic.List[double]'
$blocks=New-Object 'System.Collections.Generic.List[double]'
for($i=0;$i -lt 6;$i++){
    $m=$records[$i]
    if($m.Groups[1].Value -ne $names[$i] -or
       $m.Groups[2].Value -ne $phases[$i] -or
       [int]::Parse($m.Groups[4].Value) -ne $ids[$i] -or
       [int]::Parse($m.Groups[5].Value) -ne 33){
        throw "R217 wrong item ID, phase or sample count at $i"
    }
    $setting=[double]::Parse($m.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture)
    $distance=[double]::Parse($m.Groups[6].Value,[Globalization.CultureInfo]::InvariantCulture)
    $mean=[double]::Parse($m.Groups[7].Value,[Globalization.CultureInfo]::InvariantCulture)
    if([Math]::Abs($setting-$multipliers[$i]) -gt 0.00001 -or
       [double]::IsNaN($distance) -or [double]::IsInfinity($distance) -or
       [double]::IsNaN($mean) -or [double]::IsInfinity($mean) -or
       $distance -le 0 -or $mean -le 0 -or
       [Math]::Abs($distance / 33 - $mean) -gt 0.000001){
        throw "R217 inconsistent item setting or measured real displacement at phase $i"
    }
    $means.Add($mean)
    $blocks.Add($distance)
}
$computed=@(
 $means[0]/$means[1],
 $means[2]/$means[1],
 $means[3]/$means[4],
 $means[5]/$means[4],
 $means[4]/$means[1]
)
$expectedRatios=@([double]3.25,2.0,2.25,4.0,1.0)
$tolerances=@([double]0.70,0.55,0.60,0.85,0.25)
for($i=0;$i -lt 5;$i++){
    if([Math]::Abs($computed[$i]-$expectedRatios[$i]) -gt $tolerances[$i]){
        throw "R217 measured player displacement does not match multiplier/category $i"
    }
}
$final='R217_REAL_SWORD_CONSUMABLE_DISPLACEMENT_PASS samplesPerPhase=(\d+) callbacks=(\d+) ownedFieldWrites=(\d+) sword65Ratio=([0-9.]+) sword40Ratio=([0-9.]+) apple45Ratio=([0-9.]+) apple80Ratio=([0-9.]+) vanillaCrossItemRatio=([0-9.]+) movementClass=MovementInputFromOptions keySource=synthetic_local_binding'
if($raw -notmatch $final){throw 'R217 final native category acceptance line malformed'}
$callbacks=[int]$Matches[2]
$writes=[int]$Matches[3]
if([int]$Matches[1] -ne 33 -or $callbacks -lt 270 -or $callbacks -gt 350 -or
   $writes -lt 180 -or $writes -gt 240){
    throw 'R217 implausible native callback/write counts'
}
for($i=0;$i -lt 5;$i++){
    $logged=[double]::Parse($Matches[$i+4],[Globalization.CultureInfo]::InvariantCulture)
    if([Math]::Abs($logged-$computed[$i]) -gt 0.00001){
        throw "R217 logged ratio differs from independently recomputed displacement at $i"
    }
}
$proc=Get-CimInstance Win32_Process -Filter "ProcessId=$TestProcessId"
if(-not $proc){throw 'R217 exact original Java8 test process not found'}
$command=[string]$proc.CommandLine
if($proc.ExecutablePath -notmatch 'jdk-8' -or
   $command -notmatch 'net[.]minecraft[.]launchwrapper[.]Launch' -or
   $command -notmatch 'R217Offline' -or
   $command -notmatch 'slinkywinky[.]r217ItemCategoriesTest=true' -or
   $command -notmatch 'slinkywinky[.]r214NoSlowAgent=true' -or
   $command -notmatch 'javaagent:' -or
   -not $command.Contains($instance) -or
   $command -match 'GradleMain|experimental[/\\]forge189[/\\]build[/\\]classes'){
    throw 'R217 process identity does not match the accepted isolated client'
}
$stderrBytes=if(Test-Path -LiteralPath $stderr){(Get-Item $stderr).Length}else{-1}
$state='RUNNING_AT_ACCEPTANCE'
if($StopTestClient){
    $p=Get-Process -Id $TestProcessId -ErrorAction Stop
    if($p.MainWindowHandle -ne [IntPtr]::Zero){
        [void]$p.CloseMainWindow()
        Start-Sleep -Seconds 4
    }
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        Stop-Process -Id $TestProcessId -Force -ErrorAction Stop
        Start-Sleep -Seconds 1
        $state='VERIFIED_TEST_JVM_STOPPED_AFTER_WINDOW_CLOSE'
    }else{$state='GRACEFUL_EXIT'}
    if(Get-Process -Id $TestProcessId -ErrorAction SilentlyContinue){
        throw 'R217 isolated JVM could not be stopped'
    }
}
$receipt=Join-Path $instance 'R217_ITEM_CATEGORY_MOVEMENT_ACCEPTANCE.txt'
@(
 'R217_REAL_SWORD_CONSUMABLE_DISPLACEMENT_NATIVE_ACTION_PASS'
 "REOBF_JAR_SHA256=$expected"
 "CALLBACKS=$callbacks"
 "OWNED_INPUT_FIELD_WRITES=$writes"
 'MEASURED_TICKS_PER_PHASE=33'
 'ITEM_IDS=SWORD_276_APPLE_260'
 'INPUT_READER=Minecraft MovementInputFromOptions'
 'INPUT_STIMULUS=SYNTHETIC_LOCAL_BINDING_NOT_PHYSICAL'
 'GAME=TEMP_LOCAL_SINGLEPLAYER_ONLY'
 "SWORD_65_PERCENT_MEAN=$($means[0])"
 "SWORD_DISABLED_MEAN=$($means[1])"
 "SWORD_40_PERCENT_MEAN=$($means[2])"
 "APPLE_45_PERCENT_MEAN=$($means[3])"
 "APPLE_DISABLED_MEAN=$($means[4])"
 "APPLE_80_PERCENT_MEAN=$($means[5])"
 "SWORD_65_VANILLA_RATIO=$($computed[0])"
 "SWORD_40_VANILLA_RATIO=$($computed[1])"
 "APPLE_45_VANILLA_RATIO=$($computed[2])"
 "APPLE_80_VANILLA_RATIO=$($computed[3])"
 "CROSS_CATEGORY_VANILLA_RATIO=$($computed[4])"
 "STDERR_BYTES_AT_ACCEPTANCE=$stderrBytes"
 "JVM_STATE=$state"
) | Set-Content -LiteralPath $receipt -Encoding ASCII
Write-Output "R217_REAL_SWORD_CONSUMABLE_DISPLACEMENT_NATIVE_ACTION_PASS callbacks=$callbacks writes=$writes sword65=$($computed[0]) sword40=$($computed[1]) apple45=$($computed[2]) apple80=$($computed[3]) state=$state"
Write-Output "R217_LOCAL_RECEIPT=$receipt"
