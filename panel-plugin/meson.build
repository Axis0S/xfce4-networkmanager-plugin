plugin_sources = files(
  'main.c',
  'plugin.c',
  'nm-interface.c',
  'popup-window.c',
  'connection-editor.c',
  'settings-dialog.c',
  'status-icon.c',
  'utils.c'
)

plugin_headers = files(
  'plugin.h',
  'nm-interface.h',
  'popup-window.h',
  'connection-editor.h',
  'settings-dialog.h',
  'status-icon.h',
  'utils.h'
)

shared_library('networkmanager',
  plugin_sources,
  dependencies: [
    glib_dep,
    gtk_dep,
    libxfce4panel_dep,
    libxfce4ui_dep,
    libnm_dep,
    nm_lib_dep
  ],
  install: true,
  install_dir: plugindir,
  name_prefix: ''
)

