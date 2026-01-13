# ServerControl Architecture - Two Separate Applications

## Clear Separation

### Server Application (`server/**`)
**Purpose:** Lightweight agent that runs on each managed server
**Role:** Data transmitter only
**Communication:** WebSocket server

**Responsibilities:**
1. Start WebSocket server on IP:2030
2. Transmit server data:
   - CPU/RAM stats (real-time)
   - Terminal/command output (live streaming)
   - System logs
   - Process list
   - File system data
   - Screen capture (for remote desktop)
3. Accept commands via WebSocket:
   - Execute terminal commands
   - Kill processes
   - File operations
   - Mouse/keyboard input (for remote control)
4. Auto-discovery responder (UDP)

**Does NOT have:**
- HTTP API (everything via WebSocket)
- User interface
- Complex logic
- Multiple endpoints

**Architecture:**
```
[Server Machine]
  ┌──────────────────┐
  │  server binary   │
  ├──────────────────┤
  │ • WebSocket :2030│
  │ • UDP Discovery  │
  │ • X11 Capture    │
  │ • Command Exec   │
  │ • Stats Monitor  │
  └──────────────────┘
         ↕ WebSocket
    (data streaming)
```

### Control Application (`control/**`)
**Purpose:** Control panel for managing all servers
**Role:** Client that discovers and controls servers
**Communication:** WebSocket client

**Responsibilities:**
1. Discover all servers on network (UDP broadcast)
2. Connect to each server's WebSocket
3. Display aggregated data in TUI (ncurses)
4. Send commands to servers:
   - Terminal commands
   - Process management
   - File operations
   - Remote desktop control
5. Render server screens (remote desktop)
6. Handle user input (keyboard/mouse for remote control)

**Features:**
- Server list with live stats
- Per-server terminal
- Remote desktop viewer (like AppOnFly)
- Process manager
- File manager
- System logs viewer
- Multi-server operations

**Architecture:**
```
[Your Laptop]
  ┌────────────────────────┐
  │   control binary       │
  ├────────────────────────┤
  │ • ncurses UI           │
  │ • WebSocket clients    │
  │ • Server discovery     │
  │ • Remote desktop view  │
  │ • Command interface    │
  └────────────────────────┘
         ↕ WebSocket
    (send commands,
     receive data)
         ↓
  ┌─────────────┬─────────────┬─────────────┐
  │  Server 1   │  Server 2   │  Server 3   │
  │ 10.x.x.1    │ 10.x.x.2    │ 10.x.x.3    │
  │ :2030       │ :2030       │ :2030       │
  └─────────────┴─────────────┴─────────────┘
```

## WebSocket Protocol

### Server → Control (Outbound Data)

**Stats Updates** (every 1 second):
```json
{
  "type": "stats",
  "cpu": 45.2,
  "ram_used": 2048,
  "ram_total": 8192,
  "timestamp": 1234567890
}
```

**Terminal Output** (real-time streaming):
```json
{
  "type": "terminal_output",
  "session_id": "term_1",
  "output": "Hello World\n",
  "timestamp": 1234567890
}
```

**Screen Frame** (for remote desktop):
```json
{
  "type": "screen_frame",
  "width": 1920,
  "height": 1080,
  "format": "jpeg",
  "data": "<base64_encoded_jpeg>",
  "timestamp": 1234567890
}
```

**Process List**:
```json
{
  "type": "process_list",
  "processes": [
    {"pid": "1234", "name": "nginx", "cpu": "5%", "mem": "2%"}
  ]
}
```

**File List**:
```json
{
  "type": "file_list",
  "path": "/home/user",
  "files": [
    {"name": "file.txt", "size": 1024, "is_dir": false}
  ]
}
```

**System Logs**:
```json
{
  "type": "system_logs",
  "logs": ["[2024-01-13] System started", "..."]
}
```

**CPU Alert**:
```json
{
  "type": "cpu_alert",
  "cpu": 94.5,
  "message": "CPU usage exceeded 90%!"
}
```

### Control → Server (Commands)

