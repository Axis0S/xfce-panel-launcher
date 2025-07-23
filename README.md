Enjoying the project? If you'd like to support its future development, you can do so via <a href="https://ko-fi.com/axisos">
  <img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Support me on Ko-fi" height="35">
</a> or <a href="https://paypal.me/AxisOS">
  <img src="https://img.shields.io/badge/PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white" alt="Donate with PayPal" height="40">
</a> Any support is greatly appreciated!

# XFCE Launcher Plugin

A modern, full-screen application launcher plugin for XFCE Panel, inspired by macOS Launchpad. This plugin provides a visually appealing and efficient way to access all your installed applications with a single click.

![Screenshot](screenshots/launcher.png)

## Version

0.6

## Overview

The XFCE Launcher Plugin transforms your desktop experience by providing a full-screen application grid that overlays your current workspace. It's designed to be fast, intuitive, and seamlessly integrated with the XFCE desktop environment.

## Key Features

### Visual Design
- **Full-screen overlay**: Covers your entire screen with a semi-transparent dark background
- **Grid layout**: Applications are organized in a responsive grid that adapts to your screen size
- **Smooth animations**: Hover effects and transitions provide visual feedback
- **Modern icons**: Displays application icons at optimal size with proper scaling
- **Clean interface**: Minimalist design that focuses on your applications

### Functionality
- **Instant search**: Start typing to filter applications in real-time
- **Smart filtering**: Search matches application names and descriptions
- **Quick launch**: Single click to launch any application
- **Keyboard navigation**: 
  - Press ESC to close the launcher
  - Type to search without clicking the search box
  - Enter to launch the first search result
- **Right-click context menu**: Access additional options for each application
- **Hide applications**: Right-click and select "Hide" to remove unwanted apps from view
- **Unhide functionality**: Hidden applications can be restored through settings
- **Snap support**: Automatically discovers and displays Snap applications
- **Flatpak support**: Automatically discovers and displays Flatpak applications
- **Live refresh**: Automatically updates when applications are installed or uninstalled

### Integration
- **Panel integration**: Adds seamlessly to your XFCE panel as a launcher button
- **Customizable panel icon**: Change the launcher button icon to match your preferences
- **Theme aware**: Respects your GTK theme for consistent appearance
- **Icon theme support**: Uses your system icon theme for application icons
- **Desktop file compliance**: Reads standard .desktop files from system directories

### Performance
- **Lightweight**: Minimal resource usage when not active
- **Fast loading**: Applications are cached for instant display
- **Dynamic updates**: Automatically detects newly installed or removed applications
- **Real-time monitoring**: No need to logout/login when installing new apps
- **Efficient rendering**: Uses GTK3 for hardware-accelerated graphics

## Dependencies

You need the following development packages installed:

- gtk+-3.0
- libxfce4panel-2.0
- libxfce4util-1.0
- libxfconf-0
- gio-2.0
- gcc
- make
- pkg-config

On Debian/Ubuntu, install them with:
```bash
sudo apt install libgtk-3-dev libxfce4panel-2.0-dev libxfce4util-dev libxfconf-0-dev libgio2.0-cil-dev gcc make pkg-config
```

On Arch Linux, install them with:
```bash
sudo pacman -S gtk3 xfce4-panel xfconf xfce4-dev-tools gcc make pkg-config
```


## Installation


```bash
./install.sh
```

This installs the plugin to the system directories:
- Plugin library: `/usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/` (on Debian/Ubuntu)
- Plugin library: `/usr/lib64/xfce4/panel/plugins/` (on Fedora/Redhat)
- Plugin library: `/usr/lib/xfce4/panel/plugins/` (on Arch and other Distribution)
- Desktop file: `/usr/share/xfce4/panel/plugins/`


## Usage

1. After installation, restart XFCE Panel:
   ```bash
   xfce4-panel -r
   ```

2. Right-click on your XFCE panel and select "Panel" → "Add New Items..."

3. Find "Application Launcher" in the list and add it

4. Click the launcher icon in your panel to open the full-screen launcher

5. Type to search for applications or click on any app to launch it

6. Press ESC to close the launcher

## Uninstallation

For system-wide installation:
```bash
./uninstall.sh
```


## Customization

### Changing the Panel Icon

1. Right-click on the launcher button in your panel
2. Select "Properties" from the context menu
3. In the settings dialog:
   - Click on the icon button to open the icon chooser
   - Select from common launcher icons displayed in a grid
   - Or enter a custom icon name in the text field
   - Or click "Browse..." to select an image file from your computer
   - Supported formats: SVG, PNG, JPEG
   - Click "Reset to Default" to restore the original launcher icon
4. Click "Close" to apply your changes

The plugin will remember your icon choice and use it every time the panel starts. You can use:
- Icon names from your current icon theme
- Absolute paths to image files on your system
- Common launcher icons like `application-menu`, `show-apps`, etc.

### Styling

The plugin uses GTK CSS for styling. You can modify the appearance by editing the CSS in the source code (`src/ui.c`).

## Troubleshooting

If the plugin doesn't appear after installation:

1. Make sure XFCE Panel is restarted: `xfce4-panel -r`
2. Check if the plugin files are in the correct location:
   - System installation (Debian/Ubuntu):
     - Plugin library: `/usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `/usr/share/xfce4/panel/plugins/xfce-launcher.desktop`
   - System installation (Fedora/RedHat):
     - Plugin library: `/usr/lib64/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `/usr/share/xfce4/panel/plugins/xfce-launcher.desktop`
   - System installation (Arch and other distributions):
     - Plugin library: `/usr/lib/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `/usr/share/xfce4/panel/plugins/xfce-launcher.desktop`
3. Run XFCE Panel in debug mode to see any errors: `xfce4-panel -q && PANEL_DEBUG=1 xfce4-panel`

### Snap and Flatpak Applications

The launcher automatically detects Snap and Flatpak applications from these locations:
- Snap: `/var/lib/snapd/desktop/applications/`
- Flatpak (system): `/var/lib/flatpak/exports/share/applications/`
- Flatpak (user): `~/.local/share/flatpak/exports/share/applications/`

If Snap or Flatpak applications don't appear:
1. Ensure the applications are properly installed
2. Check that the `.desktop` files exist in the above directories
3. Some Snap/Flatpak apps may take a moment to appear after installation

## Author

**Kamil 'Novik' Nowicki**
- Email: novik@axisos.org
- Website: www.axisos.org
- GitHub: https://github.com/Axis0S/xfce-panel-launcher

## License

Copyright (C) 2025 Kamil 'Novik' Nowicki

This plugin is provided as-is for educational and personal use.
