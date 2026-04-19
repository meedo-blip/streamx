# StreamX Build & Installation Guide

## Quick Start

### Linux (Ubuntu/Debian)

```bash
# 1. Install dependencies
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    nlohmann-json3-dev \
    git

# 2. Clone and build
git clone <repository>
cd streamx
mkdir -p build
cd build

# 3. Configure build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON

# 4. Build
make -j$(nproc)

# 5. Run tests
make test

# 6. Install (optional)
sudo make install
```

### macOS

```bash
# 1. Install Homebrew if not installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 2. Install dependencies
brew install cmake ffmpeg nlohmann-json

# 3. Build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.physicalcpu)
```

### Windows (Visual Studio 2019+)

```powershell
# 1. Install dependencies via vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\vcpkg\integrate install
.\vcpkg install ffmpeg:x64-windows nlohmann-json:x64-windows

# 2. Configure and build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg-path>/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 16 2019"
cmake --build . --config Release

# 3. Run executable
Release\streamx.exe
```

## Advanced Configuration

### CMake Options

```bash
# Build without tests
cmake .. -DBUILD_TESTS=OFF

# Enable NVIDIA NVENC encoding
cmake .. -DENABLE_NVENC=ON

# Set custom FFmpeg path
cmake .. -DFFmpeg_DIR=/custom/ffmpeg/path

# Debug build with symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Manual FFmpeg Installation

If package manager installation fails:

```bash
# Download and build FFmpeg
wget https://ffmpeg.org/releases/ffmpeg-snapshot.tar.bz2
tar xjf ffmpeg-snapshot.tar.bz2
cd ffmpeg
./configure --prefix=/usr/local \
    --enable-gpl \
    --enable-libx264 \
    --enable-libx265
make -j$(nproc)
sudo make install

# Update PKG_CONFIG_PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

## Compilation Issues

### "FFmpeg not found"
```bash
# Find FFmpeg installation
pkg-config --cflags --libs libavcodec

# If not found, install:
sudo apt-get install libffmpeg-ocaml-dev  # Ubuntu
brew install ffmpeg  # macOS
```

### "nlohmann/json not found"
```bash
# Install JSON library
sudo apt-get install nlohmann-json3-dev  # Ubuntu
brew install nlohmann-json  # macOS
# Windows: vcpkg install nlohmann-json:x64-windows
```

### Linking errors
```bash
# Verify library paths
ldconfig -p | grep ffmpeg
ldconfig -p | grep json

# Set LD_LIBRARY_PATH if needed
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

## GPU Encoding (NVIDIA NVENC)

### Prerequisites
- NVIDIA GPU with Turing or newer architecture
- NVIDIA CUDA Toolkit 11.0+
- NVIDIA Video Codec SDK

### Installation

```bash
# 1. Install CUDA Toolkit
# Download from: https://developer.nvidia.com/cuda-downloads

# 2. Install Video Codec SDK
# Download from: https://developer.nvidia.com/video-codec-sdk

# 3. Build with NVENC support
mkdir -p build && cd build
cmake .. -DENABLE_NVENC=ON \
         -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda
make -j$(nproc)
```

### Verification

```bash
# Check if NVENC is available
./streamx status

# Output should show: "NVENC Support: YES"
```

## Docker Build

```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    nlohmann-json3-dev

COPY . /app
WORKDIR /app

RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

ENTRYPOINT ["./build/streamx"]
```

Build Docker image:
```bash
docker build -t streamx .
docker run --rm streamx help
```

## Troubleshooting Build Issues

### Out of Memory
```bash
# Reduce parallel jobs
make -j2  # Instead of -j$(nproc)
```

### Missing system libraries
```bash
# Install development tools
sudo apt-get install build-essential  # Ubuntu
brew install xcode-select  # macOS
```

### CMake not found
```bash
# Install CMake
sudo apt-get install cmake  # Ubuntu
brew install cmake  # macOS
choco install cmake  # Windows (with Chocolatey)
```

## Post-Installation

### Verify Installation
```bash
# Test executable
./streamx help

# Run unit tests
./streamx_tests

# Check version
./streamx --version
```

### System Integration (Linux)

```bash
# 1. Copy executable to bin
sudo cp build/streamx /usr/local/bin/

# 2. Make it executable
sudo chmod +x /usr/local/bin/streamx

# 3. Now runnable from anywhere
streamx help
```

## Development Setup

### IDE Configuration

**VS Code (.vscode/launch.json)**:
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "StreamX",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/streamx",
            "args": ["help"],
            "cwd": "${workspaceFolder}"
        }
    ]
}
```

**CLion**:
- Open project -> Select CMakeLists.txt
- CLion will auto-generate build configuration

### Running Tests

```bash
cd build
ctest --output-on-failure
ctest -V  # Verbose output
ctest -R test_frame  # Run specific test
```

## Performance Optimization

### Compiler Flags
```bash
cmake .. \
    -DCMAKE_CXX_FLAGS="-O3 -march=native" \
    -DCMAKE_BUILD_TYPE=Release
```

### Link-Time Optimization
```bash
cmake .. \
    -DCMAKE_CXX_FLAGS="-O3 -flto" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto"
```

## Next Steps

1. Review [README.md](README.md) for features and usage
2. Check [API.md](API.md) for C++ API documentation
3. See [EXAMPLES.md](EXAMPLES.md) for code examples
4. Visit project repository for support

---

For issues or questions, create an issue in the project repository.
