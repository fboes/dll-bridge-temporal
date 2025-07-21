# Aerofly FS4 Complete Bridge DLL

A comprehensive multi-interface bridge for Aerofly FS4 that exposes all 339 flight simulator variables through multiple communication channels.

## Features

- **Complete Variable Coverage**: All 339 variables from the official Aerofly FS4 SDK
- **Multiple Interfaces**: 
  - Shared Memory (primary, ultra-fast)
  - TCP Server (network access with JSON format)
- **Bidirectional Communication**: Read flight data AND send commands back to the simulator
- **Thread-Safe Operations**: Robust multi-threaded architecture
- **Auto-Reconnection**: Automatic client reconnection handling
- **Performance Optimized**: Minimal latency with efficient data structures
- **Real-Time Data**: Microsecond timestamps and update counters

## Quick Start

### Installation

1. Download the latest `AeroflyBridge.dll` from the releases
2. Copy to: `%USERPROFILE%\Documents\Aerofly FS 4\external_dll\`
3. Start Aerofly FS4

### Shared Memory Access (Recommended)

```python
import mmap
import struct
import ctypes

# Open shared memory
with mmap.mmap(-1, 3384, "AeroflyBridgeData") as mm:
    # Read timestamp (first 8 bytes)
    timestamp = struct.unpack('Q', mm[0:8])[0]
    
    # Read latitude/longitude (bytes 16-32)
    lat, lon = struct.unpack('dd', mm[16:32])
    
    print(f"Aircraft at: {lat:.6f}, {lon:.6f}")
```

### TCP Network Access

```python
import socket
import json

# Connect to data stream
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 12345))

# Read JSON data
data = json.loads(sock.recv(4096).decode())
print(f"Altitude: {data['aircraft']['altitude']} meters")
```

### Send Commands

```python
import socket
import json

# Send command via TCP
cmd_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
cmd_sock.connect(('localhost', 12346))

command = {
    "variable": "Controls.Throttle",
    "value": 0.75
}

