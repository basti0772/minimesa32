param([string]$Port = "")
$ErrorActionPreference = "Stop"
$project = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($Port)) {
    $Port = (& "$PSScriptRoot\Find-ESP32Port.ps1").Trim()
}
arduino-cli compile --fqbn esp32:esp32:esp32 $project
if ($LASTEXITCODE -ne 0) { throw "Kompilierung fehlgeschlagen." }
arduino-cli upload -p $Port --fqbn esp32:esp32:esp32 $project
if ($LASTEXITCODE -ne 0) { throw "Upload fehlgeschlagen." }
Write-Host "MiniMesa32 Teapot wurde erfolgreich auf $Port geflasht." -ForegroundColor Green
