# PowerShell build script for ServerControl Modular

Write-Host "Building ServerControl (Fully Modular for Windows)..." -ForegroundColor Cyan

# Create files directory if it doesn't exist
if (-not (Test-Path "files")) {
    New-Item -ItemType Directory -Path "files" | Out-Null
}

Write-Host "Building modular server..." -ForegroundColor Yellow
Push-Location server

# Build with g++ (MinGW) or cl (MSVC)
$compiler = Get-Command g++ -ErrorAction SilentlyContinue
if ($compiler) {
    Write-Host "Using MinGW g++ compiler..." -ForegroundColor Green
    g++ -std=c++17 -DASIO_STANDALONE -D_WIN32_WINNT=0x0601 -I../include `
        main_modular.cpp `
        platform/PlatformAbstraction.cpp `
        websocket/WebSocketManager.cpp `
        stats/StatsMonitor.cpp `
        tasks/TaskExecutor.cpp `
        -o ../files/servercontrol_modular.exe -lws2_32 -liphlpapi -lpsapi
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ Build successful!" -ForegroundColor Green
    } else {
        Write-Host "✗ Build failed!" -ForegroundColor Red
    }
} else {
    Write-Host "✗ g++ not found. Please install MinGW or MSVC." -ForegroundColor Red
    Write-Host "  Download from: https://www.mingw-w64.org/" -ForegroundColor Yellow
}

Pop-Location

if (Test-Path "files/servercontrol_modular.exe") {
    Write-Host ""
    Write-Host "Build complete!" -ForegroundColor Green
    Write-Host "  Modular Server binary: .\files\servercontrol_modular.exe" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To run the server with automatic IP assignment:" -ForegroundColor Yellow
    Write-Host "  1. Open PowerShell as Administrator" -ForegroundColor White
    Write-Host "  2. Run: .\files\servercontrol_modular.exe" -ForegroundColor White
    Write-Host ""
    Write-Host "The server will automatically assign itself an IP in 10.125.125.x range" -ForegroundColor Gray
    Get-Item files/servercontrol_modular.exe | Format-Table Name, Length, LastWriteTime
} else {
    Write-Host ""
    Write-Host "Build incomplete - binary not found" -ForegroundColor Red
}
