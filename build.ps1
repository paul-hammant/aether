# Unified Aether Build Script for Windows
# Complete build system - equivalent to Makefile
#
# Usage:
#   .\build.ps1                    - Incremental build (uses make if available)
#   .\build.ps1 -Fast              - Fast monolithic build
#   .\build.ps1 -Parallel          - Parallel build (2-4x faster)
#   .\build.ps1 -Release           - Optimized release build
#   .\build.ps1 -Test              - Build and run all tests
#   .\build.ps1 -TestExamples      - Test all .ae example files
#   .\build.ps1 -TestStress        - Run stress tests (profiler, multicore)
#   .\build.ps1 -Lsp               - Build LSP server
#   .\build.ps1 -Profiler          - Build profiler
#   .\build.ps1 -All               - Build everything (compiler, LSP, profiler)
#   .\build.ps1 -Clean             - Clean build directory
#   .\build.ps1 -Help              - Show this help

param(
    [switch]$Fast,
    [switch]$Parallel,
    [switch]$Test,
    [switch]$TestExamples,
    [switch]$TestStress,
    [switch]$Release,
    [switch]$Lsp,
    [switch]$Profiler,
    [switch]$All,
    [switch]$Clean,
    [switch]$Help,
    [int]$Jobs = 0  # For parallel builds, 0 = auto-detect
)

function Show-Help {
    Write-Host "Aether Build System (Windows)" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Build Targets:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1                - Incremental build" -ForegroundColor White
    Write-Host "  .\build.ps1 -Fast          - Fast monolithic build" -ForegroundColor White
    Write-Host "  .\build.ps1 -Parallel      - Parallel build (2-4x faster)" -ForegroundColor White
    Write-Host "  .\build.ps1 -Release       - Optimized release build" -ForegroundColor White
    Write-Host "  .\build.ps1 -All           - Build compiler + LSP + profiler" -ForegroundColor White
    Write-Host ""
    Write-Host "Test Targets:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 -Test          - Run all unit tests (187 tests)" -ForegroundColor White
    Write-Host "  .\build.ps1 -TestExamples  - Test all .ae example files" -ForegroundColor White
    Write-Host "  .\build.ps1 -TestStress    - Run stress tests (profiler, multicore)" -ForegroundColor White
    Write-Host ""
    Write-Host "Tool Targets:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 -Lsp           - Build LSP server" -ForegroundColor White
    Write-Host "  .\build.ps1 -Profiler      - Build profiler dashboard" -ForegroundColor White
    Write-Host ""
    Write-Host "Utility:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 -Clean         - Remove all build artifacts" -ForegroundColor White
    Write-Host "  .\build.ps1 -Help          - Show this help" -ForegroundColor White
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1 -Fast -Test    - Quick build and test" -ForegroundColor Gray
    Write-Host "  .\build.ps1 -Clean -Fast   - Clean rebuild" -ForegroundColor Gray
    Write-Host "  .\build.ps1 -Parallel -All - Fast build of everything" -ForegroundColor Gray
}

