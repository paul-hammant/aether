# Build Aether Profiler (Windows PowerShell)

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Building Aether Profiler Dashboard" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Check for gcc
$gccPath = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gccPath) {
    Write-Host "ERROR: gcc (MinGW) not found in PATH" -ForegroundColor Red
    exit 1
}

# Create build directory
if (-not (Test-Path "..\..\build")) {
    New-Item -ItemType Directory -Path "..\..\build" | Out-Null
}

$PROFILER_SRC = @(
    "profiler_server.c",
    "profiler_demo.c",
    "..\..\runtime\aether_arena.c",
    "..\..\runtime\aether_pool.c",
    "..\..\runtime\aether_memory_stats.c",
    "..\..\runtime\aether_tracing.c"
)

$CFLAGS = "-O2 -I..\..\runtime -I..\..\std -Wall -Wextra -Wno-unused-parameter -DAETHER_PROFILING"
$LDFLAGS = "-pthread -lws2_32"

Write-Host "Compiling profiler demo..." -ForegroundColor Yellow
$buildCmd = "gcc $CFLAGS $($PROFILER_SRC -join ' ') -o ..\..\build\profiler_demo.exe $LDFLAGS"
Write-Host $buildCmd -ForegroundColor Gray
Invoke-Expression $buildCmd

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "✅ Build successful!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Run the demo:" -ForegroundColor Cyan
    Write-Host "  .\build\profiler_demo.exe" -ForegroundColor White
    Write-Host ""
    Write-Host "Then open your browser to:" -ForegroundColor Cyan
    Write-Host "  http://localhost:8080" -ForegroundColor White
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "❌ Build failed" -ForegroundColor Red
    exit 1
}

