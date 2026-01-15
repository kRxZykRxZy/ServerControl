# UI Screenshots and Visual Documentation

## Control Application UI

### Main Server List View

```
╔═════════════════════════════════════════════════════════════╗
║ ⚡ QUANTUM SERVER CONTROL NEXUS 2050 ⚡                      ║
╚═════════════════════════════════════════════════════════════╝
  ↑/↓=Navigate | SPACE=Select | ENTER=Access | R=Sync | ESC=Exit

┌───────────────────────────────────────────────────────────┐
│ > server01        [10.125.125.1]  CPU: 15.2%  RAM:512/2048MB │  ← Codeberg Blue (Selected)
│   server02        [10.125.125.2]  CPU: 45.1%  RAM:1024/4096MB│  ← Codeberg Green (Online)
│   server03        [10.125.125.3]  CPU: 92.5%  RAM:3500/4096MB│  ← Codeberg Red (Alert!)
│   server04        [10.125.125.4]  CPU: 8.0%   RAM:256/1024MB │  ← Codeberg Green (Online)
└───────────────────────────────────────────────────────────┘

⚡ Quantum-Selected Nodes: 1
🌐 Neural Network: ACTIVE
📊 Live Stats: 4 WebSocket streams active (Main, Stats, Files, Desktop)
```

