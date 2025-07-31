data_files = [
  install_data('xfce4-networkmanager-plugin.desktop.in',
               install_dir: xfce_datadir / 'applications')
]

install_subdir('icons',
  install_dir: get_option('datadir') / 'icons'
)

install_subdir('ui',
  install_dir: get_option('datadir') / 'xfce4/panel/plugins'
)

install_data('ui/popup-window.ui', 'ui/connection-editor.ui', 'ui/settings-dialog.ui',
  install_dir: get_option('datadir') / 'xfce4/panel/plugins/ui'
)