**Execute Terminal Command**:
```json
{
  "type": "exec_command",
  "session_id": "term_1",
  "command": "ls -la",
  "cwd": "/home/user"
}
```

**Kill Process**:
```json
{
  "type": "kill_process",
  "pid": "1234",
  "signal": "15"
}
```

**File Operation**:
```json
{
  "type": "file_operation",
  "action": "read|write|delete|rename",
  "path": "/home/user/file.txt",
  "data": "..."
}
```

**Remote Desktop Control**:
```json
{
  "type": "remote_mouse",
  "x": 100,
  "y": 200,
  "button": 1,
  "pressed": true
}
```

```json
{
  "type": "remote_keyboard",
  "keycode": 65,
  "pressed": true
}
```

**Start/Stop Screen Capture**:
```json
{
  "type": "remote_control",
  "action": "start|stop",
  "fps": 30,
  "quality": 75
}
```

**Request Data**:
```json
{
  "type": "request",
  "data_type": "processes|files|logs|stats"
}
```

## Directory Structure

### Server (`server/**`)
```
server/
├── main.cpp                    # Entry point
├── core/
│   ├── Utils.h/cpp            # Utilities
│   └── Config.h/cpp           # Configuration
├── network/
│   ├── WebSocketServer.h/cpp  # WebSocket server
│   └── Discovery.h/cpp        # UDP discovery
├── system/
│   ├── Stats.h/cpp            # CPU/RAM monitoring
│   ├── Commander.h/cpp        # Command execution
│   ├── ProcessManager.h/cpp   # Process management
│   ├── FileSystem.h/cpp       # File operations
│   └── RemoteDesktop.h/cpp    # Screen capture + input
└── protocol/
    └── Messages.h             # Message definitions
```

### Control (`control/**`)
```
control/
├── main.cpp                   # Entry point
├── core/
│   ├── TaskManager.h/cpp     # Task management
│   └── ServerConnection.h/cpp # Per-server connection
├── network/
│   ├── Discovery.h/cpp       # UDP discovery
│   └── WebSocketClient.h/cpp # WebSocket client
├── ui/
│   ├── UI.h/cpp              # Main UI
│   ├── ServerList.h/cpp      # Server list view
│   ├── Terminal.h/cpp        # Terminal view
│   ├── RemoteView.h/cpp      # Remote desktop view
│   ├── ProcessView.h/cpp     # Process manager view
│   └── FileView.h/cpp        # File manager view
└── protocol/
    └── Messages.h            # Same message definitions
```

## Implementation Plan

### Phase 1: Clean WebSocket Server
1. Remove HTTP API from server
2. Pure WebSocket communication
3. Message-based protocol
4. Test with simple WebSocket client

### Phase 2: Control WebSocket Client
1. Add WebSocket client to control
2. Connect to discovered servers
3. Display live stats in UI
4. Send/receive test messages

### Phase 3: Terminal Integration
1. Server: Execute commands, stream output
2. Control: Display terminal, send commands
3. Track current directory
4. Handle multiple terminal sessions

### Phase 4: Remote Desktop
1. Server: Capture screen, send frames
2. Control: Display frames, send input
3. Handle mouse/keyboard events
4. F11 fullscreen toggle

### Phase 5: Additional Features
1. Process management
2. File operations
3. System logs
4. Multi-server commands

## Key Differences from Current Implementation

**Current (Mixed):**
- Server has both HTTP and WebSocket
- Control doesn't use WebSocket yet
- Unclear separation

**New (Clean):**
- Server: WebSocket ONLY
- Control: WebSocket client
- Clear client-server model
- All data via WebSocket
- No HTTP on server

## Benefits

1. **Simpler Server:** No HTTP routing, just WebSocket messages
2. **Real-time Everything:** All data pushed via WebSocket
3. **Cleaner Code:** Clear protocol definition
4. **Easier Testing:** Simple message format
5. **Better Performance:** No polling, pure push
6. **Scalable:** Same protocol for N servers

## Next Steps

1. Define complete message protocol
2. Implement WebSocket-only server
3. Add WebSocket client to control
4. Migrate features one by one
5. Test with multiple servers
