# Installation Guide

This project provides multiple ways to install the XFCE Launcher Plugin, depending on your needs and distribution.

## Quick Installation (Recommended)

Use the intelligent installation script that automatically detects your system:

```bash
./install.sh
```

This script will:
- Detect your Linux distribution and architecture
- Find the correct installation directory by checking where other XFCE plugins are installed
- Check for required dependencies
- Build the plugin
- Install it to the correct location
- Optionally restart the XFCE panel

## Uninstallation

To remove the plugin:

```bash
./uninstall.sh
```

## Manual Installation

If you prefer manual control:

```bash
make
sudo make install
```

## Distribution-Specific Packages

### Debian/Ubuntu Package

Build a proper .deb package:

```bash
./build-binary-deb.sh
sudo apt install ../xfce4-panel-launcher_0.4-1_amd64.deb
```

### Other Distributions

The install script supports:
- **Debian/Ubuntu**: Uses multiarch paths (e.g., `/usr/lib/x86_64-linux-gnu/xfce4/panel/plugins/`)
- **Fedora/RHEL/CentOS**: Uses `/usr/lib64/xfce4/panel/plugins/` on 64-bit systems
- **Arch/Manjaro**: Uses `/usr/lib/xfce4/panel/plugins/`
- **openSUSE**: Uses `/usr/lib64/xfce4/panel/plugins/` on 64-bit systems

## Installation Paths

The plugin installs to different locations based on your distribution:

| Distribution | Plugin Library Path | Desktop File Path |
|-------------|-------------------|------------------|
| Debian/Ubuntu | `/usr/lib/{arch}-linux-gnu/xfce4/panel/plugins/` | `/usr/share/xfce4/panel/plugins/` |
| Fedora (64-bit) | `/usr/lib64/xfce4/panel/plugins/` | `/usr/share/xfce4/panel/plugins/` |
| Arch Linux | `/usr/lib/xfce4/panel/plugins/` | `/usr/share/xfce4/panel/plugins/` |
| User Install | `~/.local/lib/xfce4/panel/plugins/` | `~/.local/share/xfce4/panel/plugins/` |

## Dependencies

Before installation, ensure you have:

### Debian/Ubuntu
```bash
sudo apt-get install build-essential libgtk-3-dev libxfce4panel-2.0-dev libxfce4util-dev
```

### Fedora/RHEL
```bash
sudo dnf install gcc make gtk3-devel xfce4-panel-devel libxfce4util-devel
```

### Arch Linux
```bash
sudo pacman -S base-devel gtk3 xfce4-panel xfce4-dev-tools
```

### openSUSE
```bash
sudo zypper install gcc make gtk3-devel xfce4-panel-devel libxfce4util-devel
```

## Troubleshooting

If the installation script can't find the correct directory:
1. Check where existing XFCE plugins are installed:
   ```bash
   find /usr/lib* -name "*.so" -path "*xfce4/panel/plugins*"
   ```

2. Use manual installation with the correct path:
   ```bash
   make
   sudo cp libxfce-launcher.so /path/to/xfce4/panel/plugins/
   sudo cp xfce-launcher.desktop /usr/share/xfce4/panel/plugins/
   ```

3. Restart the XFCE panel:
   ```bash
   xfce4-panel -r
   ```
