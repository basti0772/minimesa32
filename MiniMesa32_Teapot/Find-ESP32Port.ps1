$ErrorActionPreference = "Stop"
$ports = @(Get-CimInstance Win32_SerialPort |
    Where-Object { $_.Name -match "CP210|CH340|USB.*Serial|ESP32" })
if ($ports.Count -eq 0) {
    $ports = @(Get-CimInstance Win32_SerialPort)
}
if ($ports.Count -eq 0) {
    Write-Error "Kein serieller Port gefunden."
    exit 1
}
$match = [regex]::Match($ports[0].DeviceID, "COM\d+")
if (-not $match.Success) {
    Write-Error "COM-Port konnte nicht ermittelt werden."
    exit 1
}
Write-Output $match.Value