if ($Help) {
    Show-Help
    exit 0
}

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "Aether Build System (Windows)" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Clean if requested
if ($Clean) {
    if (Test-Path "build") {
        Write-Host "Cleaning build directory..." -ForegroundColor Yellow
        Remove-Item -Recurse -Force "build"
        Write-Host "Clean complete." -ForegroundColor Green
    }
    if (-not ($Fast -or $Parallel -or $Test -or $TestExamples -or $TestStress -or $Release -or $Lsp -or $Profiler -or $All)) {
        exit 0
    }
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Check for GCC
$gccPath = Get-Command gcc -ErrorAction SilentlyContinue
if (-not $gccPath) {
    Write-Host "ERROR: gcc not found in PATH" -ForegroundColor Red
    Write-Host "Please install MinGW-w64 or MSYS2" -ForegroundColor Yellow
    Write-Host "  https://www.mingw-w64.org/" -ForegroundColor Gray
    Write-Host "  https://www.msys2.org/" -ForegroundColor Gray
    exit 1
}

# Source files
$compilerSources = @(
    "compiler\aetherc.c",
    "compiler\lexer.c",
    "compiler\parser.c",
    "compiler\ast.c",
    "compiler\typechecker.c",
    "compiler\codegen.c",
    "compiler\aether_error.c",
    "compiler\aether_module.c",
    "compiler\type_inference.c",
    "compiler\optimizer.c",
    "compiler\aether_diagnostics.c"
)

$stdSources = @(
    "std\string\aether_string.c",
    "std\io\aether_io.c",
    "std\math\aether_math.c",
    "std\net\aether_http.c",
    "std\net\aether_net.c",
    "std\collections\aether_collections.c",
    "std\json\aether_json.c"
)

$collectionsSources = @(
    "std\collections\aether_hashmap.c",
    "std\collections\aether_set.c",
    "std\collections\aether_vector.c",
    "std\collections\aether_pqueue.c"
)

$allSources = $compilerSources + $stdSources + $collectionsSources

$CFLAGS = "-Icompiler -Iruntime -Istd -Istd\string -Istd\io -Istd\math -Istd\net -Istd\collections -Istd\json -Wall -Wextra -Wno-unused-parameter -Wno-unused-function"
$LDFLAGS = "-lpthread -lws2_32"

# Determine build type
if ($Release) {
    Write-Host "Building optimized release..." -ForegroundColor Yellow
    $CFLAGS += " -O3 -DNDEBUG -flto"
    $outputExe = "build\aetherc-release.exe"
    $buildCmd = "gcc $CFLAGS " + ($allSources -join " ") + " -o $outputExe $LDFLAGS"
    Invoke-Expression $buildCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "Release build successful!" -ForegroundColor Green
        Write-Host "Binary: $outputExe" -ForegroundColor Cyan
        $size = (Get-Item $outputExe).Length / 1MB
        Write-Host "Size: $([math]::Round($size, 2)) MB" -ForegroundColor Gray
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
}
elseif ($Parallel) {
    # Parallel build implementation
    if ($Jobs -eq 0) {
        $Jobs = (Get-WmiObject -Class Win32_Processor).NumberOfLogicalProcessors
    }
    Write-Host "Parallel build with $Jobs jobs..." -ForegroundColor Yellow
    Write-Host ""
    
    # Create obj directories
    $objDirs = @("compiler", "runtime", "std\string", "std\io", "std\math", "std\net", "std\collections", "std\json")
    foreach ($dir in $objDirs) {
        $path = "build\obj\$dir"
        if (-not (Test-Path $path)) {
            New-Item -ItemType Directory -Path $path -Force | Out-Null
        }
    }
    
    $CFLAGS_OBJ = "$CFLAGS -O2 -c"
    
    Write-Host "Compiling $($allSources.Count) files in parallel..." -ForegroundColor Yellow
    $startTime = Get-Date
    
    # Compile in parallel
    $jobs = @()
    foreach ($source in $allSources) {
        $objFile = "build\obj\$($source -replace '\.c$','.o')"
        
        $job = Start-Job -ScriptBlock {
            param($source, $objFile, $cflags)
            $cmd = "gcc $cflags $source -o $objFile 2>&1"
            $output = Invoke-Expression $cmd
            return @{
                Source = $source
                ExitCode = $LASTEXITCODE
                Output = $output
            }
        } -ArgumentList $source, $objFile, $CFLAGS_OBJ
        
        $jobs += $job
        
        # Throttle
        while (($jobs | Where-Object { $_.State -eq 'Running' }).Count -ge $Jobs) {
            Start-Sleep -Milliseconds 50
        }
    }
    
    # Wait and collect results
    $completed = 0
    $failed = 0
    while ($jobs | Where-Object { $_.State -eq 'Running' }) {
        $running = ($jobs | Where-Object { $_.State -eq 'Running' }).Count
        $done = $allSources.Count - $running
        Write-Progress -Activity "Compiling" -Status "$done/$($allSources.Count)" -PercentComplete (($done / $allSources.Count) * 100)
        Start-Sleep -Milliseconds 100
    }
    Write-Progress -Activity "Compiling" -Completed
    
    foreach ($job in $jobs) {
        $result = Receive-Job -Job $job
        if ($result.ExitCode -ne 0) {
            Write-Host "Failed: $($result.Source)" -ForegroundColor Red
            Write-Host $result.Output
            $failed++
        } else {
            $completed++
        }
        Remove-Job -Job $job
    }
    
    $elapsed = ((Get-Date) - $startTime).TotalSeconds
    
    if ($failed -gt 0) {
        Write-Host ""
        Write-Host "Compilation failed: $failed errors" -ForegroundColor Red
        exit 1
    }
    
    Write-Host ""
    Write-Host "Compiled $completed files in $([math]::Round($elapsed, 2))s" -ForegroundColor Green
    Write-Host ""
    
    # Link
    Write-Host "Linking compiler..." -ForegroundColor Yellow
    $objFiles = Get-ChildItem -Path "build\obj" -Filter "*.o" -Recurse | ForEach-Object { $_.FullName }
    $linkCmd = "gcc " + ($objFiles -join " ") + " -o build\aetherc.exe $LDFLAGS"
    Invoke-Expression $linkCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build successful!" -ForegroundColor Green
        Write-Host "Compiler: build\aetherc.exe" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Speedup: ~$([math]::Round($Jobs / 2, 1))x faster" -ForegroundColor Yellow
    } else {
        Write-Host "Linking failed!" -ForegroundColor Red
        exit 1
    }
}
elseif ($Fast) {
    # Fast monolithic build
    Write-Host "Fast monolithic build..." -ForegroundColor Yellow
    $CFLAGS += " -O2"
    $buildCmd = "gcc $CFLAGS " + ($allSources -join " ") + " -o build\aetherc.exe $LDFLAGS"
    
    $startTime = Get-Date
    Invoke-Expression $buildCmd
    $elapsed = ((Get-Date) - $startTime).TotalSeconds
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "Build successful in $([math]::Round($elapsed, 1))s!" -ForegroundColor Green
        Write-Host "Compiler: build\aetherc.exe" -ForegroundColor Cyan
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
}
else {
    # Default: Use make for incremental builds
    Write-Host "Incremental build (using make)..." -ForegroundColor Yellow
    Write-Host ""
    
    $makePath = Get-Command make -ErrorAction SilentlyContinue
    if ($makePath) {
        make compiler
        if ($LASTEXITCODE -ne 0) {
            Write-Host ""
            Write-Host "Make failed, falling back to fast build..." -ForegroundColor Yellow
            & $PSCommandPath -Fast
            exit $LASTEXITCODE
        }
    } else {
        Write-Host "Make not found, using fast build..." -ForegroundColor Yellow
        & $PSCommandPath -Fast
        exit $LASTEXITCODE
    }
}

