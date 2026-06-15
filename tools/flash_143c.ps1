#!/usr/bin/env pwsh
# Flash books (LittleFS) and firmware to the Waveshare AMOLED 1.43C
# Usage: .\tools\flash_143c.ps1

$env_name   = "waveshare_esp32s3_touch_amoled_143c"
$stale      = "src\storage\SeedBooks.generated.h"
$data_dir   = "data"

# LittleFS partition size from partitions_143c.csv (0x260000 bytes)
# Reserve ~10% for LittleFS metadata/journal overhead
$partition_bytes = 0x260000
$usable_bytes    = [int]($partition_bytes * 0.90)

# Remove stale generated file if left over from old builds
if (Test-Path $stale) {
    Write-Host "==> Removing stale $stale..." -ForegroundColor Yellow
    Remove-Item $stale -Force
}

# --- Rename files with spaces (LittleFS does not support spaces in filenames) ---
Write-Host "==> Checking for spaces in filenames..." -ForegroundColor Cyan
$spaceFiles = Get-ChildItem -Path $data_dir -Recurse -File |
              Where-Object { $_.Name -match ' ' }
if ($spaceFiles) {
    foreach ($f in $spaceFiles) {
        $newName = $f.Name -replace ' ', '_'
        $newPath = Join-Path $f.DirectoryName $newName
        Write-Host "   Renaming: $($f.Name) -> $newName" -ForegroundColor Yellow
        Rename-Item -Path $f.FullName -NewName $newName -Force
    }
    Write-Host "   Done renaming." -ForegroundColor Green
} else {
    Write-Host "   No spaces found." -ForegroundColor Green
}

# --- Space check ---
Write-Host ""
Write-Host "==> Checking available space..." -ForegroundColor Cyan
$files = Get-ChildItem -Path $data_dir -Recurse -File -ErrorAction SilentlyContinue
$total_bytes = ($files | Measure-Object -Property Length -Sum).Sum
if (-not $total_bytes) { $total_bytes = 0 }

$total_kb    = [math]::Round($total_bytes / 1KB, 1)
$usable_kb   = [math]::Round($usable_bytes / 1KB, 1)
$partition_kb = [math]::Round($partition_bytes / 1KB, 1)
$pct         = if ($usable_bytes -gt 0) { [math]::Round($total_bytes / $usable_bytes * 100, 1) } else { 0 }

Write-Host "   Partition : $partition_kb KB  (usable ~$usable_kb KB after LittleFS overhead)"
Write-Host "   Data size : $total_kb KB  ($pct% of usable space)"

if ($total_bytes -gt $usable_bytes) {
    Write-Host ""
    Write-Host "ERROR: Data ($total_kb KB) exceeds usable LittleFS space ($usable_kb KB)." -ForegroundColor Red
    Write-Host "       Remove or split some books in $data_dir\books\books\" -ForegroundColor Red
    exit 1
} elseif ($pct -gt 80) {
    Write-Host "   WARNING: space is above 80% - consider removing some books." -ForegroundColor Yellow
} else {
    Write-Host "   Space OK." -ForegroundColor Green
}

# --- Flash filesystem ---
Write-Host ""
Write-Host "==> Flashing filesystem (books)..." -ForegroundColor Cyan
pio run -e $env_name -t uploadfs
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: uploadfs failed" -ForegroundColor Red
    exit 1
}

# --- Flash firmware ---
Write-Host ""
Write-Host "==> Flashing firmware..." -ForegroundColor Cyan
pio run -e $env_name -t upload
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: upload failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Done! Books and firmware flashed successfully." -ForegroundColor Green
