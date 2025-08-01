project('xfce4-networkmanager-plugin', 'c',
  version: '0.1.0',
  license: 'GPL-2.0+',
  meson_version: '>= 0.50.0',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'buildtype=debugoptimized'
  ]
)

# Get directories
prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))
libdir = join_paths(prefix, get_option('libdir'))
localedir = join_paths(prefix, get_option('localedir'))

# Plugin specific directories
plugindir = join_paths(libdir, 'xfce4', 'panel', 'plugins')

# Dependencies
glib_dep = dependency('glib-2.0', version: '>= 2.50.0')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')
libxfce4panel_dep = dependency('libxfce4panel-2.0', version: '>= 4.14.0')
libxfce4ui_dep = dependency('libxfce4ui-2', version: '>= 4.14.0')
libnm_dep = dependency('libnm', version: '>= 1.10.0')
libnotify_dep = dependency('libnotify', version: '>= 0.7.0')

# Compiler flags
add_global_arguments('-DGETTEXT_PACKAGE="' + meson.project_name() + '"', language: 'c')
add_global_arguments('-DPACKAGE_LOCALE_DIR="' + localedir + '"', language: 'c')

# Subdirectories
subdir('po')
subdir('data')
subdir('lib')
subdir('panel-plugin')
if get_option('tests')
  subdir('tests')
endif