cmd_sock.send(json.dumps(command).encode())
cmd_sock.close()
```

## Data Structure

The shared memory contains a structured layout with all flight data:

### Memory Layout (3384 bytes total)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0-15 | 16 bytes | Header | Timestamp, validation, counter |
| 16-79 | 64 bytes | Aircraft Basic | Position, attitude, speeds |
| 80-175 | 96 bytes | Aircraft Physics | Forces, accelerations, vectors |
| 176-239 | 64 bytes | Aircraft State | Gear, flaps, ground status |
| 240-271 | 32 bytes | Engine Data | Throttles, RPM, running status |
| 272-295 | 24 bytes | Controls Input | Pilot inputs |
| 296-375 | 80 bytes | Navigation/Comm | Frequencies, courses |
| 376-439 | 64 bytes | Autopilot | AP modes and settings |
| 440-479 | 40 bytes | Performance | V-speeds |
| 480-495 | 16 bytes | Warnings | Warning flags |
| 496-3207 | 2712 bytes | All Variables Array | Direct access to all 339 variables |

### Variable Index Enum

All 339 variables are accessible by index:

```cpp
enum VariableIndex {
    AIRCRAFT_LATITUDE = 10,           // Aircraft.Latitude
    AIRCRAFT_LONGITUDE = 11,          // Aircraft.Longitude  
    AIRCRAFT_ALTITUDE = 1,            // Aircraft.Altitude
    CONTROLS_THROTTLE = 192,          // Controls.Throttle
    AUTOPILOT_ENGAGED = 172,          // Autopilot.Engaged
    // ... all 339 variables
};
```

## Available Interfaces

### 1. Shared Memory Interface (Primary)

- **Name**: `"AeroflyBridgeData"`
- **Size**: 3384 bytes
- **Access**: Direct memory mapping
- **Performance**: Ultra-fast (microseconds)
- **Platform**: Windows only

### 2. TCP Data Server

- **Port**: 12345
- **Protocol**: TCP
- **Format**: JSON
- **Updates**: Real-time streaming
- **Clients**: Multiple concurrent connections

### 3. TCP Command Server  

- **Port**: 12346
- **Protocol**: TCP
- **Format**: JSON commands
- **Direction**: Client → Simulator
- **Supported**: 100+ writable variables

## Command Examples

### Flight Controls

```json
{"variable": "Controls.Pitch.Input", "value": -0.5}
{"variable": "Controls.Roll.Input", "value": 0.3}
{"variable": "Controls.Throttle", "value": 0.8}
{"variable": "Controls.Flaps", "value": 0.5}
```

### Navigation

```json
{"variable": "Communication.COM1Frequency", "value": 118500000}
{"variable": "Navigation.NAV1Frequency", "value": 110300000}
{"variable": "Navigation.SelectedCourse1", "value": 1.57}
```

### Autopilot

```json
{"variable": "Autopilot.SelectedAirspeed", "value": 77.17}
{"variable": "Autopilot.SelectedHeading", "value": 0.785}  
{"variable": "Autopilot.SelectedAltitude", "value": 3048}
```

### Engine Controls

```json
{"variable": "Aircraft.Starter1", "value": 1}
{"variable": "Aircraft.EngineMaster1", "value": 1}
{"variable": "Controls.Mixture1", "value": 1.0}
```

## Variable Categories

### Aircraft Data (0-94)
Position, attitude, speeds, physics, system states

### Performance (95-104)
V-speeds: VS0, VS1, VFE, VNO, VNE, VAPP

### Navigation (108-141)  
NAV1/2, ILS1/2, DME1/2, ADF1/2 frequencies and data

### Communication (142-152)
COM1/2/3 frequencies, transponder

### Autopilot (153-180)
All autopilot modes, settings, and commands

### Controls (192-260)
All flight controls, engine controls, system switches

### Warnings (263-272)
Master warning/caution, system warnings

### View Controls (273-302)
Camera position, zoom, pan controls

### Simulation (303-320)
Pause, time, visibility, positioning

## Error Handling

The DLL includes comprehensive error handling:

- **Thread-Safe**: All operations are mutex-protected
- **Exception Safety**: No crashes propagated to Aerofly
- **Graceful Degradation**: TCP failures don't affect shared memory
- **Validation**: Data validation and bounds checking
- **Logging**: Debug output for troubleshooting

## Development

### Building from Source

```bash
# Requirements: Visual Studio with C++ support
cl /LD /EHsc /O2 aerofly_bridge_dll_complete.cpp /Fe:AeroflyBridge.dll /link ws2_32.lib
```

### Dependencies

- Windows SDK
- Winsock2 (ws2_32.lib)
- tm_external_message.h (from Aerofly SDK)

### Testing

Use the included Python examples or any TCP client to verify connectivity:

```bash
# Test TCP connection
telnet localhost 12345

# Test shared memory
python test_shared_memory.py
```

## Performance Characteristics

| Interface | Latency | Throughput | Clients |
|-----------|---------|------------|---------|
| Shared Memory | <1μs | >1GB/s | 1 |
| TCP Local | <1ms | >100MB/s | Multiple |
| TCP Network | ~ping | ~bandwidth | Multiple |

## Troubleshooting

### Common Issues

1. **DLL not loading**: Check file location and permissions
2. **Shared memory access denied**: Run as administrator
3. **TCP port conflicts**: Check ports 12345/12346 availability
4. **No data updates**: Verify Aerofly FS4 is running

### Debug Output

Enable debug output in Visual Studio Output window:
- Variable updates
- Client connections/disconnections  
- Command processing
- Error messages

## License

This project is open source. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create your feature branch
3. Submit a pull request

## Support

- **Issues**: Use GitHub Issues for bug reports
- **Documentation**: Check the wiki for detailed examples
- **Community**: Join discussions in the GitHub Discussions tab

---

**Note**: This DLL requires Aerofly FS4 and is compatible with Windows x64 systems. The bridge provides complete access to all simulator variables with minimal performance impact.