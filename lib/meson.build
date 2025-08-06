lib_sources = files(
  'nm-connection.c',
  'nm-device.c',
  'connection-types/ethernet.c',
  'connection-types/wifi.c',
  'connection-types/vpn.c',
  'connection-types/mobile.c',
  'connection-types/bluetooth.c'
)

lib_headers = files(
  'nm-connection.h',
  'nm-device.h',
  'connection-types/ethernet.h',
  'connection-types/wifi.h',
  'connection-types/vpn.h',
  'connection-types/mobile.h',
  'connection-types/bluetooth.h'
)

nm_lib = static_library('xfce4-nm-lib',
  sources: lib_sources,
  dependencies: [
    glib_dep,
    gtk_dep,
    libnm_dep,
    dependency('uuid')
  ],
  install: false
)

nm_lib_dep = declare_dependency(
  link_with: nm_lib,
  include_directories: include_directories('.')
)
