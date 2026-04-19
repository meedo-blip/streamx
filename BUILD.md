# StreamX Build Guide

## Prerequisites

### System Requirements
- **OS**: Linux, macOS, or Windows
- **Compiler**: GCC 7+, Clang 5+, or MSVC 2017+
- **CMake**: 3.16 or higher
- **C++ Standard**: C++17

### Dependencies

#### Required
- **FFmpeg**: libavutil, libavcodec, libavformat, libswscale
- **OpenSSL**: For RTMP connections
- **pthreads**: For threading (included on most systems)
- **Boost** (optional): For advanced utilities

#### Optional
- **Qt 6**: For GUI application
- **NVIDIA CUDA**: For NVENC GPU encoding support

### Installation by Platform

#### Ubuntu/Debian
```bash
# Core dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake \
    libavutil-dev libavcodec-dev libavformat-dev libswscale-dev \
    libssl-dev pkg-config

# Optional: GUI support
sudo apt-get install -y qt6-base-dev qt6-tools-dev

# Optional: NVIDIA NVENC support
sudo apt-get install -y nvidia-cuda-toolkit
```

#### Fedora/RHEL
```bash
# Core dependencies
sudo dnf install -y \
    gcc gcc-c++ cmake \
    ffmpeg-devel openssl-devel

# Optional: GUI support
sudo dnf install -y qt6-qtbase-devel

# Optional: CUDA
sudo dnf install -y cuda-toolkit
```

#### macOS (with Homebrew)
```bash
# Core dependencies
brew install cmake ffmpeg openssl pkg-config

# Optional: GUI support
brew install qt

# Optional: CUDA (if you have NVIDIA GPU)
# Download from NVIDIA website
```

#### Windows
- **Visual Studio**: 2017 or newer with C++ development tools
- **CMake**: Download from cmake.org
- **FFmpeg**: Pre-built binaries recommended
- **Qt 6**: Download from qt.io
- **CUDA Toolkit**: Download from nvidia.com (optional)

## Building

### Basic Build (CLI Only)

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake .. -DBUILD_TESTS=ON -DBUILD_GUI=OFF

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --config Release
```

### Build with GUI Support

```bash
mkdir build
cd build

# Configure with GUI
cmake .. \
    -DBUILD_GUI=ON \
    -DBUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run GUI
./streamx_gui
```

### Build with GPU Encoding (NVENC)

```bash
mkdir build
cd build

# Configure with NVENC support
cmake .. \
    -DENABLE_NVENC=ON \
    -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda \
    -DBUILD_GUI=ON

cmake --build . --config Release
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | ON | Build unit tests |
| `BUILD_GUI` | ON | Build Qt GUI application |
| `ENABLE_NVENC` | OFF | Enable NVIDIA NVENC encoding |
| `CMAKE_BUILD_TYPE` | Release | Debug or Release build |

### CMake Configuration Examples

#### Debug Build with Tests
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
```

#### Minimal Build (CLI only, no tests)
```bash
cmake .. -DBUILD_GUI=OFF -DBUILD_TESTS=OFF
```

#### Full Features Build
```bash
cmake .. \
    -DBUILD_GUI=ON \
    -DBUILD_TESTS=ON \
    -DENABLE_NVENC=ON \
    -DCMAKE_BUILD_TYPE=Release
```

## Running

### CLI Application
```bash
./streamx --help
./streamx start
./streamx status
```

### GUI Application
```bash
./streamx_gui
```

### Run Tests
```bash
cd build
ctest --output-on-failure
```

## Troubleshooting Build Issues

### FFmpeg Not Found
```bash
# Specify FFmpeg path explicitly
cmake .. -DFFMPEG_INCLUDE_DIR=/path/to/ffmpeg/include \
         -DFFMPEG_LIBRARY_DIR=/path/to/ffmpeg/lib
```

### Qt Not Found
```bash
# On macOS with Homebrew
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)

# On Linux
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/cmake/Qt6
```

### CUDA Not Found
```bash
# Specify CUDA path
cmake .. -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda
```

### Missing OpenSSL
```bash
# Find OpenSSL manually
cmake .. -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl  # macOS
cmake .. -DOPENSSL_ROOT_DIR=/usr/include/openssl   # Linux
```

### C++ Standard Error
```bash
# Ensure C++17 is available
cmake .. -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON
```

## Development Build

For active development with automatic rebuilds:

```bash
cd build
cmake --build . --config Debug -- -j$(nproc)
cmake --build . --target test
```

### Using ccache for Faster Rebuilds
```bash
cmake .. -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

## Cross-Compilation

### Linux to Windows (MinGW)
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw.cmake
```

### Building for ARM (Raspberry Pi)
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm.cmake
```

## Performance Build

For optimal performance in production:

```bash
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native" \
    -DENABLE_NVENC=ON
```

## Debugging

### Build with Debug Symbols
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-g -O0"
cmake --build .
```

### Enable Verbose Output
```bash
cmake --build . --verbose
```

### Run with ASAN (Address Sanitizer)
```bash
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address -g"
```

## Installation

### System-wide Installation
```bash
cd build
cmake --install . --prefix /usr/local
```

### Package Creation

#### DEB Package (Debian/Ubuntu)
```bash
cpack -G DEB
```

#### RPM Package (Fedora/RHEL)
```bash
cpack -G RPM
```

#### DMG Package (macOS)
```bash
cpack -G DragNDrop
```

## Verifying Installation

```bash
# Verify executables
./streamx --version
./streamx_gui --version

# Verify libraries
ldd ./streamx              # Linux
otool -L ./streamx         # macOS
```

## Clean Build

```bash
# Remove build artifacts
rm -rf build

# Start from scratch
mkdir build && cd build
cmake .. -DBUILD_GUI=ON
cmake --build .
```

## Advanced Configuration

### Custom FFmpeg Build
If system FFmpeg is insufficient:

```bash
# Build FFmpeg from source
./scripts/build-ffmpeg.sh

# Use custom FFmpeg in StreamX build
cmake .. -DFFMPEG_ROOT=/path/to/custom/ffmpeg
```

### Linking Against Static FFmpeg
```bash
cmake .. -DFFMPEG_USE_STATIC_LIBS=ON
```

## Support

For build issues:
1. Check CMake output for missing dependencies
2. Verify installed package versions
3. Run `cmake --system-information` for diagnostics
4. Consult troubleshooting section above

See also:
- [README.md](README.md) - Project overview
- [GUI.md](GUI.md) - GUI application guide
- [API.md](API.md) - API documentation
