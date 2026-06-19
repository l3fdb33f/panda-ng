#![allow(
    non_camel_case_types,
    non_upper_case_globals,
    improper_ctypes,
    non_snake_case,
    improper_ctypes_definitions
)]

#[cfg(feature = "aarch64")]
include!("autogen/aarch64.rs");

#[cfg(feature = "arm")]
include!("autogen/arm.rs");

#[cfg(feature = "i386")]
include!("autogen/i386.rs");

#[cfg(feature = "loongarch64")]
include!("autogen/loongarch64.rs");

#[cfg(feature = "mips64el")]
include!("autogen/mips64el.rs");

#[cfg(feature = "mips64")]
include!("autogen/mips64.rs");

#[cfg(feature = "mipsel")]
include!("autogen/mipsel.rs");

#[cfg(feature = "mips")]
include!("autogen/mips.rs");

#[cfg(feature = "riscv32")]
include!("autogen/riscv32.rs");

#[cfg(feature = "riscv64")]
include!("autogen/riscv64.rs");

#[cfg(feature = "ppc64")]
include!("autogen/ppc64.rs");

#[cfg(feature = "ppc")]
include!("autogen/ppc.rs");

#[cfg(feature = "x86_64")]
include!("autogen/x86_64.rs");

// PANDBox port: classic panda-sys named callback-type enum constants
// 'panda_cb_type_PANDA_CB_X'; panda-ng's bindgen emits bare 'PANDA_CB_X'.
// The high-level panda-rs crate references the prefixed names, so alias them.
#[cfg(feature = "x86_64")]
pub mod cb_type_compat {
    use super::*;
    pub const panda_cb_type_PANDA_CB_AFTER_BLOCK_EXEC: panda_cb_type = PANDA_CB_AFTER_BLOCK_EXEC;
    pub const panda_cb_type_PANDA_CB_AFTER_BLOCK_TRANSLATE: panda_cb_type = PANDA_CB_AFTER_BLOCK_TRANSLATE;
    pub const panda_cb_type_PANDA_CB_AFTER_CPU_EXEC_ENTER: panda_cb_type = PANDA_CB_AFTER_CPU_EXEC_ENTER;
    pub const panda_cb_type_PANDA_CB_AFTER_INSN_EXEC: panda_cb_type = PANDA_CB_AFTER_INSN_EXEC;
    pub const panda_cb_type_PANDA_CB_AFTER_INSN_TRANSLATE: panda_cb_type = PANDA_CB_AFTER_INSN_TRANSLATE;
    pub const panda_cb_type_PANDA_CB_AFTER_LOADVM: panda_cb_type = PANDA_CB_AFTER_LOADVM;
    pub const panda_cb_type_PANDA_CB_AFTER_MACHINE_INIT: panda_cb_type = PANDA_CB_AFTER_MACHINE_INIT;
    pub const panda_cb_type_PANDA_CB_ASID_CHANGED: panda_cb_type = PANDA_CB_ASID_CHANGED;
    pub const panda_cb_type_PANDA_CB_BEFORE_BLOCK_EXEC: panda_cb_type = PANDA_CB_BEFORE_BLOCK_EXEC;
    pub const panda_cb_type_PANDA_CB_BEFORE_BLOCK_EXEC_INVALIDATE_OPT: panda_cb_type = PANDA_CB_BEFORE_BLOCK_EXEC_INVALIDATE_OPT;
    pub const panda_cb_type_PANDA_CB_BEFORE_BLOCK_TRANSLATE: panda_cb_type = PANDA_CB_BEFORE_BLOCK_TRANSLATE;
    pub const panda_cb_type_PANDA_CB_BEFORE_CPU_EXEC_EXIT: panda_cb_type = PANDA_CB_BEFORE_CPU_EXEC_EXIT;
    pub const panda_cb_type_PANDA_CB_BEFORE_HANDLE_EXCEPTION: panda_cb_type = PANDA_CB_BEFORE_HANDLE_EXCEPTION;
    pub const panda_cb_type_PANDA_CB_BEFORE_HANDLE_INTERRUPT: panda_cb_type = PANDA_CB_BEFORE_HANDLE_INTERRUPT;
    pub const panda_cb_type_PANDA_CB_BEFORE_LOADVM: panda_cb_type = PANDA_CB_BEFORE_LOADVM;
    pub const panda_cb_type_PANDA_CB_BEFORE_TCG_CODEGEN: panda_cb_type = PANDA_CB_BEFORE_TCG_CODEGEN;
    pub const panda_cb_type_PANDA_CB_BLOCK_TRANSLATE: panda_cb_type = PANDA_CB_BLOCK_TRANSLATE;
    pub const panda_cb_type_PANDA_CB_CPU_RESTORE_STATE: panda_cb_type = PANDA_CB_CPU_RESTORE_STATE;
    pub const panda_cb_type_PANDA_CB_DURING_MACHINE_INIT: panda_cb_type = PANDA_CB_DURING_MACHINE_INIT;
    pub const panda_cb_type_PANDA_CB_END_BLOCK_EXEC: panda_cb_type = PANDA_CB_END_BLOCK_EXEC;
    pub const panda_cb_type_PANDA_CB_GUEST_HYPERCALL: panda_cb_type = PANDA_CB_GUEST_HYPERCALL;
    pub const panda_cb_type_PANDA_CB_HD_READ: panda_cb_type = PANDA_CB_HD_READ;
    pub const panda_cb_type_PANDA_CB_HD_WRITE: panda_cb_type = PANDA_CB_HD_WRITE;
    pub const panda_cb_type_PANDA_CB_INSN_EXEC: panda_cb_type = PANDA_CB_INSN_EXEC;
    pub const panda_cb_type_PANDA_CB_INSN_TRANSLATE: panda_cb_type = PANDA_CB_INSN_TRANSLATE;
    pub const panda_cb_type_PANDA_CB_LAST: panda_cb_type = PANDA_CB_LAST;
    pub const panda_cb_type_PANDA_CB_MAIN_LOOP_WAIT: panda_cb_type = PANDA_CB_MAIN_LOOP_WAIT;
    pub const panda_cb_type_PANDA_CB_MMIO_AFTER_READ: panda_cb_type = PANDA_CB_MMIO_AFTER_READ;
    pub const panda_cb_type_PANDA_CB_MMIO_BEFORE_WRITE: panda_cb_type = PANDA_CB_MMIO_BEFORE_WRITE;
    pub const panda_cb_type_PANDA_CB_MONITOR: panda_cb_type = PANDA_CB_MONITOR;
    pub const panda_cb_type_PANDA_CB_PHYS_MEM_AFTER_READ: panda_cb_type = PANDA_CB_PHYS_MEM_AFTER_READ;
    pub const panda_cb_type_PANDA_CB_PHYS_MEM_AFTER_WRITE: panda_cb_type = PANDA_CB_PHYS_MEM_AFTER_WRITE;
    pub const panda_cb_type_PANDA_CB_PHYS_MEM_BEFORE_READ: panda_cb_type = PANDA_CB_PHYS_MEM_BEFORE_READ;
    pub const panda_cb_type_PANDA_CB_PHYS_MEM_BEFORE_WRITE: panda_cb_type = PANDA_CB_PHYS_MEM_BEFORE_WRITE;
    pub const panda_cb_type_PANDA_CB_PRE_SHUTDOWN: panda_cb_type = PANDA_CB_PRE_SHUTDOWN;
    pub const panda_cb_type_PANDA_CB_QMP: panda_cb_type = PANDA_CB_QMP;
    pub const panda_cb_type_PANDA_CB_REPLAY_AFTER_DMA: panda_cb_type = PANDA_CB_REPLAY_AFTER_DMA;
    pub const panda_cb_type_PANDA_CB_REPLAY_BEFORE_DMA: panda_cb_type = PANDA_CB_REPLAY_BEFORE_DMA;
    pub const panda_cb_type_PANDA_CB_REPLAY_HANDLE_PACKET: panda_cb_type = PANDA_CB_REPLAY_HANDLE_PACKET;
    pub const panda_cb_type_PANDA_CB_REPLAY_HD_TRANSFER: panda_cb_type = PANDA_CB_REPLAY_HD_TRANSFER;
    pub const panda_cb_type_PANDA_CB_REPLAY_NET_TRANSFER: panda_cb_type = PANDA_CB_REPLAY_NET_TRANSFER;
    pub const panda_cb_type_PANDA_CB_REPLAY_SERIAL_READ: panda_cb_type = PANDA_CB_REPLAY_SERIAL_READ;
    pub const panda_cb_type_PANDA_CB_REPLAY_SERIAL_RECEIVE: panda_cb_type = PANDA_CB_REPLAY_SERIAL_RECEIVE;
    pub const panda_cb_type_PANDA_CB_REPLAY_SERIAL_SEND: panda_cb_type = PANDA_CB_REPLAY_SERIAL_SEND;
    pub const panda_cb_type_PANDA_CB_REPLAY_SERIAL_WRITE: panda_cb_type = PANDA_CB_REPLAY_SERIAL_WRITE;
    pub const panda_cb_type_PANDA_CB_START_BLOCK_EXEC: panda_cb_type = PANDA_CB_START_BLOCK_EXEC;
    pub const panda_cb_type_PANDA_CB_TOP_LOOP: panda_cb_type = PANDA_CB_TOP_LOOP;
    pub const panda_cb_type_PANDA_CB_UNASSIGNED_IO_READ: panda_cb_type = PANDA_CB_UNASSIGNED_IO_READ;
    pub const panda_cb_type_PANDA_CB_UNASSIGNED_IO_WRITE: panda_cb_type = PANDA_CB_UNASSIGNED_IO_WRITE;
    pub const panda_cb_type_PANDA_CB_VIRT_MEM_AFTER_READ: panda_cb_type = PANDA_CB_VIRT_MEM_AFTER_READ;
    pub const panda_cb_type_PANDA_CB_VIRT_MEM_AFTER_WRITE: panda_cb_type = PANDA_CB_VIRT_MEM_AFTER_WRITE;
    pub const panda_cb_type_PANDA_CB_VIRT_MEM_BEFORE_READ: panda_cb_type = PANDA_CB_VIRT_MEM_BEFORE_READ;
    pub const panda_cb_type_PANDA_CB_VIRT_MEM_BEFORE_WRITE: panda_cb_type = PANDA_CB_VIRT_MEM_BEFORE_WRITE;
}
#[cfg(feature = "x86_64")]
pub use cb_type_compat::*;

