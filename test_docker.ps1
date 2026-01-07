#!/usr/bin/env pwsh
# Test Docker Build and Makefile

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Aether Docker & Makefile Test Suite" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"

# Check if Docker is available
Write-Host "[1/5] Checking Docker installation..." -ForegroundColor Yellow
$docker = Get-Command docker -ErrorAction SilentlyContinue
if (-not $docker) {
    Write-Host "ERROR: Docker not found. Please install Docker Desktop." -ForegroundColor Red
    Write-Host "  Download: https://www.docker.com/products/docker-desktop/" -ForegroundColor Gray
    exit 1
}
Write-Host "SUCCESS: Docker found at $($docker.Source)" -ForegroundColor Green

# Check Docker daemon
Write-Host "`n[2/5] Checking Docker daemon..." -ForegroundColor Yellow
try {
    docker info | Out-Null
    Write-Host "SUCCESS: Docker daemon is running" -ForegroundColor Green
} catch {
    Write-Host "ERROR: Docker daemon not running. Please start Docker Desktop." -ForegroundColor Red
    exit 1
}

# Build Docker image
Write-Host "`n[3/5] Building Docker image..." -ForegroundColor Yellow
Write-Host "This will take 1-2 minutes on first run..." -ForegroundColor Gray
docker build -t aether:test -f Dockerfile . --quiet
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Docker build failed" -ForegroundColor Red
    exit 1
}
Write-Host "SUCCESS: Docker image built" -ForegroundColor Green

# Test make in Docker
Write-Host "`n[4/5] Testing make clean && make compiler in Docker..." -ForegroundColor Yellow
docker run --rm -v ${PWD}:/aether -w /aether aether:test /bin/bash -c "make clean && make -j4 compiler"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Make build failed in Docker" -ForegroundColor Red
    exit 1
}
Write-Host "SUCCESS: Make build completed" -ForegroundColor Green

# Test compiler
Write-Host "`n[5/5] Testing compiler in Docker..." -ForegroundColor Yellow
$version = docker run --rm -v ${PWD}:/aether -w /aether aether:test /aether/build/aetherc --version
Write-Host "Compiler version: $version" -ForegroundColor Cyan

if ($version -like "*Aether Compiler*") {
    Write-Host "SUCCESS: Compiler works in Docker" -ForegroundColor Green
} else {
    Write-Host "ERROR: Compiler version check failed" -ForegroundColor Red
    exit 1
}

# Summary
Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host "  All Tests Passed!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Docker environment is ready. You can now:" -ForegroundColor White
Write-Host "  • docker run -it -v ${PWD}:/aether aether:test /bin/bash" -ForegroundColor Gray
Write-Host "  • docker-compose up -d aether-dev" -ForegroundColor Gray
Write-Host "  • docker run -v ${PWD}:/aether aether:test make test" -ForegroundColor Gray
Write-Host ""
