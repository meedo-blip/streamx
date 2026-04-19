# StreamX GUI Documentation

## Overview

StreamX GUI is a professional, cross-platform desktop application for managing multi-platform live streaming. Built with Qt 6, it provides an intuitive interface for streaming configuration, real-time monitoring, and platform management.

## Features

- **Intuitive Dashboard**: Tab-based interface for different streaming aspects
- **Real-time Monitoring**: Live metrics display including bitrate, latency, and frame statistics
- **Multi-Platform Management**: Easy add/remove/edit of streaming platforms
- **Advanced Settings**: Fine-grained control over video, audio, and encoding parameters
- **Cross-Platform**: Native look and feel on Windows, macOS, and Linux
- **Modern UI**: Dark theme with responsive design

## Build Requirements

- Qt 6.0 or higher
- C++17 compatible compiler
- FFmpeg libraries
- CMake 3.16+

### Install Qt on Different Platforms

**Ubuntu/Debian:**
```bash
sudo apt-get install qt6-base-dev qt6-base-dev-tools
```

**Fedora:**
```bash
sudo dnf install qt6-qtbase-dev
```

**macOS (with Homebrew):**
```bash
brew install qt
```

**Windows:**
Download and install from https://www.qt.io/download

## Building the GUI

```bash
mkdir build
cd build
cmake .. -DBUILD_GUI=ON
cmake --build .
```

### Build Options

- `-DBUILD_GUI=ON` (default): Build the GUI application
- `-DBUILD_GUI=OFF`: Build only CLI and core library
- `-DENABLE_NVENC=ON`: Enable GPU encoding support

## Running the Application

**Linux/macOS:**
```bash
./streamx_gui
```

**Windows:**
```bash
streamx_gui.exe
```

## User Interface

### Main Window

The main window contains:
- **Menu Bar**: File and Help menus with exit and about actions
- **Tab Widget**: Four main tabs for different streaming tasks
- **Status Bar**: Current streaming status, platform count, and bitrate display

### Tab 1: Stream Control

**Purpose**: Start/stop streaming and control capture sources

**Components:**
- **Capture Type Selector**: Choose between Desktop, Window, or Audio capture
- **Start Streaming Button**: Initialize capture and broadcasting
- **Stop Streaming Button**: Gracefully stop all streams
- **CPU/GPU Usage**: Real-time resource utilization display
- **Platform Management**: Quick add/remove platform controls

**Workflow:**
1. Select capture type from dropdown
2. Click "Start Streaming" to begin
3. Monitor CPU/GPU usage
4. Click "Stop Streaming" when done

### Tab 2: Monitoring

**Purpose**: Real-time stream health and performance metrics

**Features:**
- **Overall Stream Health**: Connection status indicator
- **Combined Bitrate**: Visual representation of total bitrate
- **Average Latency**: Network latency indicator
- **Per-Platform Metrics Table**:
  - Platform name and connection status
  - Individual bitrate and latency
  - Frame drop count
  - Reconnection attempts
  - Last update timestamp

**Metrics Updates:**
- Refreshes every 500ms for real-time feedback
- Color indicators: 🟢 Connected, 🔴 Disconnected

### Tab 3: Platforms

**Purpose**: Manage streaming platforms and credentials

**Features:**
- **Platform Table**: Lists all configured platforms with types and status
- **Add Platform**: Add new streaming destinations
- **Edit Platform**: Update stream keys and settings
- **Remove Platform**: Delete platforms from configuration

**Stream Key Security:**
- Stream keys are masked as "*" in display
- Only visible during input
- Encrypted in configuration files

**Supported Platforms:**
- Twitch
- YouTube
- StreamLabs
- Custom RTMP servers

### Tab 4: Settings

**Purpose**: Configure video, audio, and encoding parameters

**Video Settings:**
- **Resolution**: 640x480 to 4096x2160 (default: 1920x1080)
- **Frame Rate**: 24 to 120 fps (default: 60)
- **Bitrate**: 1000 to 10000 kbps (default: 5000)
- **Codec**: H.264 or H.265
- **Encoding Preset**: ultrafast to slower (quality vs speed tradeoff)
- **B-Frames**: Enable/disable for better compression
- **GOP Size**: Keyframe interval (1-300 frames)

