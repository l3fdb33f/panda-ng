#[cfg(any(feature = "i386", feature = "x86_64"))]
pub type CPUArchPtr = *mut panda_sys::CPUX86State;

#[cfg(any(feature = "arm", feature = "aarch64"))]
pub type CPUArchPtr = *mut panda_sys::CPUARMState;

#[cfg(any(
    feature = "mips",
    feature = "mipsel",
    feature = "mips64",
    feature = "mips64el"
))]
pub type CPUArchPtr = *mut panda_sys::CPUMIPSState;

#[cfg(feature = "ppc")]
pub type CPUArchPtr = *mut panda_sys::CPUPPCState;

#[macro_export]
macro_rules! cpu_arch_state {
    // panda-ng: modern QEMU CPUState has no `env_ptr` field; get the arch env
    // via the panda_cpu_env() helper instead.
    ($cpu:expr) => {
        unsafe { $crate::sys::panda_cpu_env($cpu as *const _ as *mut _) as CPUArchPtr }
    };
}
