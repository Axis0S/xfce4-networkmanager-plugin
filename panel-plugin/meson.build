plugin_sources = [
  'main.c',
  'plugin.c',
  'nm-interface.c',
  'popup-window.c',
  'password-dialog.c',
  'notification.c',
  'connection-editor.c',
  'settings-dialog.c',
  'status-icon.c',
  'utils.c'
]

plugin_headers = files(
  'plugin.h',
  'nm-interface.h',
  'popup-window.h',
  'password-dialog.h',
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
    libnotify_dep,
    nm_lib_dep
  ],
  c_args: [
    '-DPACKAGE_DATA_DIR="' + datadir + '"'
  ],
  install: true,
  install_dir: plugindir,
  name_prefix: ''
)

# Install CSS file
install_data('style.css',
  install_dir: join_paths(datadir, meson.project_name(), 'panel-plugin')
)

