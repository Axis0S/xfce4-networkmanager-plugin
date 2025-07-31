test_nm_interface = executable('test-nm-interface',
  'test-nm-interface.c',
  dependencies: [
    glib_dep,
    gtk_dep,
    libnm_dep,
    nm_lib_dep
  ],
  install: false
)

test_connections = executable('test-connections',
  'test-connections.c',
  dependencies: [
    glib_dep,
    gtk_dep,
    libnm_dep,
    nm_lib_dep
  ],
  install: false
)

test('nm-interface', test_nm_interface)
test('connections', test_connections)
