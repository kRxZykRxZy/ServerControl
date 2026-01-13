# ServerControl Build and Test Script
# Comprehensive PowerShell script for building, testing, and packaging

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    
    [Parameter(Mandatory=$false)]
    [switch]$RunTests,
    
    [Parameter(Mandatory=$false)]
    [switch]$Package,
    
    [Parameter(Mandatory=$false)]
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Colors
function Write-Success { param([string]$Message) Write-Host "✓ $Message" -ForegroundColor Green }
function Write-Info { param([string]$Message) Write-Host "ℹ $Message" -ForegroundColor Cyan }
function Write-Warning { param([string]$Message) Write-Host "⚠ $Message" -ForegroundColor Yellow }
function Write-Error { param([string]$Message) Write-Host "✗ $Message" -ForegroundColor Red }

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  ServerControl Build System" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Info "Configuration: $Configuration"
Write-Host ""

# Clean build
if ($Clean) {
    Write-Info "Cleaning build artifacts..."
    Remove-Item -Recurse -Force ".\build" -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force ".\files" -ErrorAction SilentlyContinue
    Remove-Item "*.exe", "*.obj", "*.pdb", "*.ilk" -ErrorAction SilentlyContinue
    Remove-Item "control\*.exe", "control\*.obj" -ErrorAction SilentlyContinue
    Write-Success "Clean complete"
    Write-Host ""
}

# Create directories
Write-Info "Creating build directories..."
New-Item -ItemType Directory -Force -Path ".\build" | Out-Null
New-Item -ItemType Directory -Force -Path ".\files" | Out-Null
Write-Success "Directories created"
Write-Host ""

# Detect compiler
Write-Info "Detecting C++ compiler..."
$compiler = $null
$compilerVersion = ""

if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    $compiler = "msvc"
    $compilerInfo = cl.exe 2>&1 | Select-String "Version"
    $compilerVersion = "MSVC $compilerInfo"
    Write-Success "Found MSVC (cl.exe)"
} elseif (Get-Command g++.exe -ErrorAction SilentlyContinue) {
    $compiler = "mingw"
    $compilerVersion = g++.exe --version | Select-Object -First 1
    Write-Success "Found MinGW (g++.exe)"
} elseif (Get-Command cmake -ErrorAction SilentlyContinue) {
    $compiler = "cmake"
    Write-Success "Found CMake (will auto-detect compiler)"
} else {
    Write-Error "No C++ compiler found!"
    Write-Host ""
    Write-Warning "Please install one of:"
    Write-Host "  • Visual Studio 2019/2022 with C++ tools"
    Write-Host "  • MinGW-w64"
    Write-Host "  • CMake"
    exit 1
}

Write-Info "Using: $compilerVersion"
Write-Host ""

# Build with CMake if available
if ($compiler -eq "cmake" -or (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Info "Building with CMake..."
    
    Set-Location build
    cmake .. -DCMAKE_BUILD_TYPE=$Configuration
    cmake --build . --config $Configuration
    Set-Location ..
    
    # Copy binaries
    if (Test-Path ".\build\$Configuration\servercontrol.exe") {
        Copy-Item ".\build\$Configuration\servercontrol.exe" ".\files\" -Force
        Copy-Item ".\build\$Configuration\control.exe" ".\files\" -Force
    } else {
        Copy-Item ".\build\servercontrol.exe" ".\files\" -Force
        Copy-Item ".\build\control.exe" ".\files\" -Force
    }
    
    Write-Success "CMake build complete"
} else {
    # Direct compilation
    Write-Info "[1/2] Building server..."
    
    $serverArgs = @()
    $controlArgs = @()
    
    if ($compiler -eq "msvc") {
        $commonFlags = "/EHsc", "/std:c++17", "/I.\include"
        if ($Configuration -eq "Release") {
            $commonFlags += "/O2", "/MD"
        } else {
            $commonFlags += "/Od", "/MDd", "/Zi"
        }
        
        $serverArgs = $commonFlags + @("/Fe:files\servercontrol.exe", "server.cpp", "/link", "ws2_32.lib")
        $controlArgs = $commonFlags + @("/Fe:files\control.exe", 
                                        "control\main.cpp",
                                        "control\ui\UI.cpp",
                                        "control\core\TaskManager.cpp",
                                        "control\config\Config.cpp",
                                        "control\network\Server.cpp",
                                        "control\http\HttpClient.cpp",
                                        "/link", "ws2_32.lib")
        
        & cl.exe $serverArgs 2>&1 | Out-Null
    } else {
        # MinGW
        $commonFlags = "-std=c++17", "-pthread", "-I./include"
        if ($Configuration -eq "Release") {
            $commonFlags += "-O2"
        } else {
            $commonFlags += "-g"
        }
        
        $serverArgs = $commonFlags + @("server.cpp", "-o", "files/servercontrol.exe", "-lws2_32")
        $controlArgs = $commonFlags + @("control/main.cpp",
                                        "control/ui/UI.cpp",
                                        "control/core/TaskManager.cpp",
                                        "control/config/Config.cpp",
                                        "control/network/Server.cpp",
                                        "control/http/HttpClient.cpp",
                                        "-o", "files/control.exe",
                                        "-lws2_32")
        
        & g++.exe $serverArgs 2>&1 | Out-Null
    }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Server built successfully"
    } else {
        Write-Error "Server build failed"
        exit 1
    }
    
    Write-Info "[2/2] Building control..."
    
    if ($compiler -eq "msvc") {
        & cl.exe $controlArgs 2>&1 | Out-Null
    } else {
        & g++.exe $controlArgs 2>&1 | Out-Null
    }
    
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Control built successfully"
    } else {
        Write-Error "Control build failed"
        exit 1
    }
}

