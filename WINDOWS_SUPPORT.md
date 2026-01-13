# Windows 10/11 Compatibility Guide

## Supported Windows Versions

### Windows 10
- **Minimum**: Windows 10 Version 1607 (Anniversary Update) Build 14393
- **Recommended**: Windows 10 Version 22H2 or later

**Tested on:**
- ✓ Windows 10 Home
- ✓ Windows 10 Pro
- ✓ Windows 10 Enterprise
- ✓ Windows 10 Education
- ✓ Windows 10 LTSC

**Version compatibility:**
- ✓ 1607 Anniversary Update (Build 14393)
- ✓ 1703 Creators Update (Build 15063)
- ✓ 1709 Fall Creators Update (Build 16299)
- ✓ 1803 April 2018 Update (Build 17134)
- ✓ 1809 October 2018 Update (Build 17763)
- ✓ 1903 May 2019 Update (Build 18362)
- ✓ 1909 November 2019 Update (Build 18363)
- ✓ 2004 May 2020 Update (Build 19041)
- ✓ 20H2 October 2020 Update (Build 19042)
- ✓ 21H1 May 2021 Update (Build 19043)
- ✓ 21H2 November 2021 Update (Build 19044)
- ✓ 22H2 October 2022 Update (Build 19045)

### Windows 11
- **All versions supported**

**Tested on:**
- ✓ Windows 11 Home
- ✓ Windows 11 Pro
- ✓ Windows 11 Enterprise
- ✓ Windows 11 Education

**Version compatibility:**
- ✓ 21H2 (Build 22000)
- ✓ 22H2 (Build 22621)
- ✓ 23H2 (Build 22631)

### Windows Server
- ✓ Windows Server 2016
- ✓ Windows Server 2019
- ✓ Windows Server 2022
- ✓ Windows Server Core editions

## Technical Requirements

### System Requirements
- **OS**: Windows 10 (1607+) or Windows 11
- **Architecture**: x64 (64-bit)
- **RAM**: 512 MB minimum, 2 GB recommended
- **Disk**: 50 MB for installation
- **Network**: TCP/IP stack

### Dependencies
**Included in Windows 10/11:**
- Windows Sockets 2.2 (WS2_32.DLL)
- IP Helper API (IPHLPAPI.DLL)
- Windows Subsystem APIs

**No additional runtime required!**

### Build Dependencies (for compilation)
**Option 1: Visual Studio**
- Visual Studio 2019 or 2022
- Desktop development with C++
- Windows 10/11 SDK

**Option 2: MinGW**
- MinGW-w64 (w64devkit recommended)
- GCC 9.0 or later

**Option 3: CMake**
- CMake 3.15 or later
- Any C++17 compatible compiler

## Build Instructions

### Using PowerShell (Recommended for Windows 10/11)

```powershell
# Build for Windows 10/11
.\build-windows.ps1

# Build with specific configuration
.\build-windows.ps1 -Configuration Release
```

### Using Command Prompt

```batch
# Build for Windows 10/11
build-windows.bat
```

### Using CMake

```batch
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Windows 10/11 Specific Features

### API Support
- **Windows Sockets 2**: Full async networking
- **IP Helper API**: Network interface enumeration
- **Unicode**: Full UTF-16 support
- **Windows Events**: Native event logging
- **Windows Services**: Service integration

### Optimizations
- **Windows 10/11 APIs**: Uses modern Windows APIs
- **Thread Pool**: Windows thread pool for efficiency
- **Completion Ports**: I/O completion ports for networking
- **Memory**: Large page support where available

### Security
- **DEP**: Data Execution Prevention enabled
- **ASLR**: Address Space Layout Randomization
- **SafeSEH**: Safe Structured Exception Handling
- **Control Flow Guard**: Available on Windows 10/11

## Installation

### Standard Installation

```powershell
# Copy executables
Copy-Item files\servercontrol.exe C:\ServerControl\
Copy-Item files\control.exe C:\ServerControl\
```

### Windows Service Installation

```powershell
# Install as Windows Service
.\deploy.ps1 -Component Server -Service -AutoStart

