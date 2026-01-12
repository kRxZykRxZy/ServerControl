# ServerControl

A powerful server management system with auto-discovery, providing centralized control over multiple Linux servers.

## Overview

ServerControl consists of two components:

1. **Server** (`server.cpp`) - Lightweight HTTP server that runs on each managed server
2. **Control** (`control/`) - Terminal-based UI application for managing all servers

## Features

### Server Component
- HTTP API for remote command execution
- Real-time CPU and RAM statistics
- Task management and logging
- Auto-discovery via UDP broadcast
- Multiple concurrent task execution

### Control Component
- **Auto-Discovery**: Automatically finds servers on the network (no manual configuration needed)
- **Beautiful TUI**: Enhanced terminal UI with colors, borders, and status indicators
- **Server Dashboard**: Real-time view of all servers with CPU/RAM usage
- **Server Context Menu**: Right-click style menu for each server with:
  - 💻 **Terminal**: Interactive command prompt with history
  - 📋 **View Logs**: Browse task logs from all running/completed tasks
  - 🗑️ **Kill Tasks**: Select and terminate running tasks
  - 📊 **CPU/RAM Stats**: Detailed server statistics with visual bars
- **Multi-Server Operations**: Execute commands on multiple servers simultaneously
- **Task Tracking**: Monitor all running and completed tasks across all servers

## Building

### Prerequisites
- C++17 compiler (g++)
- ASIO (libasio-dev)
- nlohmann-json (nlohmann-json3-dev)
- ncurses (libncurses-dev)

On Ubuntu/Debian:
```bash
sudo apt-get install -y g++ libasio-dev nlohmann-json3-dev libncurses-dev
```

### Compile
```bash
./build.sh
```

Or manually:
```bash
# Build server
g++ -std=c++17 -pthread server.cpp -o server

# Build control
cd control
g++ -std=c++17 -pthread main.cpp UI.cpp TaskManager.cpp Config.cpp Server.cpp HttpClient.cpp -o control -lncurses
```

## Usage

### Running the Server
On each server you want to manage:
```bash
./server
```

The server will:
- Start HTTP API on port 8080
- Start UDP discovery responder on port 8081
- Report its hostname for identification

### Running the Control
On your laptop/control machine:
```bash
cd control
./control
```

The control application will:
- Automatically discover all servers on the local network
- Display a real-time dashboard
- Allow you to manage servers interactively

### Control Interface

#### Main Dashboard
- **↑/↓**: Navigate between servers
- **SPACE**: Select/deselect servers for multi-server operations
- **ENTER**: Open server menu
- **R**: Refresh server stats
- **ESC**: Quit

#### Server Menu
- **1**: Open Terminal
- **2**: View Logs
- **3**: Kill Tasks
- **4**: View CPU/RAM Stats
- **ESC**: Back to dashboard

#### Terminal Mode
- Type commands and press **ENTER** to execute on the selected server
- Command history is displayed
- **ESC**: Return to menu

#### Kill Tasks Mode
- **↑/↓**: Navigate between running tasks
- **ENTER**: Kill selected task
- **ESC**: Return to menu

## Server API Endpoints

- `POST /exec` - Execute a command
  - Body: `{"cmd": "command to run"}`
  - Returns: `{"task_id": "123"}`

- `GET /tasks` - List all tasks
  - Returns: Array of tasks with id, command, and running status

- `GET /logs?id=<task_id>` - Get task logs
  - Returns: `{"logs": "task output"}`

- `POST /kill?id=<task_id>` - Kill a task

- `GET /stats` - Get server statistics
  - Returns: `{"cpu": 45.2, "ram_used": 2048, "ram_total": 8192}`

- `GET /hostname` - Get server hostname
  - Returns: `{"hostname": "server01"}`

## Network Requirements

- Servers and control must be on the same local network for auto-discovery
- Port 8080 (TCP) for HTTP API
- Port 8081 (UDP) for discovery
- Broadcast packets must be allowed

## Fallback Configuration

If auto-discovery fails (e.g., in restricted networks), the control will use a default configuration. Edit `control/Config.cpp` to customize the fallback server list.

## Security Note

⚠️ This software is designed for trusted internal networks. It:
- Has no authentication
- Allows arbitrary command execution
- Should NOT be exposed to the internet
- Should only be used in controlled environments

## Architecture

### Server-Side
- Multi-threaded HTTP server using ASIO
- Each client connection handled in a separate thread
- Task execution in background threads
- Real-time stats gathering from `/proc` filesystem

### Control-Side
- Event-driven ncurses TUI
- HTTP client for server communication
- Task manager for tracking operations across servers
- Auto-discovery via UDP broadcast

## Future Enhancements

Potential improvements:
- Authentication and encryption
- File upload/download
- Log streaming in real-time
- Server groups and tags
- Configuration profiles
- Scripting support
- Web-based UI option

## License

MIT License - See LICENSE file for details
