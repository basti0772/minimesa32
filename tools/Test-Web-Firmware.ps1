#requires -Version 7.0
[CmdletBinding()]
param([string]$RepositoryRoot = (Split-Path $PSScriptRoot -Parent))
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $RepositoryRoot
$cfg = Get-Content (Join-Path $PSScriptRoot 'web-firmware.json') -Raw | ConvertFrom-Json
$version = (Get-Content (Join-Path $root 'VERSION') -Raw).Trim()
$docs = Join-Path $root 'docs'
$manifest = Get-Content (Join-Path $docs 'manifest.json') -Raw | ConvertFrom-Json
$info = Get-Content (Join-Path $root 'firmware/build-info.json') -Raw | ConvertFrom-Json
if ($manifest.name -ne $cfg.name -or $manifest.version -ne $version -or $info.version -ne $version) { throw 'Name/version mismatch.' }
if ($manifest.builds.Count -ne 1 -or $manifest.builds[0].chipFamily -ne $cfg.chipFamily -or $info.chip -ne 'esp32') { throw 'Wrong chip family.' }
if ($manifest.builds[0].parts.Count -ne 1) { throw 'Expected one merged image.' }
$part = $manifest.builds[0].parts[0]
if ($part.offset -ne 0 -or $info.mergedOffset -ne 0) { throw 'Wrong merged offset.' }
$expectedPath = 'firmware/'+$cfg.slug+'.bin'
if ($part.path -ne $expectedPath) { throw 'Manifest must resolve inside the /docs publishing root.' }
$siteURL = [Uri]'https://basti0772.github.io/minimesa32/manifest.json'
$imageURL = [Uri]::new($siteURL,$part.path)
if ($imageURL.AbsolutePath -ne '/minimesa32/'+$expectedPath) { throw 'Incorrect Pages-relative path.' }
$siteFile = Join-Path $docs $part.path
$canonical = Join-Path $root $expectedPath
foreach ($path in @($siteFile,$canonical)) {
    if (!(Test-Path $path)) { throw "Missing firmware: $path" }
    if ((Get-Item $path).Length -ne $info.mergedSize -or (Get-FileHash $path).Hash.ToLowerInvariant() -ne $info.sha256) { throw 'Firmware size/hash mismatch.' }
}
foreach ($name in @('build-info.json','partitions.csv','flash_args')) {
    if ((Get-FileHash (Join-Path $root "firmware/$name")).Hash -ne (Get-FileHash (Join-Path $docs "firmware/$name")).Hash) { throw "Pages metadata mismatch: $name" }
}
$image = [IO.File]::ReadAllBytes($canonical)
$imageText = [Text.Encoding]::ASCII.GetString($image)
if ($imageText -match '[A-Za-z]:[\\/](?:Users|Windows|Program Files)[\\/]') {
    throw 'Firmware contains local Windows paths. Check compiler prefix maps.'
}
$end = 0
foreach ($source in $info.parts) {
    if ($source.offset -lt $end -or $source.size -le 0 -or $source.offset+$source.size -gt $image.Length) { throw 'Invalid source image extent.' }
    $slice = [byte[]]::new($source.size)
    [Array]::Copy($image,[int]$source.offset,$slice,0,$slice.Length)
    if ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($slice)).ToLowerInvariant() -ne $source.sha256) { throw "Bad source image at $($source.offsetHex)" }
    $end = $source.offset+$source.size
}
$argsText = Get-Content (Join-Path $root 'firmware/flash_args')
foreach ($source in $info.parts) {
    $found = $false
    foreach ($line in $argsText) {
        if ($line.Trim() -match '^(0x[0-9A-Fa-f]+|\d+)\s+(\S+\.bin)$') {
            $offset = if ($Matches[1].StartsWith('0x')) { [Convert]::ToInt32($Matches[1].Substring(2),16) } else { [int]$Matches[1] }
            if ($offset -eq $source.offset -and $Matches[2] -eq $source.file) { $found=$true }
        }
    }
    if (!$found) { throw 'Source offset not found in build flash_args.' }
}
$html = Get-Content (Join-Path $docs 'index.html') -Raw
if ($html -notmatch 'esp-web-install-button' -or $html -notmatch 'manifest="manifest.json"') { throw 'Installer element missing.' }
Write-Host "PASS: $($cfg.chipFamily), version $version, $($info.mergedSize) bytes; image hashes, offsets and Pages URL valid."
Write-Host $imageURL.AbsoluteUri
