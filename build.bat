@echo off
REM Build script for ServerControl on Windows using MSVC (cl.exe)
REM Requires Visual Studio 2019 or later with C++ development tools

echo ========================================
echo   ServerControl Windows Build (MSVC)
echo ========================================
echo.

REM Check if cl.exe is available
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: cl.exe not found. Please run this from a Visual Studio Developer Command Prompt.
    echo.
    echo To setup environment, run one of:
    echo   - "x64 Native Tools Command Prompt for VS 2019"
    echo   - "x64 Native Tools Command Prompt for VS 2022"
    echo   - Or run: "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    exit /b 1
)

echo [1/3] Building server...
echo.

REM Build server
cl.exe /EHsc /std:c++17 /MD /O2 /I.\include ^
    /Fe:servercontrol.exe ^
    server.cpp ^
    /link ws2_32.lib

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Server build failed
    exit /b 1
)

echo.
echo [2/3] Building control application...
echo.

REM Build control
cd control
cl.exe /EHsc /std:c++17 /MD /O2 /I..\include ^
    /Fe:control.exe ^
    main.cpp ^
    ui\UI.cpp ^
    core\TaskManager.cpp ^
    config\Config.cpp ^
    network\Server.cpp ^
    http\HttpClient.cpp ^
    /link ws2_32.lib

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Control build failed
    cd ..
    exit /b 1
)
cd ..

echo.
echo [3/3] Build complete!
echo.
echo Binaries created:
echo   Server:  servercontrol.exe
echo   Control: control\control.exe
echo.
echo ========================================
echo   Build successful!
echo ========================================
