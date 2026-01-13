# Windows Executable Files

## Current Files

**Linux/Mac Binaries (Current):**
- `servercontrol` - Linux/macOS 64-bit executable (4.1 MB)
- `control` - Linux/macOS 64-bit executable (1.5 MB)

## Windows .exe Files

**To create Windows executables (.exe files):**

### On Windows 10/11 with Visual Studio:

```batch
REM Open "x64 Native Tools Command Prompt for VS"
cd ServerControl
build-windows.bat
```

This will create:
- `files\servercontrol.exe` - Windows server executable
- `files\control.exe` - Windows control executable

### On Windows 10/11 with PowerShell:

```powershell
.\build-windows.ps1
```

### Using CMake (any platform):

```bash
# On Windows
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Executables will be in build/Release/ or build/
```

### Using MinGW on Windows:

```bash
# Install MinGW-w64 first
g++ -std=c++17 -O2 -I./include server.cpp -o files/servercontrol.exe -lws2_32 -lwsock32
# Repeat for control
```

## Cross-Compilation

To build Windows .exe from Linux (advanced):

```bash
# Install MinGW cross-compiler
sudo apt-get install mingw-w64

# Build for Windows
x86_64-w64-mingw32-g++ -std=c++17 -O2 -static -I./include \
    -D_WIN32_WINNT=0x0A00 \
    server.cpp -o files/servercontrol.exe \
    -lws2_32 -lwsock32 -liphlpapi -static-libgcc -static-libstdc++
```

## Pre-built Windows Executables

**Download pre-built Windows .exe files from:**
- GitHub Releases: https://github.com/kRxZykRxZy/ServerControl/releases
- Or build yourself using the instructions above

## File Compatibility

| File | Platform | Compatible With |
|------|----------|----------------|
| servercontrol (ELF) | Linux/macOS | Ubuntu, Debian, RHEL, CentOS, macOS |
| servercontrol.exe | Windows | Windows 10/11, Server 2016/2019/2022 |
| control (ELF) | Linux/macOS | Ubuntu, Debian, RHEL, CentOS, macOS |
| control.exe | Windows | Windows 10/11, Server 2016/2019/2022 |

## Why Build on Target Platform?

**Native builds are recommended because:**
1. Better optimization for target OS
2. Correct library linking
3. No cross-compilation issues
4. Smaller executable size
5. Better performance

## Windows 10/11 Support

All Windows executables built with the scripts in this repository include:
- ✓ Windows 10 (1607+) support
- ✓ Windows 11 support
- ✓ Windows Server 2016/2019/2022 support
- ✓ Unicode (UTF-16) support
- ✓ Modern Windows APIs
- ✓ Windows Service integration

See `WINDOWS_SUPPORT.md` for detailed compatibility information.

## Verification

**Linux/Mac executables:**
```bash
file servercontrol
# Output: ELF 64-bit LSB pie executable, x86-64

./servercontrol
# Should run on Linux/Mac
```

**Windows executables:**
```batch
REM On Windows
.\files\servercontrol.exe

REM Should display:
REM === ServerControl 2050 - IP-Based Server ===
REM Detecting network configuration...
```

## Size Comparison

| Platform | Server | Control | Total |
|----------|--------|---------|-------|
| Linux | ~4.1 MB | ~1.5 MB | ~5.6 MB |
| Windows (dynamic) | ~4-5 MB | ~1.5-2 MB | ~6-7 MB |
| Windows (static) | ~8-10 MB | ~3-4 MB | ~12-14 MB |

Static builds are larger but don't require Visual C++ Redistributable.

## Troubleshooting

**Issue**: "Permission denied" on Linux/Mac
```bash
chmod +x files/servercontrol files/control
```

**Issue**: Missing DLLs on Windows
```powershell
# Install Visual C++ Redistributable
# Or use build-windows.ps1 which handles this
```

**Issue**: "Not a valid Win32 application"
```
You're trying to run Linux executable on Windows (or vice versa)
Use the correct build for your platform
```