**Color Scheme (Codeberg Palette):**
- **Blue (#2185d0)** - Selected server, info messages
- **Green (#21ba45)** - Online servers, success messages
- **Red (#db2828)** - CPU alerts, errors, critical warnings
- **Orange/Yellow (#f2711c)** - Warnings, moderate alerts
- **Teal/Cyan (#00b5ad)** - Info panels, borders

### Server Details with WebSocket Status

```
╔═══════════════════════════════════════════════════════════╗
║ 📊 SERVER: server01 [10.125.125.1:2030]                  ║
╚═══════════════════════════════════════════════════════════╝

WebSocket Connections:
  🎮 Main Control    → ws://10.125.125.1:2040  [CONNECTED]
  📊 Stats/Monitor   → ws://10.125.125.1:2041  [CONNECTED]
  📁 File Operations → ws://10.125.125.1:2042  [CONNECTED]
  🖥️ Remote Desktop  → ws://10.125.125.1:2043  [CONNECTED]

System Stats (Real-time):
  CPU: ████████░░░░░░░░░░ 45.2%
  RAM: ██████████████░░░░ 1024/2048 MB (50%)
  
Actions: [T]erminal | [F]iles | [L]ogs | [K]ill | [S]tats | [ESC]Back
```

## Modular Server Startup (Console Output)

### Windows Startup with Auto IP Assignment

```
=== ServerControl 2050 - Modular Windows/Linux Server ===
Detecting network configuration...
Platform: Windows

Scanning 10.125.125.x subnet for available IP...
  Testing 10.125.125.1... [IN USE]
  Testing 10.125.125.2... [IN USE]
  Testing 10.125.125.3... [AVAILABLE]
✓ Found available IP: 10.125.125.3

Attempting to assign IP via netsh...
  Executing: netsh interface ip add address "Ethernet" 10.125.125.3 255.255.255.0
✓ Successfully assigned IP address: 10.125.125.3
  Server will bind to this IP for all services

✓ Network Configuration:
  Hostname:       DESKTOP-SERVER03
  Bind IP:        10.125.125.3
  IP Assignment:  Automatic (10.125.125.x)
  HTTP API:       10.125.125.3:2030
  UDP Discovery:  10.125.125.3:2031

Initializing WebSocket servers...
WebSocket server (Main Control) started on port 2040
WebSocket server (Stats/Monitoring) started on port 2041
WebSocket server (File Operations) started on port 2042
WebSocket server (Remote Desktop) started on port 2043
  Main Control:   10.125.125.3:2040
  Stats/Monitor:  10.125.125.3:2041
  File Ops:       10.125.125.3:2042
  Remote Desktop: 10.125.125.3:2043

Starting stats monitor...
UDP Discovery listening on port 2031

✓ All services started successfully!
✓ CPU monitoring active - will alert when CPU > 90%
✓ Server bound to: 10.125.125.3:2030

Waiting for connections...
```

### Linux Startup (Manual IP Configuration)

```
=== ServerControl 2050 - Modular Windows/Linux Server ===
Detecting network configuration...
Platform: Linux
Note: Automatic IP assignment is currently Windows-only
      On Linux, manually configure IP with: sudo ip addr add 10.125.125.x/24 dev eth0

✓ Network Configuration:
  Hostname:       ubuntu-server01
  Bind IP:        192.168.1.100
  IP Assignment:  Using existing network configuration
  HTTP API:       192.168.1.100:2030
  UDP Discovery:  192.168.1.100:2031

Initializing WebSocket servers...
WebSocket server (Main Control) started on port 2040
WebSocket server (Stats/Monitoring) started on port 2041
WebSocket server (File Operations) started on port 2042
WebSocket server (Remote Desktop) started on port 2043
  Main Control:   192.168.1.100:2040
  Stats/Monitor:  192.168.1.100:2041
  File Ops:       192.168.1.100:2042
  Remote Desktop: 192.168.1.100:2043

Starting stats monitor...
UDP Discovery listening on port 2031

✓ All services started successfully!
✓ CPU monitoring active - will alert when CPU > 90%
✓ Server bound to: 192.168.1.100:2030

Waiting for connections...
```

## Control App Discovery Process

```
Scanning for servers on network...
Scanning 10.125.125.x subnet...
  Sending discovery to 10.125.125.1...
  Sending discovery to 10.125.125.2...
  Sending discovery to 10.125.125.3...
  ...
  Sending discovery to 10.125.125.254...

✓ Discovered server: server01 at 10.125.125.1:2030 
  (WS: 2040,2041,2042,2043)
✓ Discovered server: server02 at 10.125.125.2:2030 
  (WS: 2040,2041,2042,2043)
✓ Discovered server: server03 at 10.125.125.3:2030 
  (WS: 2040,2041,2042,2043)

Total servers discovered: 3
Connecting to WebSocket streams...
```

## Live Stats Monitoring (WebSocket Stream)

```
[Stats WebSocket - Port 2041]

{"type":"stats_update","hostname":"server01","cpu":15.2,"ram_used":512,"ram_total":2048,"timestamp":1705338478123}
{"type":"stats_update","hostname":"server01","cpu":16.1,"ram_used":515,"ram_total":2048,"timestamp":1705338479124}
{"type":"stats_update","hostname":"server01","cpu":18.5,"ram_used":520,"ram_total":2048,"timestamp":1705338480125}
{"type":"stats_update","hostname":"server01","cpu":91.2,"ram_used":1800,"ram_total":2048,"timestamp":1705338481126}
{"type":"cpu_alert","cpu":91.2,"hostname":"server01","message":"CPU usage exceeded 90%!","timestamp":1705338481126}
⚠️  CPU ALERT: 91.2% on server01
```

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                   CONTROL APPLICATION                       │
│  ┌────────────────────────────────────────────────────────┐ │
│  │         Codeberg-Styled Terminal UI (ncurses)          │ │
│  └────────────────────────────────────────────────────────┘ │
│                            │                                │
│  ┌─────────────────────────┴──────────────────────────────┐ │
│  │           10.125.125.x Subnet Scanner (UDP)            │ │
│  └─────────────────────────┬──────────────────────────────┘ │
│                            │                                │
│  ┌─────────────────────────┴──────────────────────────────┐ │
│  │    4 WebSocket Clients per Server (WS Connections)     │ │
│  │  🎮 Main  📊 Stats  📁 Files  🖥️ Desktop              │ │
│  └────────────────────────────────────────────────────────┘ │
└──────────────────────────┬──────────────────────────────────┘
                           │ Network
━━━━━━━━━━━━━━━━━━━━━━━━━━━┿━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   ┌────▼─────┐      ┌────▼─────┐      ┌────▼─────┐
   │ Server 1 │      │ Server 2 │      │ Server 3 │
   │10.125.  │      │10.125.  │      │10.125.  │
   │125.1     │      │125.2     │      │125.3     │
   └──────────┘      └──────────┘      └──────────┘
        │                  │                  │
   ┌────┴─────┐      ┌────┴─────┐      ┌────┴─────┐
   │Platform  │      │Platform  │      │Platform  │
   │Abstract  │      │Abstract  │      │Abstract  │
   └──────────┘      └──────────┘      └──────────┘
        │                  │                  │
   ┌────┴─────┐      ┌────┴─────┐      ┌────┴─────┐
   │4x WebSkt │      │4x WebSkt │      │4x WebSkt │
   │Servers   │      │Servers   │      │Servers   │
   │2040-2043 │      │2040-2043 │      │2040-2043 │
   └──────────┘      └──────────┘      └──────────┘
```

## Key Visual Features

1. **Codeberg Color Palette:**
   - Primary Blue (#2185d0) for selected items and info
   - Success Green (#21ba45) for online status and success
   - Error Red (#db2828) for alerts and errors
   - Warning Orange (#f2711c) for warnings
   - Info Teal (#00b5ad) for informational elements

2. **Unicode Box Drawing:**
   - Clean, professional borders using ═, ║, ╔, ╗, ╚, ╝, ├, ┤, ┬, ┴, ┼
   - Progress bars using █, ▓, ▒, ░

3. **Real-Time Updates:**
   - Stats update every second via dedicated WebSocket
   - Live terminal output streaming
   - Instant CPU alerts when threshold exceeded

4. **Multi-Server Support:**
   - Each server automatically gets unique 10.125.125.x IP
   - All 4 WebSocket servers discovered per host
   - Color-coded status indicators

## Screenshots

_Note: Since this is a terminal application, actual screenshots would show:_
- Full-screen terminal UI with Unicode characters
- Color-coded server listings
- Real-time updating stats
- Codeberg-themed color scheme
- Clean, professional layout

To capture actual screenshots, run the application on:
- **Windows Terminal** (best colors and Unicode support)
- **Linux terminal** with 256-color support
- Take screenshots using Snipping Tool (Windows) or `scrot` (Linux)