Write-Host ""

# Get file sizes
if (Test-Path ".\files\servercontrol.exe") {
    $serverSize = (Get-Item ".\files\servercontrol.exe").Length / 1MB
    $controlSize = (Get-Item ".\files\control.exe").Length / 1MB
    
    Write-Info "Binary sizes:"
    Write-Host ("  Server:  {0:N2} MB" -f $serverSize) -ForegroundColor White
    Write-Host ("  Control: {0:N2} MB" -f $controlSize) -ForegroundColor White
    Write-Host ""
}

# Run tests
if ($RunTests) {
    Write-Info "Running tests..."
    Write-Host ""
    
    if (Test-Path ".\tests.ps1") {
        & .\tests.ps1
    } else {
        Write-Warning "Test script not found (tests.ps1)"
    }
    
    Write-Host ""
}

# Package
if ($Package) {
    Write-Info "Creating distribution package..."
    
    $version = "2.0.0"
    $packageName = "ServerControl-$version-Windows"
    $packageDir = ".\build\$packageName"
    
    # Create package directory
    New-Item -ItemType Directory -Force -Path $packageDir | Out-Null
    
    # Copy files
    Copy-Item ".\files\*" $packageDir -Recurse -Force
    Copy-Item ".\README.md" $packageDir -Force -ErrorAction SilentlyContinue
    Copy-Item ".\ARCHITECTURE.md" $packageDir -Force -ErrorAction SilentlyContinue
    Copy-Item ".\deploy.ps1" $packageDir -Force -ErrorAction SilentlyContinue
    
    # Create README
    @"
ServerControl v$version
======================

Windows Distribution

Files:
  servercontrol.exe - Server component (run on managed machines)
  control.exe       - Control panel (run on admin machine)

Installation:
  1. Run deploy.ps1 as Administrator
  2. Or manually copy files to desired location

Usage:
  Server:  .\servercontrol.exe
  Control: .\control.exe

Documentation:
  See README.md and ARCHITECTURE.md

"@ | Out-File "$packageDir\INSTALL.txt" -Encoding UTF8
    
    # Create ZIP
    $zipPath = ".\build\$packageName.zip"
    Compress-Archive -Path $packageDir -DestinationPath $zipPath -Force
    
    $zipSize = (Get-Item $zipPath).Length / 1MB
    Write-Success ("Package created: $packageName.zip ({0:N2} MB)" -f $zipSize)
    Write-Host ""
}

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Output files:" -ForegroundColor White
Write-Host "  .\files\servercontrol.exe" -ForegroundColor Cyan
Write-Host "  .\files\control.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  • Run tests:       .\build-all.ps1 -RunTests" -ForegroundColor Gray
Write-Host "  • Create package:  .\build-all.ps1 -Package" -ForegroundColor Gray
Write-Host "  • Deploy:          .\deploy.ps1" -ForegroundColor Gray
Write-Host ""
