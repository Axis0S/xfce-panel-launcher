#!/bin/bash
set -e

# Intelligent installation script for XFCE launcher plugin
# Detects distribution and installs to the correct directory

COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_YELLOW='\033[1;33m'
COLOR_NC='\033[0m' # No Color

# Global variables
DISTRO=""
DISTRO_VERSION=""
PLUGIN_LIB_DIR=""
PLUGIN_DESKTOP_DIR="/usr/share/xfce4/panel/plugins"
USE_SUDO=true
ARCH=""

function print_error() {
    echo -e "${COLOR_RED}Error: $1${COLOR_NC}" >&2
}

function print_success() {
    echo -e "${COLOR_GREEN}$1${COLOR_NC}"
}

function print_info() {
    echo -e "${COLOR_YELLOW}$1${COLOR_NC}"
}

function detect_architecture() {
    ARCH=$(uname -m)
    case $ARCH in
        x86_64)
            print_info "Architecture: x86_64 (64-bit)"
            ;;
        i?86)
            ARCH="i386"
            print_info "Architecture: i386 (32-bit)"
            ;;
        aarch64)
            print_info "Architecture: ARM64"
            ;;
        armv7*)
            ARCH="armhf"
            print_info "Architecture: ARMv7"
            ;;
        *)
            print_info "Architecture: $ARCH"
            ;;
    esac
}

function detect_distribution() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        DISTRO=$ID
        DISTRO_VERSION=$VERSION_ID
        print_info "Detected: $NAME $VERSION"
    elif [ -f /etc/debian_version ]; then
        DISTRO="debian"
        DISTRO_VERSION=$(cat /etc/debian_version)
        print_info "Detected: Debian $DISTRO_VERSION"
    elif [ -f /etc/redhat-release ]; then
        DISTRO="rhel"
        print_info "Detected: Red Hat based system"
    else
        print_error "Could not detect distribution"
        exit 1
    fi
}

function find_xfce_plugin_dir() {
    print_info "Searching for XFCE panel plugin directory..."
    
    # First, try to find where existing plugins are installed
    local existing_plugin
    existing_plugin=$(find /usr/lib* -name "libactions.so" -path "*xfce4/panel/plugins*" 2>/dev/null | head -1)
    
    if [ -n "$existing_plugin" ]; then
        PLUGIN_LIB_DIR=$(dirname "$existing_plugin")
        print_success "Found existing plugins in: $PLUGIN_LIB_DIR"
        return 0
    fi
    
    # If no existing plugins found, determine based on distribution
    case $DISTRO in
        debian|ubuntu|linuxmint)
            # Check for multiarch
            if command -v dpkg-architecture &> /dev/null; then
                local multiarch=$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null)
                if [ -n "$multiarch" ]; then
                    PLUGIN_LIB_DIR="/usr/lib/$multiarch/xfce4/panel/plugins"
                fi
            fi
            ;;
        fedora|rhel|centos|rocky|almalinux)
            if [ "$ARCH" = "x86_64" ]; then
                PLUGIN_LIB_DIR="/usr/lib64/xfce4/panel/plugins"
            else
                PLUGIN_LIB_DIR="/usr/lib/xfce4/panel/plugins"
            fi
            ;;
        arch|manjaro|endeavouros)
            PLUGIN_LIB_DIR="/usr/lib/xfce4/panel/plugins"
            ;;
        opensuse*)
            if [ "$ARCH" = "x86_64" ]; then
                PLUGIN_LIB_DIR="/usr/lib64/xfce4/panel/plugins"
            else
                PLUGIN_LIB_DIR="/usr/lib/xfce4/panel/plugins"
            fi
            ;;
        *)
            # Default fallback
            PLUGIN_LIB_DIR="/usr/lib/xfce4/panel/plugins"
            ;;
    esac
    
    print_info "Will use: $PLUGIN_LIB_DIR"
}

