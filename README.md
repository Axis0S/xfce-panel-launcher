# XFCE Launcher Plugin

A full-screen application launcher plugin for XFCE Panel, similar to macOS Launchpad.

## Version

0.4

## Features

- Full-screen overlay with semi-transparent background
- Grid layout showing all installed applications
- Search functionality to filter applications
- Smooth hover effects on application icons
- **Hide Applications**: Right-click any application and select "Hide" to remove it from view
- **Dynamic Updates**: Automatically detects newly installed or removed applications
- Press ESC to close the launcher
- Click any application to launch it

## Dependencies

You need the following development packages installed:

- gtk+-3.0
- libxfce4panel-2.0
- libxfce4util-1.0
- gio-2.0
- gcc
- make
- pkg-config

On Arch Linux, install them with:
```bash
sudo pacman -S gtk3 xfce4-panel xfce4-dev-tools gcc make pkg-config
```

## Building

### Simple Build (Recommended)

Use the simple Makefile:

```bash
make
```

### Using Autotools

If you prefer to use the autotools build system:

```bash
mkdir m4
xdt-autogen
./configure --prefix=/usr
make
```

## Installation

### System-wide Installation (Recommended)

```bash
make
sudo make install
```

This installs the plugin to the system directories:
- Plugin library: `/usr/lib/xfce4/panel/plugins/`
- Desktop file: `/usr/share/xfce4/panel/plugins/`

### Local User Installation

For testing without sudo privileges:

```bash
make
make install-local
```

This installs to your user directory (`~/.local/lib/xfce4/panel/plugins/`).

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
sudo make uninstall
```

For local user installation:
```bash
make uninstall-local
```

## Customization

The plugin uses GTK CSS for styling. You can modify the appearance by editing the CSS in the source code (`xfce-launcher.c`).

## Troubleshooting

If the plugin doesn't appear after installation:

1. Make sure XFCE Panel is restarted: `xfce4-panel -r`
2. Check if the plugin files are in the correct location:
   - System installation:
     - Plugin library: `/usr/lib/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `/usr/share/xfce4/panel/plugins/xfce-launcher.desktop`
   - Local installation:
     - Plugin library: `~/.local/lib/xfce4/panel/plugins/libxfce-launcher.so`
     - Desktop file: `~/.local/share/xfce4/panel/plugins/xfce-launcher.desktop`
3. Run XFCE Panel in debug mode to see any errors: `xfce4-panel -q && PANEL_DEBUG=1 xfce4-panel`

## License

This plugin is provided as-is for educational and personal use.
