# Automated Build Script for UEFI Nyancat Player
$ErrorActionPreference = "Stop"

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   Building UEFI Nyancat Player..." -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# 1. Generate assets using Python
Write-Host "[1/3] Generating Video Asset (nyancat_frames.h)..." -ForegroundColor Yellow
python compress_gif.py

Write-Host "[2/3] Generating Audio Asset (nyancat_audio.h)..." -ForegroundColor Yellow
python parse_audio.py

# 2. Compile C code to EFI binary using MinGW-w64 GCC
Write-Host "[3/3] Compiling C source to UEFI binary..." -ForegroundColor Yellow
gcc -O3 -shared -nostdlib -mno-red-zone -fno-stack-protector -fno-builtin "-Wl,--entry=efi_main" "-Wl,--subsystem,10" -o nyancat.efi main.c

Write-Host ""
Write-Host "=========================================" -ForegroundColor Green
Write-Host "   BUILD SUCCESSFUL!" -ForegroundColor Green
Write-Host "   Output: nyancat.efi" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
