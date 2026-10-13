param([int]$Seconds = 10, [int]$Baud = 115200, [string]$Port = "")
$ErrorActionPreference = "Stop"
if ([string]::IsNullOrWhiteSpace($Port)) {
    $Port = (& "$PSScriptRoot\Find-ESP32Port.ps1").Trim()
}
$serial = [System.IO.Ports.SerialPort]::new($Port, $Baud)
$serial.ReadTimeout = 250
$serial.Open()
try {
    $until = [DateTime]::UtcNow.AddSeconds($Seconds)
    while ([DateTime]::UtcNow -lt $until) {
        try { Write-Host -NoNewline $serial.ReadExisting() } catch {}
        Start-Sleep -Milliseconds 50
    }
    Write-Host -NoNewline $serial.ReadExisting()
} finally {
    $serial.Close()
    $serial.Dispose()
}
