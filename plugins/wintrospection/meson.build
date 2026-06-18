# wintrospection: Windows OS-introspection backend (osi provider).
# Ported from classic PANDA. Depends on libosi (libosi/libiohal/liboffset).
#
# libosi discovery (portable):
#   - macOS arm64: built locally at panda-plus/libosi/install-mac (no system
#     install without sudo) — add its include/lib dirs explicitly.
#   - Linux: libosi is system-installed (headers /usr/local/include, libs
#     /usr/lib) by the libosi .deb — on the default search paths, so just -l.
fs = import('fs')
libosi_prefix = meson.global_source_root() / '..' / 'libosi' / 'install-mac'

sources += files('wintrospection.cpp', 'pandamemory.cpp')
link_args += ['-losi', '-liohal', '-loffset']
if fs.is_dir(libosi_prefix / 'lib')
    # macOS local libosi build
    plugin_cflags += ['-I' + libosi_prefix / 'include']
    link_args += ['-L' + libosi_prefix / 'lib', '-Wl,-rpath,' + libosi_prefix / 'lib']
endif
