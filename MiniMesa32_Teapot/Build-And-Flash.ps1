param([string]$Port = "")
$ErrorActionPreference = "Stop"
$project = $PSScriptRoot
$repoRoot = Split-Path $project -Parent
$webBuilder = Join-Path $repoRoot 'tools/Build-Web-Firmware.ps1'
$uploadArgs = @('upload','--fqbn','esp32:esp32:esp32')
if (Test-Path $webBuilder) {
    # Shared build, repository-local display config, no global library edits.
    & pwsh -NoProfile -File $webBuilder
    if ($LASTEXITCODE -ne 0) { throw "Web-Firmware-Build fehlgeschlagen." }
    $uploadArgs += @('--input-dir',(Join-Path $repoRoot '.build/web'))
} else {
    # Standalone Arduino ZIP: configure TFT_eSPI as described in README.md.
    arduino-cli compile --fqbn esp32:esp32:esp32 $project
    if ($LASTEXITCODE -ne 0) { throw "Kompilierung fehlgeschlagen." }
}
if ([string]::IsNullOrWhiteSpace($Port)) {
    $Port = (& "$PSScriptRoot\Find-ESP32Port.ps1").Trim()
}
arduino-cli @uploadArgs -p $Port $project
if ($LASTEXITCODE -ne 0) { throw "Upload fehlgeschlagen." }
Write-Host "MiniMesa32 Teapot wurde erfolgreich auf $Port geflasht." -ForegroundColor Green
