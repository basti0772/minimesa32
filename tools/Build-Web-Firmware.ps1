#requires -Version 7.0
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$config = Get-Content (Join-Path $PSScriptRoot 'web-firmware.json') -Raw | ConvertFrom-Json
$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
if ($version -notmatch '^\d+\.\d+\.\d+([+-][A-Za-z0-9.-]+)?$') { throw 'Invalid VERSION.' }
$build = Join-Path $root '.build/web'
$sketch = Join-Path $root $config.sketch
$setup = (Join-Path $sketch 'config/User_Setup.h').Replace('\','/')
New-Item -ItemType Directory -Force -Path $build | Out-Null
# __FILE__ in library assertions can otherwise leak the builder's Windows profile.
# Cover both slash spellings used by Arduino recipes; map the project as well for
# workspaces located outside the profile. These are computed, never stored paths.
$prefixFlags = @()
foreach ($prefix in @([Environment]::GetFolderPath('UserProfile'),$root)) {
    $prefixFlags += ('"-ffile-prefix-map=' + $prefix.Replace('\','/') + '=build"')
    $prefixFlags += ('"-ffile-prefix-map=' + $prefix + '=build"')
}
$safeFlags = $prefixFlags -join ' '
# Force the checked-in display setup into ALL C++ translation units, including
# TFT_eSPI.cpp, without modifying the user's globally installed library.
$compileArgs = @('compile','--fqbn',$config.fqbn,'--build-path',$build,
    '--build-property',('compiler.c.extra_flags=' + $safeFlags),
    '--build-property',('compiler.cpp.extra_flags=' + $safeFlags + ' -DUSER_SETUP_LOADED -include "' + $setup + '"'))
$propertiesOutput = & arduino-cli @compileArgs '--show-properties=expanded' $sketch
if ($LASTEXITCODE -ne 0) { throw 'Cannot resolve Arduino build properties.' }
$properties = @{}
foreach ($line in $propertiesOutput) {
    if ($line -match '^([^=]+)=(.*)$') { $properties[$Matches[1]] = $Matches[2] }
}
foreach ($key in @('build.mcu','build.flash_size','build.flash_mode','build.img_freq',
                  'build.partitions','runtime.platform.path','tools.esptool_py.path','tools.esptool_py.cmd')) {
    if (!$properties.ContainsKey($key)) { throw "Missing build property: $key" }
}
if ($properties['build.mcu'] -ne 'esp32') { throw 'This project supports the classic ESP32 only.' }
$platformVersion = Split-Path $properties['runtime.platform.path'] -Leaf
if ($platformVersion -ne $config.coreVersion) {
    throw "Expected ESP32 core $($config.coreVersion); found $platformVersion. Install the pinned version."
}
Write-Host "Building $($config.name) $version / $($config.fqbn)"
$librariesOutput = & arduino-cli lib list --format json
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect installed Arduino libraries.' }
$libraries = $librariesOutput | ConvertFrom-Json
$tft = @($libraries.installed_libraries | Where-Object { $_.library.name -eq 'TFT_eSPI' })
if ($tft.Count -ne 1 -or $tft[0].library.version -ne $config.tftVersion) {
    throw "Install exactly TFT_eSPI@$($config.tftVersion) before building."
}
& arduino-cli @compileArgs '--clean' $sketch
if ($LASTEXITCODE -ne 0) { throw 'Arduino compilation failed. No web firmware published.' }

# flash_args is emitted by the actual ESP32 Arduino core build hooks. Do not
# substitute assumed addresses if it is absent or changes format.
$flashArgsPath = Join-Path $build 'flash_args'
if (!(Test-Path $flashArgsPath)) { throw 'Build did not produce flash_args.' }
$lines = @(Get-Content $flashArgsPath | Where-Object { $_.Trim() })
$tokens = @($lines[0].Trim() -split '\s+')
if ($tokens.Count -ne 6) { throw 'Unexpected flash_args options.' }
$options = @{}
for ($i=0; $i -lt $tokens.Count; $i+=2) { $options[$tokens[$i]] = $tokens[$i+1] }
foreach ($key in @('--flash-mode','--flash-freq','--flash-size')) {
    if (!$options.ContainsKey($key)) { throw "Missing flash option: $key" }
}
if ($options['--flash-mode'] -ne $properties['build.flash_mode'] -or
    $options['--flash-freq'] -ne $properties['build.img_freq'] -or
    $options['--flash-size'] -ne $properties['build.flash_size']) { throw 'Build/flash_args mismatch.' }
if ($options['--flash-mode'] -notin @('dio','dout')) { throw 'Unsupported merged-image mode; review core output.' }
$parts = @()
$mergeParts = @()
$previousEnd = 0
foreach ($line in $lines[1..($lines.Count-1)]) {
    if ($line.Trim() -notmatch '^(0x[0-9a-fA-F]+|\d+)\s+"?([^"\r\n]+\.bin)"?$') { throw "Unexpected image entry: $line" }
    $address = $Matches[1]
    $file = $Matches[2]
    if ([IO.Path]::GetFileName($file) -ne $file) { throw 'Expected build-local binary names in flash_args.' }
    $offset = if ($address.StartsWith('0x')) { [Convert]::ToInt32($address.Substring(2),16) } else { [int]$address }
    $path = Join-Path $build $file
    if (!(Test-Path $path)) { throw "Missing build image: $file" }
    $length = (Get-Item $path).Length
    if (!$length -or $offset -lt $previousEnd) { throw 'Empty or overlapping flash images.' }
    $previousEnd = $offset + $length
    $parts += [ordered]@{ file=$file; offset=$offset; offsetHex=('0x{0:X}' -f $offset); size=$length; sha256=(Get-FileHash $path).Hash.ToLowerInvariant() }
    $mergeParts += @($address,$path)
}
if ($parts.Count -ne 4) { throw 'Expected four images from this core; review changed flash layout.' }
if ($options['--flash-size'] -notmatch '^(\d+)MB$') { throw 'Unexpected flash size.' }
$flashBytes = [int]$Matches[1]*1MB
if ($previousEnd -gt $flashBytes) { throw 'Images exceed configured flash size.' }
$esptool = Join-Path $properties['tools.esptool_py.path'] $properties['tools.esptool_py.cmd']
if (!(Test-Path $esptool)) { throw 'Cannot locate the esptool selected by Arduino.' }
$merged = Join-Path $build ($config.slug + '.bin')
# No full-flash padding: only the span containing the required images is merged.
# Explicit target-offset=0 makes manifest offset zero a deliberate tool setting.
& $esptool --chip $properties['build.mcu'] merge-bin --format raw --target-offset 0 `
    --output $merged --flash-mode $options['--flash-mode'] --flash-freq $options['--flash-freq'] `
    --flash-size $options['--flash-size'] @mergeParts
if ($LASTEXITCODE -ne 0) { throw 'esptool merge-bin failed.' }
$mergedBytes = [IO.File]::ReadAllBytes($merged)
if ($mergedBytes.Length -lt $previousEnd -or $mergedBytes.Length -gt $flashBytes) { throw 'Invalid merged size.' }
# With the same explicit settings, no header patch should differ from the build.
# Verify every source image at its reported offset in the merged output.
foreach ($part in $parts) {
    $source = [IO.File]::ReadAllBytes((Join-Path $build $part.file))
    $slice = [byte[]]::new($source.Length)
    [Array]::Copy($mergedBytes,[int]$part.offset,$slice,0,$slice.Length)
    if ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($slice)).ToLowerInvariant() -ne $part.sha256) {
        throw "Merged data differs from build image: $($part.file)"
    }
}
$hash = (Get-FileHash $merged).Hash.ToLowerInvariant()
$metadata = [ordered]@{
    name=$config.name; version=$version; chipFamily=$config.chipFamily; chip=$properties['build.mcu'];
    fqbn=$config.fqbn; coreVersion=$platformVersion; tftVersion=$config.tftVersion;
    flashMode=$options['--flash-mode']; flashFrequency=$options['--flash-freq']; flashSize=$options['--flash-size'];
    partitionScheme=$properties['build.partitions']; offsetSource='Arduino build output: flash_args';
    mergedOffset=0; mergedSize=$mergedBytes.Length; sha256=$hash; parts=$parts
}
$manifest = [ordered]@{
    name=$config.name; version=$version; new_install_prompt_erase=$true; new_install_improv_wait_time=0;
    builds=@([ordered]@{chipFamily=$config.chipFamily; parts=@([ordered]@{path=('firmware/'+$config.slug+'.bin');offset=0})})
}
foreach ($dest in @((Join-Path $root 'firmware'),(Join-Path $root 'docs/firmware'))) {
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item $merged (Join-Path $dest ($config.slug+'.bin')) -Force
    $metadata | ConvertTo-Json -Depth 12 | Set-Content (Join-Path $dest 'build-info.json') -Encoding utf8NoBOM
    Copy-Item (Join-Path $build 'partitions.csv') (Join-Path $dest 'partitions.csv') -Force
    Copy-Item $flashArgsPath (Join-Path $dest 'flash_args') -Force
}
$manifest | ConvertTo-Json -Depth 10 | Set-Content (Join-Path $root 'docs/manifest.json') -Encoding utf8NoBOM
& (Join-Path $PSScriptRoot 'Test-Web-Firmware.ps1')
Write-Host "Firmware: firmware/$($config.slug).bin ($($mergedBytes.Length) bytes)"
Write-Host "SHA256: $hash"
Write-Host "Pages copy: docs/firmware/$($config.slug).bin"
