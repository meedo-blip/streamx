# StreamX Makefile - Auto-detects OS and builds with appropriate plans

.PHONY: all clean help build test install uninstall debug release

# Detect OS - use shell to read /etc/os-release properly
ifeq ($(shell uname -s),Linux)
	detected_os := $(shell ( \
		if [ -f /etc/os-release ]; then \
			. /etc/os-release; \
			echo "$$ID"; \
		else \
			echo "unknown"; \
		fi))
	# Check ID_LIKE for fedora-based distros if ID is empty
	ifeq ($(detected_os),)
		detected_os := $(shell ( \
			if [ -f /etc/os-release ]; then \
				. /etc/os-release; \
				echo "$$ID_LIKE"; \
			fi))
		# If ID_LIKE contains fedora, default to nobara for Wayland support
		ifeq ($(findstring fedora,$(detected_os)),fedora)
			detected_os := nobara
		endif
	endif
else
	detected_os := unknown
endif

# If still empty or unknown, try a different approach
ifeq ($(detected_os),)
detected_os := $(shell (grep -E "^ID=" /etc/os-release 2>/dev/null | cut -d= -f2 | tr -d '"'))
endif

# Default target
all: build

help:
	@echo "StreamX Build System"
	@echo "===================="
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  build     - Build the project (default)"
	@echo "  clean     - Clean build artifacts"
	@echo "  test      - Build and run tests"
	@echo "  debug     - Build with debug flags"
	@echo "  release   - Build with optimizations (default)"
	@echo "  install   - Install to system"
	@echo "  uninstall - Remove installation"
	@echo ""
	@echo "Options (via environment):"
	@echo "  PLANS=\"A B C D\"   - Enable specific plans (default: all)"
	@echo "  TARGET_OS=ubuntu - Override auto-detected OS"
	@echo ""
	@echo "Detected OS: $(detected_os)"
	@echo ""
	@echo "Plan options per OS:"
	@echo "  nobara/fedora: All plans enabled (Plan D = GPU Screen Recorder)"
	@echo "  ubuntu/debian: Plans A, B, C enabled (Plan D off - needs manual install)"
	@echo "  arch: All plans enabled"

build:
	@echo "=== Building StreamX ==="
	@echo "Detected OS: $(detected_os)"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DSTREAMX_TARGET_DISTRO=$(detected_os)
	@cd build && cmake --build . -j$$(nproc)

debug:
	@echo "=== Building StreamX (Debug) ==="
	@echo "Detected OS: $(detected_os)"
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug -DSTREAMX_TARGET_DISTRO=$(detected_os)
	@cd build && cmake --build . -j$$(nproc)

test: build
	@echo "=== Running tests ==="
	@cd build && ctest --output-on-failure

clean:
	@echo "=== Cleaning build ==="
	@rm -rf build
	@mkdir -p build

install:
	@echo "=== Installing StreamX ==="
	@cd build && cmake --install . --prefix /usr

uninstall:
	@echo "=== Uninstalling StreamX ==="
	@cd build && cmake --uninstall

distro:
	@echo "Detected: $(detected_os)"
	@grep -E "^(ID|ID_LIKE)=" /etc/os-release

# Allow override with TARGET_OS env var
ifneq ($(TARGET_OS),)
	detected_os := $(TARGET_OS)
endif