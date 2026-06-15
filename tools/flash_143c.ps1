#!/usr/bin/env pwsh
# Flash books (LittleFS) and firmware to the Waveshare AMOLED 1.43C
# Usage: .\tools\flash_143c.ps1

$env_name = "waveshare_esp32s3_touch_amoled_143c"

Write-Host "==> Flashing filesystem (books)..." -ForegroundColor Cyan
pio run -e $env_name -t uploadfs
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: uploadfs failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "==> Flashing firmware..." -ForegroundColor Cyan
pio run -e $env_name -t upload
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: upload failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Done! Books and firmware flashed successfully." -ForegroundColor Green
