# XFCE4 NetworkManager Plugin

A native XFCE4 panel plugin for NetworkManager that provides comprehensive network management capabilities with a lightweight, theme-aware interface.

## Features

- **Native XFCE4 Integration**: Seamlessly integrates with XFCE4 panel
- **Full NetworkManager Support**: All connection types (Ethernet, Wi-Fi, VPN, Mobile, Bluetooth)
- **Theme-Aware UI**: Respects system theme with configurable transparency
- **Quick Access**: Fast connect/disconnect from panel
- **Comprehensive Settings**: Full connection configuration interface
- **Multi-language**: Internationalization support

## Current Status

This project is in early development. The build infrastructure and project structure have been set up, but the core functionality is still being implemented.

### What's Done
- ✓ Project structure and directory layout
- ✓ Meson build system configuration
- ✓ Internationalization framework
- ✓ Development documentation

### What's Next
- [ ] Basic plugin skeleton
- [ ] NetworkManager D-Bus integration
- [ ] UI implementation
- [ ] Connection type handlers

## Building

### Dependencies

- GLib >= 2.50.0
- GTK+ >= 3.22.0
- libxfce4panel >= 4.14.0
- libxfce4ui >= 4.14.0
- NetworkManager >= 1.10.0
- libnm >= 1.10.0
- Meson >= 0.50.0
- Ninja

### Build Instructions

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install libglib2.0-dev libgtk-3-dev libxfce4panel-2.0-dev \
                 libxfce4ui-2-dev libnm-dev meson ninja-build

# Clone and build
git clone https://github.com/yourusername/xfce4-networkmanager-plugin.git
cd xfce4-networkmanager-plugin
meson setup build
cd build
ninja

# Install (optional)
sudo ninja install
```

## Development

See [DEVELOPMENT.md](DEVELOPMENT.md) for detailed development documentation, including:
- Architecture overview
- Development phases
- File structure
- Implementation details

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

This project is licensed under GPL-2.0+. See LICENSE file for details.

## Acknowledgments

Inspired by:
- GNOME network-manager-applet
- KDE plasma-nm
- XFCE4 plugin development community
