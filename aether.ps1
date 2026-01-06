# Aether CLI - Professional command-line interface
# Usage: .\aether.ps1 [command] [args]

param(
    [Parameter(Position=0)]
    [string]$Command = "repl",
    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$Args
)

$VERSION = "0.4.0"
$ROOT = $PSScriptRoot

function Show-Help {
    Write-Host @"
Aether Language CLI v$VERSION

Usage: .\aether.ps1 <command> [options]

Commands:
  repl                 Start interactive REPL (default)
  run <file>           Compile and run an Aether file
  compile <file>       Compile Aether to C
  build                Build the compiler
  test                 Run all tests
  clean                Clean build artifacts
  version              Show version
  help                 Show this help

Examples:
  .\aether.ps1                          # Start REPL
  .\aether.ps1 run examples/hello.ae    # Run a file
  .\aether.ps1 test                     # Run tests
  .\aether.ps1 compile myprogram.ae     # Compile to C

"@ -ForegroundColor Cyan
}

switch ($Command.ToLower()) {
    "repl" {
        if (-not (Test-Path "$ROOT\build\aether-repl.exe")) {
            Write-Host "Building REPL..." -ForegroundColor Yellow
            gcc -o "$ROOT\build\aether-repl.exe" "$ROOT\tools\aether_repl.c" -O2 -Wall
        }
        & "$ROOT\build\aether-repl.exe"
    }
    
    "run" {
        if ($Args.Count -eq 0) {
            Write-Host "Error: No file specified" -ForegroundColor Red
            Write-Host "Usage: .\aether.ps1 run <file>"
            exit 1
        }
        $file = $Args[0]
        if (-not (Test-Path $file)) {
            Write-Host "Error: File not found: $file" -ForegroundColor Red
            exit 1
        }
        Write-Host "Compiling and running: $file" -ForegroundColor Cyan
        $output = [System.IO.Path]::GetFileNameWithoutExtension($file)
        & "$ROOT\build\aetherc.exe" $file -o "$output.c"
        gcc -O2 "$output.c" -o "$output.exe" -I"$ROOT\runtime" -I"$ROOT\std" "$ROOT\runtime\*.c" "$ROOT\std\*\*.c" -lm -lpthread
        & ".\$output.exe"
    }
    
    "compile" {
        if ($Args.Count -eq 0) {
            Write-Host "Error: No file specified" -ForegroundColor Red
            exit 1
        }
        $file = $Args[0]
        $output = [System.IO.Path]::GetFileNameWithoutExtension($file) + ".c"
        & "$ROOT\build\aetherc.exe" $file -o $output
        Write-Host "Output: $output" -ForegroundColor Green
    }
    
    "build" {
        & "$ROOT\build.ps1"
    }
    
    "test" {
        if (-not (Test-Path "$ROOT\build\test_runner.exe")) {
            Write-Host "Building tests..." -ForegroundColor Yellow
            & "$ROOT\build.ps1" -Test
        }
        & "$ROOT\build\test_runner.exe"
    }
    
    "clean" {
        Write-Host "Cleaning..." -ForegroundColor Cyan
        Remove-Item "$ROOT\build\*.exe", "$ROOT\build\*.o" -ErrorAction SilentlyContinue
        Write-Host "Done!" -ForegroundColor Green
    }
    
    "version" {
        Write-Host "Aether Language v$VERSION" -ForegroundColor Cyan
    }
    
    "help" {
        Show-Help
    }
    
    default {
        Write-Host "Unknown command: $Command" -ForegroundColor Red
        Show-Help
        exit 1
    }
}
