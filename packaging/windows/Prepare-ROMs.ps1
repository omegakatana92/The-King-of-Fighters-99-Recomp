# Prepare-ROMs.ps1 - build a runtime-ready KOF '99 ROM set from your own dumps.
#
# This script extracts the MAME kof99 and neogeo sets and runs the bundled
# decrypt tool.  It does not download anything and does not contain ROM data.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1
#   powershell -ExecutionPolicy Bypass -File .\Prepare-ROMs.ps1 -Kof99Zip C:\path\kof99.zip -NeoGeoZip C:\path\neogeo.zip

param(
    [string]$Kof99Zip = "",
    [string]$NeoGeoZip = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $OutDir) { $OutDir = Join-Path $here "roms" }
$src = Join-Path $here "roms-src"

function Ask-Zip($label, $current) {
    if ($current -and (Test-Path $current)) { return (Resolve-Path $current).Path }
    while ($true) {
        $p = Read-Host "Path to $label"
        $p = $p.Trim('"').Trim()
        if (Test-Path $p) { return (Resolve-Path $p).Path }
        Write-Host "Not found: $p" -ForegroundColor Yellow
    }
}

$Kof99Zip = Ask-Zip "kof99.zip" $Kof99Zip
$NeoGeoZip = Ask-Zip "neogeo.zip" $NeoGeoZip

if (-not (Test-Path (Join-Path $here "kof99_decrypt.exe"))) {
    throw "kof99_decrypt.exe is missing from $here"
}

New-Item -ItemType Directory -Force $src | Out-Null
New-Item -ItemType Directory -Force $OutDir | Out-Null

Write-Host "Extracting $Kof99Zip"
tar -xf $Kof99Zip -C $src
if ($LASTEXITCODE -ne 0) { throw "Could not extract kof99.zip with tar" }
Write-Host "Extracting $NeoGeoZip"
tar -xf $NeoGeoZip -C $src
if ($LASTEXITCODE -ne 0) { throw "Could not extract neogeo.zip with tar" }

Write-Host "Decrypting into $OutDir"
& (Join-Path $here "kof99_decrypt.exe") --rom-path $src --out $OutDir
if ($LASTEXITCODE -ne 0) { throw "kof99_decrypt failed" }

Write-Host ""
Write-Host "Done. Run:" -ForegroundColor Green
Write-Host "  .\`"KOF 99.exe`" --rom-path `"$OutDir`""
