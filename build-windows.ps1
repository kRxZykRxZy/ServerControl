# ServerControl Windows 10/11 PowerShell Build
# Full support for Windows 10 and Windows 11

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  ServerControl Windows 10/11 Build" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check Windows version
$osVersion = [System.Environment]::OSVersion.Version
$isWindows10 = $osVersion.Major -eq 10 -and $osVersion.Build -ge 14393
$isWindows11 = $osVersion.Major -eq 10 -and $osVersion.Build -ge 22000

Write-Host "System Information:" -ForegroundColor Yellow
Write-Host "  OS Version: $($osVersion.Major).$($osVersion.Minor) Build $($osVersion.Build)" -ForegroundColor White

if ($isWindows11) {
    Write-Host "  Platform: Windows 11" -ForegroundColor Green
} elseif ($isWindows10) {
    Write-Host "  Platform: Windows 10" -ForegroundColor Green
} else {
    Write-Host "  Platform: Windows $($osVersion.Major).$($osVersion.Minor)" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "WARNING: This build is optimized for Windows 10/11" -ForegroundColor Yellow
    $continue = Read-Host "Continue anyway? (y/n)"
    if ($continue -ne 'y') {
        exit 1
    }
}

Write-Host ""

# Detect compiler
$compiler = $null
if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    $compiler = "msvc"
    Write-Host "✓ MSVC compiler found" -ForegroundColor Green
} elseif (Get-Command g++.exe -ErrorAction SilentlyContinue) {
    $compiler = "mingw"
    Write-Host "✓ MinGW compiler found" -ForegroundColor Green
} else {
    Write-Host "✗ No C++ compiler found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install one of:" -ForegroundColor Yellow
    Write-Host "  • Visual Studio 2019/2022 with C++ tools" -ForegroundColor White
    Write-Host "  • MinGW-w64" -ForegroundColor White
    exit 1
}

Write-Host ""

# Create output directory
New-Item -ItemType Directory -Force -Path "files" | Out-Null

# Windows 10/11 specific flags
$winFlags = "-D_WIN32_WINNT=0x0A00", "-DWINVER=0x0A00", "-DNTDDI_VERSION=0x0A000000", "-DUNICODE", "-D_UNICODE"

Write-Host "[1/2] Building server for Windows 10/11..." -ForegroundColor Yellow
Write-Host ""

if ($compiler -eq "msvc") {
    $flags = "/EHsc", "/std:c++17", "/I.\include"
    $flags += $winFlags
    
    if ($Configuration -eq "Release") {
        $flags += "/O2", "/MD"
    } else {
        $flags += "/Od", "/MDd", "/Zi"
    }
    
    & cl.exe $flags /Fe:files\servercontrol.exe server.cpp /link ws2_32.lib wsock32.lib iphlpapi.lib 2>&1 | Out-Null
} else {
    $flags = "-std=c++17", "-pthread", "-I./include"
    $flags += $winFlags
    
    if ($Configuration -eq "Release") {
        $flags += "-O2"
    } else {
        $flags += "-g"
    }
    
    & g++.exe $flags server.cpp -o files/servercontrol.exe -lws2_32 -lwsock32 -liphlpapi 2>&1 | Out-Null
}

if ($LASTEXITCODE -eq 0) {
    $size = (Get-Item "files\servercontrol.exe").Length / 1MB
    Write-Host ("✓ Server built successfully ({0:N2} MB)" -f $size) -ForegroundColor Green
} else {
    Write-Host "✗ Server build failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[2/2] Building control for Windows 10/11..." -ForegroundColor Yellow
Write-Host ""

Set-Location control

if ($compiler -eq "msvc") {
    $flags = "/EHsc", "/std:c++17", "/I..\include"
    $flags += $winFlags
    
    if ($Configuration -eq "Release") {
        $flags += "/O2", "/MD"
    } else {
        $flags += "/Od", "/MDd", "/Zi"
    }
    
    & cl.exe $flags /Fe:..\files\control.exe `
        main.cpp ui\UI.cpp core\TaskManager.cpp config\Config.cpp `
        network\Server.cpp http\HttpClient.cpp `
        /link ws2_32.lib wsock32.lib iphlpapi.lib 2>&1 | Out-Null
} else {
    $flags = "-std=c++17", "-pthread", "-I../include"
    $flags += $winFlags
    
    if ($Configuration -eq "Release") {
        $flags += "-O2"
    } else {
        $flags += "-g"
    }
    
    & g++.exe $flags `
        main.cpp ui/UI.cpp core/TaskManager.cpp config/Config.cpp `
        network/Server.cpp http/HttpClient.cpp `
        -o ../files/control.exe -lws2_32 -lwsock32 -liphlpapi 2>&1 | Out-Null
}

Set-Location ..

if ($LASTEXITCODE -eq 0) {
    $size = (Get-Item "files\control.exe").Length / 1MB
    Write-Host ("✓ Control built successfully ({0:N2} MB)" -f $size) -ForegroundColor Green
} else {
    Write-Host "✗ Control build failed" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Display compatibility info
Write-Host "Windows 10/11 Executables:" -ForegroundColor White
Write-Host "  files\servercontrol.exe" -ForegroundColor Cyan
Write-Host "  files\control.exe" -ForegroundColor Cyan
Write-Host ""

Write-Host "Compatible with:" -ForegroundColor White
Write-Host "  ✓ Windows 10 (1607 Anniversary Update and later)" -ForegroundColor Green
Write-Host "  ✓ Windows 10 versions: 1809, 1903, 1909, 2004, 20H2, 21H1, 21H2, 22H2" -ForegroundColor Green
Write-Host "  ✓ Windows 11 (all versions: 21H2, 22H2, 23H2)" -ForegroundColor Green
Write-Host "  ✓ Windows Server 2016, 2019, 2022" -ForegroundColor Green
Write-Host ""

Write-Host "Features:" -ForegroundColor White
Write-Host "  • Native Windows 10/11 APIs" -ForegroundColor Gray
Write-Host "  • Unicode support (UTF-16)" -ForegroundColor Gray
Write-Host "  • Windows Sockets 2.2" -ForegroundColor Gray
Write-Host "  • IP Helper API" -ForegroundColor Gray
Write-Host "  • Modern Windows subsystem" -ForegroundColor Gray
Write-Host ""

Write-Host "Next Steps:" -ForegroundColor Yellow
Write-Host "  • Test:    .\files\servercontrol.exe" -ForegroundColor Gray
Write-Host "  • Deploy:  .\deploy.ps1 -Service -AutoStart" -ForegroundColor Gray
Write-Host "  • Package: .\build-all.ps1 -Package" -ForegroundColor Gray
Write-Host ""