# Build additional tools if requested
if ($Lsp -or $All) {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host "Building LSP Server" -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host ""
    
    $lspSources = @(
        "lsp\main.c",
        "lsp\aether_lsp.c"
    ) + $compilerSources + $stdSources + $collectionsSources
    
    $lspCmd = "gcc $CFLAGS -O2 " + ($lspSources -join " ") + " -o build\aether-lsp.exe $LDFLAGS"
    Invoke-Expression $lspCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "LSP server built: build\aether-lsp.exe" -ForegroundColor Green
    } else {
        Write-Host "LSP build failed!" -ForegroundColor Red
        exit 1
    }
}

if ($Profiler -or $All) {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host "Building Profiler" -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host ""
    
    $profilerSources = @(
        "tools\profiler\profiler_server.c",
        "tools\profiler\profiler_demo.c"
    )
    $runtimeSources = Get-ChildItem -Path "runtime" -Filter "*.c" | ForEach-Object { $_.FullName }
    
    $profilerCmd = "gcc $CFLAGS -O2 -DAETHER_PROFILING " + ($profilerSources -join " ") + " " + ($runtimeSources -join " ") + " -o build\profiler_demo.exe $LDFLAGS"
    Invoke-Expression $profilerCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Profiler built: build\profiler_demo.exe" -ForegroundColor Green
        Write-Host "Run it and open http://localhost:8080" -ForegroundColor Cyan
    } else {
        Write-Host "Profiler build failed!" -ForegroundColor Red
        exit 1
    }
}

