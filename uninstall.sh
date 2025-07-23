#!/bin/bash
set -e

# Intelligent uninstallation script for XFCE launcher plugin

COLOR_RED='\033[0;31m'
COLOR_GREEN='\033[0;32m'
COLOR_YELLOW='\033[1;33m'
COLOR_NC='\033[0m' # No Color

function print_error() {
    echo -e "${COLOR_RED}Error: $1${COLOR_NC}" >&2
}

function print_success() {
    echo -e "${COLOR_GREEN}$1${COLOR_NC}"
}

function print_info() {
    echo -e "${COLOR_YELLOW}$1${COLOR_NC}"
}

function find_installed_plugin() {
    print_info "Searching for installed plugin..."
    
    local locations=(
        "/usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/libxfce-launcher.so"
        "/usr/lib/i386-linux-gnu/xfce4/panel/plugins/libxfce-launcher.so"
        "/usr/lib/aarch64-linux-gnu/xfce4/panel/plugins/libxfce-launcher.so"
        "/usr/lib/armhf-linux-gnu/xfce4/panel/plugins/libxfce-launcher.so"
        "/usr/lib64/xfce4/panel/plugins/libxfce-launcher.so"
        "/usr/lib/xfce4/panel/plugins/libxfce-launcher.so"
        "/usr/local/lib/xfce4/panel/plugins/libxfce-launcher.so"
        "$HOME/.local/lib/xfce4/panel/plugins/libxfce-launcher.so"
    )
    
    local desktop_locations=(
        "/usr/share/xfce4/panel/plugins/xfce-launcher.desktop"
        "/usr/local/share/xfce4/panel/plugins/xfce-launcher.desktop"
        "$HOME/.local/share/xfce4/panel/plugins/xfce-launcher.desktop"
    )
    
    local found_lib=""
    local found_desktop=""
    
    # Find library file
    for loc in "${locations[@]}"; do
        if [ -f "$loc" ]; then
            found_lib="$loc"
            print_success "Found plugin library: $loc"
            break
        fi
    done
    
    # Find desktop file
    for loc in "${desktop_locations[@]}"; do
        if [ -f "$loc" ]; then
            found_desktop="$loc"
            print_success "Found desktop file: $loc"
            break
        fi
    done
    
    if [ -z "$found_lib" ] && [ -z "$found_desktop" ]; then
        print_error "Plugin not found in any standard location"
        return 1
    fi
    
    echo "$found_lib|$found_desktop"
    return 0
}

function remove_plugin() {
    local lib_file="$1"
    local desktop_file="$2"
    local use_sudo=true
    
    print_info "Removing plugin files..."
    
    # Check if we need sudo
    if [[ "$lib_file" == "$HOME"* ]] || [[ "$desktop_file" == "$HOME"* ]]; then
        use_sudo=false
        print_info "Removing from user directory (no sudo required)"
    else
        print_info "Removing from system directory (sudo required)"
    fi
    
    # Remove library file
    if [ -n "$lib_file" ] && [ -f "$lib_file" ]; then
        if $use_sudo; then
            sudo rm -v "$lib_file"
        else
            rm -v "$lib_file"
        fi
    fi
    
    # Remove desktop file
    if [ -n "$desktop_file" ] && [ -f "$desktop_file" ]; then
        if $use_sudo; then
            sudo rm -v "$desktop_file"
        else
            rm -v "$desktop_file"
        fi
    fi
    
    print_success "Plugin removed successfully!"
}

function restart_panel() {
    print_info "Restarting XFCE Panel..."
    xfce4-panel -r &
    sleep 2
    print_success "Panel restarted"
}

function main() {
    echo "=== XFCE Launcher Plugin Uninstallation ==="
    echo ""
    
    # Find installed plugin
    result=$(find_installed_plugin) || {
        echo ""
        echo "No installation found. Nothing to uninstall."
        exit 0
    }
    
    IFS='|' read -r lib_file desktop_file <<< "$result"
    
    # Ask for confirmation
    echo ""
    echo "Files to be removed:"
    [ -n "$lib_file" ] && echo "  - $lib_file"
    [ -n "$desktop_file" ] && echo "  - $desktop_file"
    echo ""
    read -p "Continue with uninstallation? (y/N) " -n 1 -r
    echo ""
    
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "Uninstallation cancelled"
        exit 0
    fi
    
    # Remove plugin
    remove_plugin "$lib_file" "$desktop_file"
    
    # Ask to restart panel
    read -p "Restart XFCE Panel now? (y/N) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        restart_panel
    fi
    
    echo ""
    print_success "Uninstallation complete!"
}

# Run main function
main
