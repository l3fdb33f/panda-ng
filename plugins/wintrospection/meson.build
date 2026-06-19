# wintrospection: Windows OS-introspection backend (osi provider). Single
# translation unit (pandamemory.cpp is #included at the end of wintrospection.cpp)
# so the plugin builds directly into the shared object and exports init_plugin /
# uninit_plugin for the loader (the main/secondary static-lib split drops a main
# source's symbols when the secondary doesn't reference it). Depends on libosi.
fs = import('fs')
libosi_prefix = meson.global_source_root() / '..' / 'libosi' / 'install-mac'
sources += files('wintrospection.cpp')
link_args += ['-losi', '-liohal', '-loffset']
if fs.is_dir(libosi_prefix / 'lib')
    plugin_cflags += ['-I' + libosi_prefix / 'include']
    link_args += ['-L' + libosi_prefix / 'lib', '-Wl,-rpath,' + libosi_prefix / 'lib']
endif
