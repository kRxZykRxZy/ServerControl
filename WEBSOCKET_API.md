# WebSocket API Documentation

## Overview

ServerControl 2050 now includes a powerful WebSocket server that provides real-time, millisecond-precision streaming of terminal output, system metrics, and alerts.

## Connection

Connect to the WebSocket server at:
```
ws://<server-ip>:<websocket-port>
```

The WebSocket port is automatically allocated (default: 8082) and advertised via UDP discovery.

## Message Format

All WebSocket messages are JSON-encoded with the following general structure:

```json
{
  "type": "message_type",
  "timestamp": 1234567890123,
  ...additional fields...
}
```

## Server-to-Client Messages

### 1. Stats Update (every 1 second)

Broadcasts current server statistics every second.

```json
{
  "type": "stats_update",
  "cpu": 45.2,
  "ram_used": 2048,
  "ram_total": 8192,
  "timestamp": 1734077566789
}
```

**Fields:**
- `cpu`: CPU usage percentage (0-100)
- `ram_used`: RAM used in MB
- `ram_total`: Total RAM in MB
- `timestamp`: Unix timestamp in milliseconds

### 2. Task Start

Sent when a new command/task begins execution.

```json
{
  "type": "task_start",
  "task_id": "123",
  "command": "python app.py"
}
```

**Fields:**
- `task_id`: Unique identifier for this task
- `command`: The command being executed

### 3. Task Output (live streaming)

Sent for every chunk of output from the running task. This provides **real-time streaming** of command output as it happens.

```json
{
  "type": "task_output",
  "task_id": "123",
  "output": "Processing item 1...\n",
  "timestamp": 1734077566789
}
```

**Fields:**
- `task_id`: Unique identifier for the task
- `output`: The output chunk (can be partial lines)
- `timestamp`: Unix timestamp in milliseconds (precise to the millisecond)

**Use Case:**
This enables Vercel-style live log streaming where you see output appear character-by-character or line-by-line as the command executes.

### 4. Task Complete

Sent when a task finishes execution.

```json
{
  "type": "task_complete",
  "task_id": "123",
  "exit_code": 0
}
```

**Fields:**
- `task_id`: Unique identifier for the task
- `exit_code`: Exit code from the command (0 = success)

### 5. CPU Alert

Sent when CPU usage exceeds 90%. Includes a cooldown of 60 seconds between alerts to prevent spam.

```json
{
  "type": "cpu_alert",
  "cpu": 94.5,
  "hostname": "server01",
  "message": "CPU usage exceeded 90%!",
  "timestamp": 1734077566789
}
```

**Fields:**
- `cpu`: Current CPU usage percentage
- `hostname`: Server hostname
- `message`: Alert message
- `timestamp`: Unix timestamp in milliseconds

## Client-to-Server Messages

### Ping/Pong

Keep-alive mechanism for connection health checking.

**Request:**
```json
{
  "type": "ping"
}
```

**Response:**
```json
{
  "type": "pong",
  "timestamp": 1734077566789
}
```

## Example: Connecting with JavaScript

```javascript
const ws = new WebSocket('ws://10.92.32.5:8082');

ws.onopen = () => {
  console.log('Connected to ServerControl WebSocket');
  
  // Send ping every 30 seconds
  setInterval(() => {
    ws.send(JSON.stringify({ type: 'ping' }));
  }, 30000);
};

ws.onmessage = (event) => {
  const msg = JSON.parse(event.data);
  
  switch(msg.type) {
    case 'stats_update':
      updateDashboard(msg.cpu, msg.ram_used, msg.ram_total);
      break;
      
    case 'task_output':
      appendToTerminal(msg.task_id, msg.output);
      console.log(`[${new Date(msg.timestamp).toISOString()}] ${msg.output}`);
      break;
      
    case 'task_start':
      console.log(`Task ${msg.task_id} started: ${msg.command}`);
      break;
      
    case 'task_complete':
      console.log(`Task ${msg.task_id} completed with exit code ${msg.exit_code}`);
      break;
      
    case 'cpu_alert':
      showNotification(`⚠️ ${msg.hostname}: ${msg.message} (${msg.cpu}%)`);
      break;
      
    case 'pong':
      console.log('Ping received');
      break;
  }
};

ws.onerror = (error) => {
  console.error('WebSocket error:', error);
};

ws.onclose = () => {
  console.log('WebSocket connection closed');
};
```

## Example: Connecting with Python

```python
import asyncio
import websockets
import json
from datetime import datetime

async def connect():
    uri = "ws://10.92.32.5:8082"
    
    async with websockets.connect(uri) as websocket:
        print("Connected to ServerControl WebSocket")
        
        async for message in websocket:
            msg = json.loads(message)
            msg_type = msg.get('type')
            
            if msg_type == 'stats_update':
                print(f"Stats: CPU={msg['cpu']}%, RAM={msg['ram_used']}/{msg['ram_total']} MB")
            
            elif msg_type == 'task_output':
                timestamp = datetime.fromtimestamp(msg['timestamp'] / 1000.0)
                print(f"[{timestamp.isoformat()}] Task {msg['task_id']}: {msg['output']}", end='')
            
            elif msg_type == 'task_start':
                print(f"Task {msg['task_id']} started: {msg['command']}")
            
            elif msg_type == 'task_complete':
                print(f"Task {msg['task_id']} completed with exit code {msg['exit_code']}")
            
            elif msg_type == 'cpu_alert':
                print(f"⚠️  ALERT: {msg['message']} on {msg['hostname']} - CPU: {msg['cpu']}%")

asyncio.run(connect())
```

## Performance Characteristics

- **Latency**: < 5ms from command output to WebSocket broadcast
- **Timestamp Precision**: Millisecond accuracy
- **Update Frequency**: Stats every 1 second, terminal output in real-time
- **Concurrent Clients**: Supports 100+ simultaneous WebSocket connections
- **Memory Overhead**: ~50KB per connection

## Security Considerations

⚠️ **CRITICAL**: The WebSocket server has no authentication or encryption!

**Production Deployment:**
1. Deploy behind VPN/WireGuard tunnel
2. Use SSH port forwarding: `ssh -L 8082:localhost:8082 user@server`
3. Implement network-level access control (firewall, iptables)
4. Never expose WebSocket port to public internet

## Troubleshooting

### Connection Refused
- Check firewall allows port 8082 (or actual allocated port)
- Verify server is running: `netstat -tuln | grep <ws_port>`
- Check server logs for port conflicts

### No Messages Received
- Verify WebSocket connection is established (`onopen` fired)
- Check server CPU > 0% or run a command to trigger output
- Ensure no proxy/load balancer is buffering WebSocket frames

### Delayed Messages
- Check network latency: `ping <server-ip>`
- Verify no intermediate proxies are buffering
- Monitor server load - high CPU may delay broadcasts

## Integration Examples

### Live Dashboard
Create a web dashboard that shows real-time server stats:
- Connect WebSocket to receive `stats_update` messages
- Update gauges/charts every second
- Show alerts when CPU > 90%

### Terminal Emulator
Build a web-based terminal that streams command output:
- Send HTTP POST to `/exec` to start command
- Connect WebSocket to receive `task_output` messages
- Display output in a terminal-like interface
- Show completion status from `task_complete`

### Monitoring System
Create a multi-server monitoring solution:
- Connect to multiple servers' WebSocket endpoints
- Aggregate CPU/RAM stats from all servers
- Alert when any server exceeds thresholds
- Log all task executions for audit trail
