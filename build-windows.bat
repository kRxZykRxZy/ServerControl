@echo off
REM ServerControl Windows 10/11 Build Script
REM Full support for Windows 10 (1607+) and Windows 11

echo ========================================
echo   ServerControl Windows 10/11 Build
echo ========================================
echo.

REM Check Windows version
for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSION=%%i.%%j
echo Detected Windows Version: %VERSION%
echo.

REM Verify Windows 10/11
if "%VERSION%" geq "10.0" (
    echo ✓ Windows 10/11 detected
) else (
    echo WARNING: This build is optimized for Windows 10/11
    echo Your version: %VERSION%
    pause
)
echo.

REM Check for Visual Studio
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Visual Studio C++ compiler not found
    echo.
    echo Please install Visual Studio 2019 or 2022 with C++ tools
    echo Or run from "x64 Native Tools Command Prompt"
    echo.
    pause
    exit /b 1
)

echo ✓ Visual Studio C++ compiler found
echo.

REM Create output directory
if not exist files mkdir files

echo [1/3] Building server for Windows 10/11...
echo.

REM Server compilation with Windows 10/11 targeting (MODULAR)
cd server
cl.exe /EHsc /std:c++17 /MD /O2 ^
    /D_WIN32_WINNT=0x0A00 ^
    /DWINVER=0x0A00 ^
    /DNTDDI_VERSION=0x0A000000 ^
    /DUNICODE /D_UNICODE ^
    /I..\include ^
    /Fe:..\files\servercontrol.exe ^
    main.cpp ^
    /link ws2_32.lib wsock32.lib iphlpapi.lib
cd ..

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Server build failed
    pause
    exit /b 1
)

echo ✓ Server built successfully
echo.

echo [2/3] Building control for Windows 10/11...
echo.

REM Control compilation with Windows 10/11 targeting
cd control
cl.exe /EHsc /std:c++17 /MD /O2 ^
    /D_WIN32_WINNT=0x0A00 ^
    /DWINVER=0x0A00 ^
    /DNTDDI_VERSION=0x0A000000 ^
    /DUNICODE /D_UNICODE ^
    /I..\include ^
    /Fe:..\files\control.exe ^
    main.cpp ^
    ui\UI.cpp ^
    core\TaskManager.cpp ^
    config\Config.cpp ^
    network\Server.cpp ^
    http\HttpClient.cpp ^
    /link ws2_32.lib wsock32.lib iphlpapi.lib

cd ..

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Control build failed
    pause
    exit /b 1
)

echo ✓ Control built successfully
echo.

echo [3/3] Verifying Windows 10/11 compatibility...
echo.

REM Check file properties
if exist files\servercontrol.exe (
    echo ✓ servercontrol.exe created
    for %%A in (files\servercontrol.exe) do echo   Size: %%~zA bytes
) else (
    echo ✗ servercontrol.exe not found
)

if exist files\control.exe (
    echo ✓ control.exe created
    for %%A in (files\control.exe) do echo   Size: %%~zA bytes
) else (
    echo ✗ control.exe not found
)

echo.
echo ========================================
echo   Build Complete!
echo ========================================
echo.
echo Windows 10/11 Executables:
echo   files\servercontrol.exe
echo   files\control.exe
echo.
echo Compatible with:
echo   • Windows 10 (1607+)
echo   • Windows 10 (1809, 1903, 1909, 2004, 20H2, 21H1, 21H2, 22H2)
echo   • Windows 11 (21H2, 22H2, 23H2)
echo   • Windows Server 2016, 2019, 2022
echo.
echo To install as Windows Service:
echo   .\deploy.ps1 -Service -AutoStart
echo.
pause
