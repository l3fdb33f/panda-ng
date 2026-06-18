# wintrospection: Windows OS-introspection backend (osi provider).
# Ported from classic PANDA. Depends on libosi (libosi/libiohal/liboffset),
# built for macOS arm64 at panda-plus/libosi/install-mac (override LIBOSI_PREFIX
# below for other layouts; on Linux libosi installs to /usr/local).
libosi_prefix = meson.global_source_root() / '..' / 'libosi' / 'install-mac'

plugin_cflags += ['-I' + libosi_prefix / 'include']
sources += files('wintrospection.cpp', 'pandamemory.cpp')
link_args += [
    '-L' + libosi_prefix / 'lib',
    '-losi', '-liohal', '-loffset',
    '-Wl,-rpath,' + libosi_prefix / 'lib',
]
