# StreamX

A cross-platform streaming application for Linux with multi-backend screen capture support.

## Features

- **Multi-Plan Capture System** - Automatic fallback between different capture methods:
  - Plan A: D-Bus OBS-style screencast
  - Plan B: xdg-portal/PipeWire (most modern distros)
  - Plan C: FFmpeg libavdevice (universal fallback)
  - Plan D: GPU Screen Recorder (best for Wayland on Fedora/Nobara)

- **Platform Support**: Twitch, YouTube, custom RTMP
- **Qt6 GUI**: Modern interface with stream controls
- **Auto-Detection**: Automatically detects your Linux distro and configures the best capture plans

## Requirements

- Linux (tested on Nobara with KDE Wayland)
- Qt6
- FFmpeg (libavcodec, libavformat, libavdevice, libswscale)
- PipeWire + xdg-desktop-portal
- GPU Screen Recorder (optional, for Plan D on Fedora/Nobara)

## Building

```bash
# Clone and build
make build

# Or using CMake directly
mkdir build && cd build
cmake ..
make
```

### Build Options

```bash
# Override target distro
make build TARGET_OS=ubuntu

# Debug build
make debug

# Run tests
make test

# Clean
make clean
```

### Supported Distros

| Distro | Plan D (GPU Screen Recorder) | Notes |
|--------|------------------------------|-------|
| Nobara | Enabled | Best support |
| Fedora | Enabled | Best support |
| Ubuntu | Disabled | Plan A/B/C work |
| Debian | Disabled | Plan A/B/C work |
| Arch | Enabled | Available on AUR |

## Usage

### GUI

```bash
./build/streamx_gui
```

### CLI

```bash
# Add platform
./build/streamx add-platform twitch <stream_key>

# Start streaming
./build/streamx start

# Stop streaming
./build/streamx stop

# Show status
./build/streamx status
```

## Architecture

```
StreamX
├── src/core/          - Core streaming components (capture, encode, buffer)
├── src/gui/           - Qt6 GUI application
├── src/platform/      - RTMP handling
├── src/platforms/     - Platform implementations (Twitch, YouTube, etc)
├── src/plan_d/        - GPU Screen Recorder integration
└── src/portal/        - D-Bus screencast (Plan A)
```

## Key Components

- **Capture**: Multi-plan screen capture with Wayland support
- **Encoder**: FFmpeg-based (libx264) video encoding
- **RTMP Handler**: Direct RTMP streaming to platforms
- **Streaming Controller**: Orchestrates capture → encode → stream pipeline

## Troubleshooting

### "no target node available" on Wayland
- Plan D (GPU Screen Recorder) should automatically activate on Fedora/Nobara
- Check that GPU Screen Recorder is installed: `which gpu-screen-recorder`

### Stream shows test pattern
- Plan B failed to capture frames
- Ensure xdg-desktop-portal is running
- Check PipeWire is active: `pw-info`

## License

MIT License

## Contributing

Issues and pull requests welcome!