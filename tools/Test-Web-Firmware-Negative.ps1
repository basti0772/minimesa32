#requires -Version 7.0
# Isolated, intentionally broken fixtures; never modifies published files.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$testRoot = Join-Path $root ('.build/tests-' + [Guid]::NewGuid().ToString('N'))
$cases = @('wrong-chip','wrong-offset','wrong-path','wrong-version','corrupt-binary')
foreach ($case in $cases) {
    $fixture = Join-Path $testRoot $case
    New-Item -ItemType Directory -Path $fixture -Force | Out-Null
    Copy-Item (Join-Path $root 'VERSION') $fixture
    Copy-Item (Join-Path $root 'firmware') $fixture -Recurse
    Copy-Item (Join-Path $root 'docs') $fixture -Recurse
    $manifestPath = Join-Path $fixture 'docs/manifest.json'
    $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
    switch ($case) {
        'wrong-chip' { $manifest.builds[0].chipFamily = 'ESP32-S3' }
        'wrong-offset' { $manifest.builds[0].parts[0].offset = 4096 }
        'wrong-path' { $manifest.builds[0].parts[0].path = '../firmware/minimesa32.bin' }
        'wrong-version' { $manifest.version = '0.0.0' }
        'corrupt-binary' {
            $path = Join-Path $fixture 'docs/firmware/minimesa32.bin'
            $bytes = [IO.File]::ReadAllBytes($path)
            $bytes[0] = $bytes[0] -bxor 1
            [IO.File]::WriteAllBytes($path,$bytes)
        }
    }
    $manifest | ConvertTo-Json -Depth 10 | Set-Content $manifestPath -Encoding utf8NoBOM
    $rejected = $false
    try { & (Join-Path $PSScriptRoot 'Test-Web-Firmware.ps1') -RepositoryRoot $fixture }
    catch { $rejected = $true; Write-Host "PASS: rejected $case ($($_.Exception.Message))" }
    if (!$rejected) { throw "Validation incorrectly accepted $case" }
}