**Audio Settings:**
- **Sample Rate**: 16kHz to 192kHz (default: 48kHz)
- **Channels**: 1-8 channels (default: stereo/2)
- **Bitrate**: 64-320 kbps (default: 128)

**Encoding Settings:**
- **GPU Encoding**: Enable NVIDIA NVENC for GPU acceleration
- **Adaptive Bitrate**: Automatically adjust bitrate based on network conditions

**Configuration Buttons:**
- **Save Settings**: Save current configuration to file
- **Load Settings**: Restore previously saved configuration
- **Reset to Defaults**: Restore factory default settings

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Q` | Exit application |
| `F5` | Refresh metrics |

## Theme and Customization

The application uses a modern dark theme optimized for streaming environments:

**Color Scheme:**
- Background: #2b2b2b
- Accent: #0d47a1 (Blue)
- Success: #4caf50 (Green)
- Error: #f44336 (Red)
- Text: #ffffff (White)

**Customization:**
Theme can be modified in `main_window.cpp` by editing the `ApplyStylesheet()` function.

## Configuration Files

**Default Locations:**
- Linux: `~/.config/streamx/config.json`
- macOS: `~/Library/Application\ Support/streamx/config.json`
- Windows: `%APPDATA%\streamx\config.json`

## Troubleshooting

### GUI Won't Start
```bash
# Check dependencies
ldd ./streamx_gui  # Linux
otool -L ./streamx_gui  # macOS

# Enable debug logging
export QT_DEBUG_PLUGINS=1
./streamx_gui
```

### Display Issues
```bash
# Force platform-independent rendering
./streamx_gui -platform offscreen
```

### Can't Add Platforms
- Verify stream key is correct
- Check internet connection
- Check platform API credentials

### High CPU Usage
- Lower encoding preset to "faster" or "ultrafast"
- Reduce frame rate to 30fps
- Lower resolution to 720p

### GPU Encoding Not Available
- Verify NVIDIA drivers installed: `nvidia-smi`
- NVIDIA GPU required for NVENC
- Check build flag: `cmake .. -DENABLE_NVENC=ON`

## Performance Tips

1. **For Live Streaming:**
   - Use H.264 for compatibility
   - Preset: "medium" to "slow"
   - Bitrate: 3000-5000 kbps for 720p60

2. **For High-Quality Streaming:**
   - Use H.265 for better compression
   - Preset: "slow" or "slower"
   - Bitrate: 5000-8000 kbps for 1080p60

3. **For CPU-Constrained Systems:**
   - Enable GPU encoding (NVENC)
   - Use "ultrafast" preset
   - Lower frame rate to 30fps
   - Reduce resolution to 720p

## Integration with CLI

Both CLI and GUI share the same core streaming engine:

**CLI**: `streamx` command-line tool
**GUI**: `streamx_gui` graphical application

Both can coexist and share configuration. Use GUI for interactive setup and monitoring, or CLI for automated/scripted streaming.

## Development and Extending

### Architecture

```
StreamX GUI
├── Main Window (UI Orchestrator)
├── Stream Control Widget (Capture & Platform Management)
├── Monitoring Widget (Real-time Metrics)
├── Platforms Widget (Platform Configuration)
├── Settings Widget (Encoding Configuration)
└── Core Library (Shared Streaming Engine)
```

### Adding New Widgets

1. Create header in `include/streamx/gui/`
2. Implement in `src/gui/`
3. Add source files to `CMakeLists.txt`
4. Include in `main_window.cpp`
5. Add tab to tab widget

### Creating Custom Themes

Edit stylesheet in `ApplyStylesheet()` method:

```cpp
QString stylesheet = R"(
    /* Dark theme definitions */
    QMainWindow { background-color: #2b2b2b; }
    /* Add more rules */
)";
```

## API Integration

The GUI uses the StreamingController interface defined in `include/streamx/streaming_controller.h`.

**Key Methods:**
- `Initialize()` - Set video/audio configuration
- `StartStreaming()` - Begin streaming
- `StopStreaming()` - Stop all streams
- `AddPlatform()` - Add destination platform
- `GetOverallHealth()` - Get combined metrics
- `GetPlatformHealth()` - Get per-platform metrics

## License

StreamX GUI is part of the StreamX project and follows the same license terms.

## Support

For bugs, feature requests, or improvements, refer to the main StreamX documentation or project repository.