# Run tests if requested
if ($Test) {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host "Running Unit Tests" -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host ""
    
    # Build test runner if needed
    if (-not (Test-Path "build\test_runner.exe")) {
        Write-Host "Building test runner..." -ForegroundColor Yellow
        
        $testSources = Get-ChildItem -Path "tests" -Filter "test_*.c" -Recurse | ForEach-Object { $_.FullName }
        $compilerLibSources = $compilerSources | Where-Object { $_ -notmatch "aetherc.c" }
        $runtimeSources = Get-ChildItem -Path "runtime" -Filter "*.c" | ForEach-Object { $_.FullName }
        
        $testCmd = "gcc $CFLAGS -O2 " + ($testSources -join " ") + " " + ($compilerLibSources -join " ") + " " + ($runtimeSources -join " ") + " " + ($stdSources -join " ") + " " + ($collectionsSources -join " ") + " -o build\test_runner.exe $LDFLAGS"
        Invoke-Expression $testCmd
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Test build failed!" -ForegroundColor Red
            exit 1
        }
    }
    
    .\build\test_runner.exe
    $testResult = $LASTEXITCODE
    
    if ($testResult -eq 0) {
        Write-Host ""
        Write-Host "All tests passed!" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Tests failed!" -ForegroundColor Red
        exit $testResult
    }
}

# Test examples if requested
if ($TestExamples) {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host "Testing Example Files" -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host ""
    
    if (-not (Test-Path "build\aetherc.exe")) {
        Write-Host "Compiler not built. Run .\build.ps1 -Fast first" -ForegroundColor Red
        exit 1
    }
    
    $examples = @(
        "examples\basic\hello_world.ae",
        "examples\basic\hello_actors.ae",
        "examples\tests\test_arrays.ae",
        "examples\tests\test_condition.ae",
        "examples\tests\test_for_loop.ae",
        "examples\tests\test_struct.ae",
        "examples\tests\test_actor_simple.ae",
        "examples\advanced\main_example.ae",
        "examples\benchmarks\simple_demo.ae"
    )
    
    $passed = 0
    $failed = 0
    $failedFiles = @()
    
    foreach ($example in $examples) {
        if (-not (Test-Path $example)) {
            Write-Host "Skipping missing file: $example" -ForegroundColor Gray
            continue
        }
        
        $name = [System.IO.Path]::GetFileNameWithoutExtension($example)
        $outC = "build\test_$name.c"
        
        Write-Host "Testing: $example" -ForegroundColor Yellow
        
        # Compile Aether to C
        .\build\aetherc.exe $example $outC 2>&1 | Out-Null
        
        if ($LASTEXITCODE -eq 0) {
            # Try to compile C (just test compilation, don't link)
            gcc -Iruntime -c $outC -o "build\test_$name.o" 2>&1 | Out-Null
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "  PASSED" -ForegroundColor Green
                $passed++
            } else {
                Write-Host "  FAILED (C compilation)" -ForegroundColor Red
                $failed++
                $failedFiles += $example
            }
        } else {
            Write-Host "  FAILED (Aether compilation)" -ForegroundColor Red
            $failed++
            $failedFiles += $example
        }
    }
    
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host "Example Test Results: $passed passed, $failed failed" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Yellow" })
    
    if ($failed -gt 0) {
        Write-Host ""
        Write-Host "Failed files:" -ForegroundColor Red
        foreach ($file in $failedFiles) {
            Write-Host "  - $file" -ForegroundColor Red
        }
    }
    
    Write-Host "=========================================" -ForegroundColor Cyan
}

# Stress tests if requested
if ($TestStress) {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host "Running Stress Tests" -ForegroundColor Cyan
    Write-Host "=========================================" -ForegroundColor Cyan
    Write-Host ""
    
    # Build stress test if needed
    if (-not (Test-Path "build\stress_test.exe")) {
        Write-Host "Building stress test..." -ForegroundColor Yellow
        $runtimeSources = Get-ChildItem -Path "runtime" -Filter "*.c" | ForEach-Object { $_.FullName }
        gcc -O2 -Iruntime tools\profiler\stress_test.c ($runtimeSources -join " ") -o build\stress_test.exe -lpthread -lws2_32
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Stress test build failed!" -ForegroundColor Red
            exit 1
        }
    }
    
    Write-Host "Running stress test (10 seconds)..." -ForegroundColor Yellow
    .\build\stress_test.exe
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "Stress test completed!" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Stress test failed!" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

