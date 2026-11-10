# R196: verify linked, source-pinned native descriptor metadata using the
# actual previously compiled 66-entry COFF manager (with first-party stubs).
# Never loads the full DLL, injects, or launches Minecraft.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$UpstreamRoot,
    [Parameter(Mandatory=$true)][string]$CorrectedBuild,
    [Parameter(Mandatory=$true)][string]$OriginalBuild,
    [string]$OutputRoot = (Join-Path $env:TEMP 'slinkywinky-r196-linked-native-acceptance')
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $repo 'reference_compat\compiled_manager_harness'
$corrected = (Resolve-Path $CorrectedBuild).Path
$original = (Resolve-Path $OriginalBuild).Path
$upstream = (Resolve-Path $UpstreamRoot).Path
$relativeObject = 'CMakeFiles\slinky.dir\src\engine\jni\module_manager.cpp.obj'
$goodObj = Join-Path $corrected $relativeObject
$badObj = Join-Path $original $relativeObject
$expectedHashes = @(
    @{ Name='corrected DLL'; Path=(Join-Path $corrected 'slinky.dll'); Expected='3613a466433fb1fa83d2d9e96815ece719c166f881c78642dc18d3e13ae2aa14' },
    @{ Name='corrected COFF'; Path=$goodObj; Expected='632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae' },
    @{ Name='original COFF'; Path=$badObj; Expected='40573226782654580a89504ecbb7ccb8b0217f832cb4d1fa47b771f277c78f80' }
)
foreach ($entry in $expectedHashes) {
    if (-not (Test-Path -LiteralPath $entry.Path -PathType Leaf)) {
        throw "Missing pinned build input: $($entry.Name)"
    }
    $actual = (Get-FileHash -LiteralPath $entry.Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $entry.Expected) {
        throw "R196 compiled artifact SHA-256 mismatch: $($entry.Name)"
    }
}
if (Test-Path -LiteralPath $OutputRoot) {
    throw "Refusing to overwrite previous R196 acceptance folder: $OutputRoot"
}
New-Item -ItemType Directory -Path $OutputRoot | Out-Null
$fixture = Join-Path $OutputRoot 'linked_descriptor_fixture.txt'
$json = Join-Path $OutputRoot 'linked_descriptor_report.json'
& python (Join-Path $repo 'tools\extract_r196_linked_descriptor_manifest.py') --upstream $upstream --dll (Join-Path $corrected 'slinky.dll') --map (Join-Path $corrected 'slinky.map') --fixture-out $fixture --json-out $json
if ($LASTEXITCODE -ne 0) { throw 'Pinned compiled descriptor extraction failed' }
$fixtureHash = (Get-FileHash -LiteralPath $fixture -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Output "R196_EXTRACTED_FIXTURE_SHA256=$fixtureHash"

$goodDir = Join-Path $OutputRoot 'corrected'
& cmake -S $source -B $goodDir "-DSLINKY_MANAGER_OBJECT=$goodObj" "-DSLINKY_DESCRIPTOR_FIXTURE=$fixture"
if ($LASTEXITCODE -ne 0) { throw 'R196 corrected harness configure failed' }
foreach ($config in @('Debug','Release')) {
    & cmake --build $goodDir --config $config --parallel 4
    if ($LASTEXITCODE -ne 0) { throw "R196 corrected harness compilation failed $config" }
    & ctest --test-dir $goodDir -C $config --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "R196 linked-metadata CTest failed $config" }
    Write-Output "R196_CORRECTED_$($config)_PASS"
}
$badDir = Join-Path $OutputRoot 'original-negative'
& cmake -S $source -B $badDir "-DSLINKY_MANAGER_OBJECT=$badObj"
if ($LASTEXITCODE -ne 0) { throw 'R196 original harness configure failed' }
foreach ($config in @('Debug','Release')) {
    & cmake --build $badDir --config $config --parallel 4
    if ($LASTEXITCODE -ne 0) { throw "R196 original harness compile failed $config" }
    $exe = Join-Path $badDir "$config\slinkywinky_r195_manager_acceptance.exe"
    $stdoutFile = Join-Path $OutputRoot "original-$config.stdout.txt"
    $stderrFile = Join-Path $OutputRoot "original-$config.stderr.txt"
    $quotedFixture = '"' + $fixture + '"'
    $proc = Start-Process -FilePath $exe -ArgumentList $quotedFixture -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile
    $diagnostic = Get-Content -LiteralPath $stderrFile -Raw
    if ($proc.ExitCode -ne 2 -or -not $diagnostic.Contains('R195_COMPILED_MANAGER_FAIL real-metadata manager count not 66')) {
        throw "R196 original 67-entry negative control did not reject correctly: $config"
    }
    Write-Output "R196_ORIGINAL_$($config)_EXPECTED_REJECTION_PASS"
}
Write-Output 'R196_PINNED_LINKED_COFF_RUNTIME_ACCEPTANCE_PASS'