# Verify installation
Get-Service ServerControl
```

### Portable Installation

1. Copy `files\servercontrol.exe` and `files\control.exe` to any folder
2. Run directly - no installation needed
3. Configuration stored in `%APPDATA%\ServerControl\`

## Firewall Configuration

### Windows Defender Firewall

**Automatic (with deploy.ps1):**
```powershell
.\deploy.ps1 -Service
# Firewall rules created automatically
```

**Manual:**
```powershell
# Allow server
New-NetFirewallRule -DisplayName "ServerControl Server" `
    -Direction Inbound -Protocol TCP -LocalPort 2030 -Action Allow

# Allow WebSocket
New-NetFirewallRule -DisplayName "ServerControl WebSocket" `
    -Direction Inbound -Protocol TCP -LocalPort 2032 -Action Allow

# Allow Discovery
New-NetFirewallRule -DisplayName "ServerControl Discovery" `
    -Direction Inbound -Protocol UDP -LocalPort 2031 -Action Allow
```

## Troubleshooting

### Windows 10 Issues

**Issue**: "The application was unable to start correctly (0xc0000142)"
**Solution**: Install Visual C++ Redistributable 2015-2022
```powershell
# Download from Microsoft
# https://aka.ms/vs/17/release/vc_redist.x64.exe
```

**Issue**: "VCRUNTIME140.dll was not found"
**Solution**: Rebuild with static linking
```powershell
.\build-windows.ps1
# Or install VC++ Redistributable
```

### Windows 11 Issues

**Issue**: Windows Security blocking execution
**Solution**: Add exclusion
```powershell
Add-MpPreference -ExclusionPath "C:\ServerControl"
```

**Issue**: SmartScreen warning
**Solution**: Click "More info" then "Run anyway"
- Or: Sign executable (for production deployment)

### Common Issues

**Issue**: Port already in use
**Solution**: Server auto-detects and uses alternate port
- Default: 2030, 2031, 2032
- Will try 2030-2040 range

**Issue**: Network interface not found
**Solution**: Server will bind to first available interface
- Check: `ipconfig /all`
- Verify: Network adapter is enabled

## Performance Tuning

### Windows 10

```powershell
# Disable Nagle's algorithm for low latency
# Already optimized in ServerControl

# Increase network buffer
netsh int tcp set global autotuninglevel=normal

# Enable RSS (Receive Side Scaling)
Enable-NetAdapterRss -Name "*"
```

### Windows 11

```powershell
# Windows 11 has better defaults
# No additional tuning needed for most cases

# For extreme performance:
netsh int tcp set global chimney=enabled
netsh int tcp set global dca=enabled
netsh int tcp set global netdma=enabled
```

## Compatibility Matrix

| Feature | Win 10 1607 | Win 10 22H2 | Win 11 | Win Server 2022 |
|---------|------------|-------------|---------|-----------------|
| Server | ✓ | ✓ | ✓ | ✓ |
| Control | ✓ | ✓ | ✓ | ✓ |
| WebSocket | ✓ | ✓ | ✓ | ✓ |
| Discovery | ✓ | ✓ | ✓ | ✓ |
| Service | ✓ | ✓ | ✓ | ✓ |
| Remote Desktop | ✓ | ✓ | ✓ | ✓ |
| Performance | Good | Better | Best | Best |

## Version Detection

ServerControl automatically detects Windows version:

```
System Information:
  OS Version: 10.0 Build 22621
  Platform: Windows 11
  
✓ Full Windows 11 support enabled
```

## Updates

**Windows Update compatibility:**
- ServerControl works through Windows Updates
- No rebuild needed after Windows updates
- Service auto-restarts if needed

## Support

For Windows-specific issues:
1. Check Event Viewer (Windows Logs > Application)
2. Enable debug logging
3. Check firewall rules
4. Verify Windows version compatibility

## License

ServerControl is compatible with:
- Windows 10 licensing (Home, Pro, Enterprise, Education)
- Windows 11 licensing (all editions)
- Windows Server licensing (2016, 2019, 2022)

No additional licenses required for ServerControl itself.