function check_dependencies() {
    print_info "Checking dependencies..."
    
    local missing_deps=()
    
    # Check for required commands
    for cmd in gcc make pkg-config; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=("$cmd")
        fi
    done
    
    # Check for required libraries
    for lib in gtk+-3.0 libxfce4panel-2.0 libxfce4util-1.0 gio-2.0; do
        if ! pkg-config --exists $lib 2>/dev/null; then
            missing_deps+=("$lib")
        fi
    done
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_info "Please install the required dependencies:"
        
        case $DISTRO in
            debian|ubuntu|linuxmint)
                echo "sudo apt-get install build-essential libgtk-3-dev libxfce4panel-2.0-dev libxfce4util-dev"
                ;;
            fedora|rhel|centos)
                echo "sudo dnf install gcc make gtk3-devel xfce4-panel-devel libxfce4util-devel"
                ;;
            arch|manjaro)
                echo "sudo pacman -S base-devel gtk3 xfce4-panel xfce4-dev-tools"
                ;;
            opensuse*)
                echo "sudo zypper install gcc make gtk3-devel xfce4-panel-devel libxfce4util-devel"
                ;;
        esac
        return 1
    fi
    
    print_success "All dependencies satisfied"
    return 0
}

function build_plugin() {
    print_info "Building plugin..."
    
    if [ -f "libxfce-launcher.so" ]; then
        make clean
    fi
    
    if make; then
        print_success "Build successful"
        return 0
    else
        print_error "Build failed"
        return 1
    fi
}

function install_plugin() {
    print_info "Installing plugin..."
    
    # Check if we need sudo
    if [[ "$PLUGIN_LIB_DIR" == "$HOME"* ]]; then
        USE_SUDO=false
        print_info "Installing to user directory (no sudo required)"
    else
        print_info "Installing to system directory (sudo required)"
    fi
    
    # Create directories
    if $USE_SUDO; then
        sudo mkdir -p "$PLUGIN_LIB_DIR"
        sudo mkdir -p "$PLUGIN_DESKTOP_DIR"
    else
        mkdir -p "$PLUGIN_LIB_DIR"
        mkdir -p "$PLUGIN_DESKTOP_DIR"
    fi
    
    # Copy files
    if $USE_SUDO; then
        sudo cp -v libxfce-launcher.so "$PLUGIN_LIB_DIR/"
        sudo cp -v xfce-launcher.desktop "$PLUGIN_DESKTOP_DIR/"
    else
        cp -v libxfce-launcher.so "$PLUGIN_LIB_DIR/"
        cp -v xfce-launcher.desktop "$PLUGIN_DESKTOP_DIR/"
    fi
    
    print_success "Installation complete!"
}

function restart_panel() {
    print_info "Restarting XFCE Panel..."
    xfce4-panel -r &
    sleep 2
    print_success "Panel restarted"
}

function main() {
    echo "=== XFCE Launcher Plugin Installation ==="
    echo ""
    
    # Detect system information
    detect_architecture
    detect_distribution
    
    # Check dependencies
    if ! check_dependencies; then
        exit 1
    fi
    
    # Find installation directory
    find_xfce_plugin_dir
    
    # Ask for confirmation
    echo ""
    echo "Installation details:"
    echo "  Plugin library: $PLUGIN_LIB_DIR/libxfce-launcher.so"
    echo "  Desktop file: $PLUGIN_DESKTOP_DIR/xfce-launcher.desktop"
    echo ""
    read -p "Continue with installation? (y/N) " -n 1 -r
    echo ""
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "Installation cancelled"
        exit 0
    fi
    
    # Build and install
    if build_plugin; then
        install_plugin
        
        # Ask to restart panel
        read -p "Restart XFCE Panel now? (y/N) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            restart_panel
        fi
        
        echo ""
        print_success "Installation finished!"
        echo "To add the launcher to your panel:"
        echo "1. Right-click on the panel"
        echo "2. Select 'Panel' → 'Add New Items...'"
        echo "3. Find 'Application Launcher' and add it"
    else
        exit 1
    fi
}

# Run main function
main


