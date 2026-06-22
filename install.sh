#!/bin/bash
set -e

# Colored output utilities
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0;m' # No Color

echo -e "${BLUE}=== Secrets Manager (passmgr) Installer ===${NC}"

# 1. Detect Linux distribution
echo "Detecting operating system..."
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    # Handle derivatives
    if echo "$ID_LIKE" | grep -q "debian"; then
        DISTRO_FAMILY="debian"
    elif echo "$ID_LIKE" | grep -q "arch"; then
        DISTRO_FAMILY="arch"
    elif echo "$ID_LIKE" | grep -q "rhel\|fedora"; then
        DISTRO_FAMILY="fedora"
    elif echo "$ID_LIKE" | grep -q "suse"; then
        DISTRO_FAMILY="suse"
    else
        DISTRO_FAMILY=$ID
    fi
else
    echo -e "${RED}Error: /etc/os-release not found. Cannot detect distribution.${NC}"
    DISTRO_FAMILY="unknown"
fi

echo "Detected distribution: $NAME ($DISTRO_FAMILY)"

# 2. Check and install dependencies
if pkg-config --exists libsodium && command -v make >/dev/null 2>&1 && command -v gcc >/dev/null 2>&1; then
    echo "Dependencies (libsodium, make, gcc) are already satisfied. Skipping package manager installation."
else
    echo "Installing missing dependencies (may require sudo)..."
    case "$DISTRO_FAMILY" in
        arch|manjaro|endeavouros)
            sudo pacman -S --needed --noconfirm base-devel libsodium pkg-config
            ;;
        debian|ubuntu|mint|pop)
            sudo apt-get update
            sudo apt-get install -y build-essential libsodium-dev pkg-config
            ;;
        fedora|centos|rhel)
            sudo dnf install -y make gcc libsodium-devel pkg-config
            ;;
        suse|opensuse*)
            sudo zypper install -y make gcc libsodium-devel pkg-config
            ;;
        *)
            echo -e "${YELLOW}Warning: Unknown distribution family. Please ensure you have make, gcc, libsodium, and pkg-config installed manually.${NC}"
            ;;
    esac
fi

# 3. Verify libsodium installation
if ! pkg-config --exists libsodium; then
    echo -e "${RED}Error: libsodium was not found by pkg-config. Please install libsodium (>= 1.0.18) manually.${NC}"
    exit 1
fi
echo -e "${GREEN}libsodium version $(pkg-config --modversion libsodium) detected.${NC}"

# 4. Compile the project
echo "Compiling the application..."
make clean
make

# 5. Define installation prefix (default to local user bin)
PREFIX="$HOME/.local"
echo -e "Installing binary to ${BLUE}$PREFIX/bin${NC}..."
mkdir -p "$PREFIX/bin"

# Compile and copy binary
make PREFIX="$PREFIX" install

# 6. Install Firefox Native Messaging manifest
echo "Installing Firefox Native Messaging host manifest..."
make PREFIX="$PREFIX" install-native

# 7. Check if $PREFIX/bin is in $PATH
if [[ ":$PATH:" != *":$PREFIX/bin:"* ]]; then
    echo -e "${YELLOW}Warning: $PREFIX/bin is not in your PATH.${NC}"
    echo "To be able to run 'passmgr' from anywhere, add this to your ~/.bashrc or ~/.zshrc:"
    echo -e "${BLUE}  export PATH=\$PATH:\$HOME/.local/bin${NC}"
fi

echo -e "${GREEN}=== Installation Completed Successfully! ===${NC}"
echo "You can now open Firefox, load the extension, and it will communicate with the C host at: $PREFIX/bin/passmgr"
