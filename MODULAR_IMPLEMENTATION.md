# Windows-Friendly Modularization - Implementation Summary

## Overview

ServerControl has been fully modularized with Windows/Linux cross-platform support and enhanced with multiple WebSocket servers for specialized communication channels.

## New Requirement: Automatic IP Assignment

**Yes, the server will automatically assign itself an IP in the 10.125.125.x range on Windows.**

### How It Works:

1. **On Startup** (Windows only):
   - The server scans the 10.125.125.1 through 10.125.125.254 range
   - It pings each IP to check availability
   - Selects the **first available IP** (starting from 10.125.125.1)
   - Uses `netsh` to assign that IP to the network interface
   - Binds all services (HTTP, WebSocket, Discovery) to that IP

2. **Example Scenario**:
   - First server starts → gets 10.125.125.1
   - Second server starts → gets 10.125.125.2
   - Third server starts → gets 10.125.125.3
   - And so on...

3. **Fallback**:
   - If assignment fails (no admin privileges), falls back to existing IP
   - If all IPs in range are taken, falls back to existing IP
   - On Linux, you must manually configure IP addresses

4. **Manual Assignment** (if needed):
   ```cmd
   # Run as Administrator on Windows
   netsh interface ip add address "Ethernet" 10.125.125.1 255.255.255.0
   ```

## Modular Architecture

### Server Modules

```
server/
├── platform/              # Cross-platform abstraction
│   ├── PlatformAbstraction.h
│   └── PlatformAbstraction.cpp
│       ├── Linux implementation (using /proc, popen, etc.)
│       └── Windows implementation (using WinAPI, WMI, etc.)
├── websocket/             # WebSocket server management
│   ├── WebSocketManager.h
│   └── WebSocketManager.cpp
│       └── Manages 4 WebSocket servers per host
├── stats/                 # System statistics monitoring
│   ├── StatsMonitor.h
│   └── StatsMonitor.cpp
│       └── CPU/RAM monitoring with alerts
├── tasks/                 # Command execution
│   ├── TaskExecutor.h
│   └── TaskExecutor.cpp
│       └── Cross-platform command execution
├── files/                 # File operations (planned)
│   └── FileManager.h
├── http/                  # HTTP server (to be extracted)
└── main_modular.cpp       # Main entry point using all modules
```

### Four WebSocket Servers Per Host

Each server instance now runs **four separate WebSocket servers** on different ports:

1. **Main Control WebSocket** (port 2040)
   - Task execution commands
   - Task output streaming
   - General control messages

2. **Stats/Monitoring WebSocket** (port 2041)
   - Real-time CPU/RAM updates (every 1 second)
   - CPU alert notifications (when > 90%)
   - System health metrics

3. **File Operations WebSocket** (port 2042)
   - File upload/download streaming
   - File operation notifications
   - Reserved for future file sync

4. **Remote Desktop WebSocket** (port 2043)
   - Reserved for remote desktop framebuffer streaming
   - Mouse/keyboard event handling
   - Screen capture streaming

### Port Configuration

- **HTTP API**: 2030
- **UDP Discovery**: 2031
- **WebSocket Main**: 2040
- **WebSocket Stats**: 2041
- **WebSocket Files**: 2042
- **WebSocket Desktop**: 2043

All services bind to the assigned 10.125.125.x IP address.

## Control Application Updates

### Subnet Scanning

The control application now:
- Scans the **10.125.125.x subnet** specifically (10.125.125.1-254)
- Sends UDP discovery packets to each IP
- Also broadcasts to the general network (255.255.255.255)
- Discovers all four WebSocket ports per server

### Server Discovery Response

Discovery responses now include all WebSocket ports:
```json
{
  "type": "SERVER_RESPONSE",
  "hostname": "server01",
  "port": 2030,
  "ws_main": 2040,
  "ws_stats": 2041,
  "ws_files": 2042,
  "ws_desktop": 2043,
  "discovery_port": 2031
}
```

### UI Color Scheme

Updated to **Codeberg-inspired palette**:
- **Color 1 (Green)**: Success/Online - Codeberg green (#21ba45)
- **Color 2 (Red)**: Error/Critical - Codeberg red (#db2828)
- **Color 3 (Yellow)**: Warning - Codeberg orange approximation (#f2711c)
- **Color 4 (Cyan)**: Info/Primary - Codeberg teal/blue (#00b5ad/#2185d0)
- **Color 5 (White on Blue)**: Selected - Codeberg primary blue

## Platform Abstraction

### Windows-Specific Implementations

All Linux-specific code has been replaced with Windows equivalents:

| Function | Linux | Windows |
|----------|-------|---------|
| CPU Stats | `/proc/stat` | `GetSystemTimes()` |
| RAM Stats | `/proc/meminfo` | `GlobalMemoryStatusEx()` |
| Command Execution | `popen()` | `_popen()` |
| Kill Process | `kill -9` | `TerminateProcess()` |
| Shutdown | `shutdown -h now` | `shutdown /s /t 10` |
| Reboot | `reboot` | `shutdown /r /t 10` |
| OS Info | `/etc/os-release` | WMI/Registry |
| Network Interfaces | `ip addr` | `GetAdaptersAddresses()` |
| IP Assignment | N/A | `netsh interface ip add` |

## Building

### Linux
```bash
./build_modular.sh
```

### Windows
```cmd
build_modular.bat  # To be created
```

Or use CMake for cross-platform builds.

## Running

### Server
```bash
# Linux
./files/servercontrol_modular

# Windows (as Administrator for IP assignment)
.\files\servercontrol_modular.exe
```

### Control
```bash
# Linux
./files/control

# Windows
.\files\control.exe
```

## Features Summary

✅ **Completed**:
- Platform abstraction layer (Windows/Linux)
- Four WebSocket servers per host
- Automatic 10.125.125.x IP assignment (Windows)
- Stats monitoring with CPU alerts
- Task executor with callbacks
- Control app scans 10.125.125.x subnet
- Codeberg color palette in UI
- Modular server architecture

🚧 **In Progress**:
- Complete file manager module
- HTTP server module extraction
- System manager module

📋 **Planned**:
- Remote desktop streaming implementation
- File sync over WebSocket
- Cluster management features

## Notes

- IP assignment requires **Administrator privileges** on Windows
- On Linux, manual IP configuration is needed
- The subnet 10.125.125.0/24 provides 254 usable host addresses
- Each server gets a unique IP automatically on Windows
- All four WebSockets are automatically discovered by control app
