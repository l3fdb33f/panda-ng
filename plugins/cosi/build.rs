// Plugin cdylibs resolve panda_*/qemu_* symbols from libpanda at load time.
// On macOS the linker rejects undefined symbols by default, so allow dynamic
// lookup (the C/C++ plugins get this via meson's b_lundef=false). No-op on Linux.
fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("macos") {
        println!("cargo:rustc-link-arg=-Wl,-undefined,dynamic_lookup");
    }
}
