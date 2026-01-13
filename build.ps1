# PowerShell build script for ServerControl on Windows
# Supports both MSVC (cl.exe) and MinGW (g++)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  ServerControl Windows Build" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Detect compiler
$compiler = $null
$compilerName = ""

if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    $compiler = "msvc"
    $compilerName = "MSVC (cl.exe)"
} elseif (Get-Command g++.exe -ErrorAction SilentlyContinue) {
    $compiler = "mingw"
    $compilerName = "MinGW (g++)"
} else {
    Write-Host "ERROR: No C++ compiler found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install one of:" -ForegroundColor Yellow
    Write-Host "  1. Visual Studio with C++ tools (cl.exe)" -ForegroundColor Yellow
    Write-Host "  2. MinGW-w64 (g++.exe)" -ForegroundColor Yellow
    exit 1
}

Write-Host "Detected compiler: $compilerName" -ForegroundColor Green
Write-Host ""

# Build server
Write-Host "[1/2] Building server..." -ForegroundColor Yellow

if ($compiler -eq "msvc") {
    cl.exe /EHsc /std:c++17 /MD /O2 /I.\include `
        /Fe:servercontrol.exe `
        server.cpp `
        /link ws2_32.lib 2>&1 | Out-Null
} else {
    g++.exe -std=c++17 -O2 -pthread -I./include `
        server.cpp `
        -o servercontrol.exe `
        -lws2_32 2>&1 | Out-Null
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Server build failed" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Server built successfully" -ForegroundColor Green

# Build control
Write-Host "[2/2] Building control..." -ForegroundColor Yellow

Set-Location control

if ($compiler -eq "msvc") {
    cl.exe /EHsc /std:c++17 /MD /O2 /I..\include `
        /Fe:control.exe `
        main.cpp `
        ui\UI.cpp `
        core\TaskManager.cpp `
        config\Config.cpp `
        network\Server.cpp `
        http\HttpClient.cpp `
        /link ws2_32.lib 2>&1 | Out-Null
} else {
    g++.exe -std=c++17 -O2 -pthread -I../include `
        main.cpp `
        ui/UI.cpp `
        core/TaskManager.cpp `
        config/Config.cpp `
        network/Server.cpp `
        http/HttpClient.cpp `
        -o control.exe `
        -lws2_32 2>&1 | Out-Null
}

Set-Location ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Control build failed" -ForegroundColor Red
    exit 1
}

Write-Host "✓ Control built successfully" -ForegroundColor Green
Write-Host ""

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Binaries:" -ForegroundColor White
Write-Host "  Server:  servercontrol.exe" -ForegroundColor Cyan
Write-Host "  Control: control\control.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "Usage:" -ForegroundColor White
Write-Host "  .\servercontrol.exe          # Run on each server" -ForegroundColor Yellow
Write-Host "  .\control\control.exe        # Run on admin machine" -ForegroundColor Yellow