// PANDBox port: symbols the high-level panda-rs crate expects that panda-ng's
// bindgen doesn't emit. RR is driven via the QEMU monitor in the fork (no
// panda_record/replay C API); provide non-functional stubs so the crate
// compiles — cosi never records/replays, and PANDBox uses the pandare2 monitor
// RR shim. MEMTX_OK exists in autogen; add the other result codes.
pub mod pandbox_compat {
    use core::ffi::c_char;
    pub const MEMTX_ERROR: u32 = 1;
    pub const MEMTX_DECODE_ERROR: u32 = 2;
    #[allow(non_camel_case_types)]
    pub type RRCTRL_ret = i32;
    pub const RRCTRL_ret_RRCTRL_OK: RRCTRL_ret = 0;
    pub const RRCTRL_ret_RRCTRL_EINVALID: RRCTRL_ret = 1;
    pub const RRCTRL_ret_RRCTRL_EPENDING: RRCTRL_ret = 2;
    #[allow(unused_variables)]
    pub unsafe fn panda_record_begin(name: *const c_char, snap: *const c_char) -> RRCTRL_ret { RRCTRL_ret_RRCTRL_EINVALID }
    pub unsafe fn panda_record_end() -> RRCTRL_ret { RRCTRL_ret_RRCTRL_EINVALID }
    #[allow(unused_variables)]
    pub unsafe fn panda_replay_begin(name: *const c_char) -> RRCTRL_ret { RRCTRL_ret_RRCTRL_EINVALID }
    pub unsafe fn panda_replay_end() -> RRCTRL_ret { RRCTRL_ret_RRCTRL_EINVALID }
}
pub use pandbox_compat::*;
