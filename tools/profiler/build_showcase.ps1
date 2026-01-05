# Build Profiler Showcase Demo
Write-Host "Building Profiler Showcase Demo" -ForegroundColor Cyan

$gccPath = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gccPath) {
    Write-Host "ERROR: gcc not found" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path "..\..\build")) {
    New-Item -ItemType Directory -Path "..\..\build" | Out-Null
}

$SOURCES = @(
    "profiler_server.c",
    "profiler_showcase_demo.c",
    "..\..\runtime\aether_arena.c",
    "..\..\runtime\aether_pool.c",
    "..\..\runtime\aether_memory_stats.c",
    "..\..\runtime\aether_tracing.c"
)

$CFLAGS = "-O2 -I..\..\runtime -I..\..\std -Wall -Wextra -Wno-unused-parameter -DAETHER_PROFILING"
$LDFLAGS = "-pthread -lws2_32"

Write-Host "Compiling..." -ForegroundColor Yellow
$buildCmd = "gcc $CFLAGS $($SOURCES -join ' ') -o ..\..\build\profiler_showcase.exe $LDFLAGS"
Invoke-Expression $buildCmd

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "Build successful!" -ForegroundColor Green
    Write-Host ""
    Write-Host "To run:" -ForegroundColor Yellow
    Write-Host "  .\build\profiler_showcase.exe" -ForegroundColor White
    Write-Host ""
    Write-Host "Then open browser to:" -ForegroundColor Yellow
    Write-Host "  http://localhost:8080" -ForegroundColor White
    Write-Host ""
} else {
    Write-Host "Build failed" -ForegroundColor Red
    exit 1
}
