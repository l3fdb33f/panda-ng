/* osi_mac: macOS (XNU) OSI provider for panda-ng.
 *
 * Enumerates guest processes by walking the kernel `allproc` list. Handles the
 * Monterey boot kernelcache (MH_FILESET) by locating the live KC Mach-O header
 * and per-segment-rebasing the KDK-dSYM symbol addresses (a uniform KASLR slide
 * does NOT work — the kernelcache relinks segments at independent bases).
 *
 * Offsets/addresses are for macOS Monterey 12.6 / xnu-8020.240.7 (build 21G115),
 * derived from the KDK kernel dSYM (dwarf2json ISF).
 */
#define __STDC_FORMAT_MACROS
#include "panda.h"

bool init_plugin(void *);
void uninit_plugin(void *);
#include "osi/osi_types.h"
#include "osi/os_intro.h"
#include "osi/osi_ext.h"

/* ---- XNU layout constants (Monterey 12.6 / xnu-8020.240.7) ---- */
#define KBASE              0xffffff8000000000ULL
/* dSYM static symbol addresses (from the ISF) */
#define SYM_ALLPROC        0xffffff8000e93770ULL
#define SYM_CPU_DATA_PTR   0xffffff8000c60000ULL  /* cpu_data_t *cpu_data_ptr[] */
/* struct proc field offsets (validated against the live kernel) */
#define OFF_P_LIST_NEXT    0     /* proc.p_list.le_next */
#define OFF_P_TASK         16    /* proc.task           */
#define OFF_P_PPID         40    /* proc.p_ppid         */
#define OFF_P_PID          104   /* proc.p_pid          */
/* current-process chain: cpu_data -> thread -> task -> proc */
#define OFF_CD_ACTIVE_THREAD 16   /* cpu_data.cpu_active_thread */
#define OFF_THREAD_T_TASK    1584 /* thread.t_task              */
#define OFF_THREAD_ID        1784 /* thread.thread_id (tid)     */
#define OFF_TASK_BSD_INFO    928  /* task.bsd_info (the proc)   */
/* address space: proc -> task.map -> vm_map.pmap -> pmap.pm_cr3 */
#define OFF_TASK_MAP         40   /* task.map (vm_map *)        */
#define OFF_VMMAP_PMAP       64   /* _vm_map.pmap               */
#define OFF_PMAP_CR3         64   /* pmap.pm_cr3                */
/* kexts: kmod global -> kmod_info linked list */
#define SYM_KMOD             0xffffff8000e28848ULL
#define OFF_KMOD_NEXT        0
#define OFF_KMOD_NAME        16   /* char name[64] */
#define OFF_KMOD_ADDR        156  /* vm_address_t  */
#define OFF_KMOD_SIZE        164  /* vm_size_t     */
#define NAME_WIN_OFF       1020  /* window covering p_comm(~+1029)/p_name(~+1046) */
#define NAME_WIN_LEN       64
#define MAX_PROCS          4096

/* dSYM segment table (name, vmaddr, size) — symbols are rebased per segment. */
struct seg { const char *name; uint64_t base, size; };
static const struct seg dsym_segs[] = {
    {"__HIB",        0xffffff8000100000ULL, 0xa0000},
    {"__TEXT",       0xffffff8000200000ULL, 0xa00000},
    {"__DATA",       0xffffff8000c00000ULL, 0x297000},
    {"__DATA_CONST", 0xffffff8000e97000ULL, 0x88000},
};
#define N_DSYM_SEGS (sizeof(dsym_segs)/sizeof(dsym_segs[0]))

/* resolved runtime state (cached after first successful resolve) */
static bool     resolved = false;
static uint64_t kc_seg_rt[N_DSYM_SEGS]; /* runtime base of each dsym_segs entry */
static uint64_t allproc_rt = 0;
static uint64_t cpu_data_ptr_rt = 0;
static uint64_t kc_header_rt = 0;   /* live kernelcache Mach-O header */

/* ---- memory helpers ---- */
static inline bool rd(CPUState *cpu, uint64_t va, void *buf, int len) {
    return panda_virtual_memory_read(cpu, va, (uint8_t *)buf, len) == MEMTX_OK;
}
static inline uint64_t rdq(CPUState *cpu, uint64_t va) {
    uint64_t v = 0; rd(cpu, va, &v, 8); return v;
}
static inline uint32_t rdw(CPUState *cpu, uint64_t va) {
    uint32_t v = 0; rd(cpu, va, &v, 4); return v;
}
static inline bool is_kptr(uint64_t v) { return v >= KBASE; }
static uint64_t rebase(uint64_t static_addr);

/* Locate the live kernelcache Mach-O header (magic feedfacf, filetype 0xc). */
static uint64_t find_kc_header(CPUState *cpu) {
    /* fast path: the KC links __TEXT at this base on this setup */
    uint64_t guess = 0xffffff8005a00000ULL;
    if (rdw(cpu, guess) == 0xfeedfacf && rdw(cpu, guess + 12) == 0xc)
        return guess;
    /* fallback: wide scan (handles per-boot KC slide), 2MB-aligned */
    for (uint64_t a = 0xffffff8004000000ULL; a < 0xffffff8030000000ULL; a += 0x200000ULL) {
        if (rdw(cpu, a) == 0xfeedfacf && rdw(cpu, a + 12) == 0xc)
            return a;
    }
    return 0;
}

/* Parse KC LC_SEGMENT_64 runtime bases and rebase allproc. Runs once. */
static bool resolve(CPUState *cpu) {
    if (resolved) return true;
    if (!panda_in_kernel(cpu)) return false;   /* need kernel CR3 for kernel VAs */
    uint64_t kc = find_kc_header(cpu);
    if (!kc) return false;
    uint32_t ncmds = rdw(cpu, kc + 16);
    if (ncmds == 0 || ncmds > 4096) return false;
    uint64_t off = kc + 32;
    for (uint32_t i = 0; i < ncmds; i++) {
        uint32_t cmd = rdw(cpu, off), cmdsize = rdw(cpu, off + 4);
        if (cmdsize == 0) break;
        if (cmd == 0x19) {  /* LC_SEGMENT_64 */
            char nm[17] = {0};
            rd(cpu, off + 8, nm, 16);
            uint64_t vmaddr = rdq(cpu, off + 24);
            for (size_t s = 0; s < N_DSYM_SEGS; s++) {
                if (strncmp(nm, dsym_segs[s].name, 16) == 0)
                    kc_seg_rt[s] = vmaddr;
            }
        }
        off += cmdsize;
    }
    kc_header_rt    = kc;
    allproc_rt      = rebase(SYM_ALLPROC);
    cpu_data_ptr_rt = rebase(SYM_CPU_DATA_PTR);
    if (!allproc_rt) return false;
    resolved = true;
    printf("osi_mac: resolved KC @ 0x%llx, allproc @ 0x%llx, cpu_data_ptr @ 0x%llx\n",
           (unsigned long long)kc, (unsigned long long)allproc_rt,
           (unsigned long long)cpu_data_ptr_rt);
    return true;
}

/* Map a dSYM-static kernel symbol to its runtime address (per-segment). */
static uint64_t rebase(uint64_t static_addr) {
    for (size_t s = 0; s < N_DSYM_SEGS; s++) {
        if (static_addr >= dsym_segs[s].base &&
            static_addr <  dsym_segs[s].base + dsym_segs[s].size && kc_seg_rt[s])
            return static_addr - dsym_segs[s].base + kc_seg_rt[s];
    }
    return 0;
}

/* The proc of the process currently running on this CPU (cpu_data_ptr[0] ->
 * cpu_active_thread -> t_task -> bsd_info). Single-core (RR) -> cpu index 0. */
static uint64_t current_proc(CPUState *cpu) {
    if (!cpu_data_ptr_rt) return 0;
    uint64_t cd = rdq(cpu, cpu_data_ptr_rt);          /* cpu_data_ptr[0] */
    if (!is_kptr(cd)) return 0;
    uint64_t thr = rdq(cpu, cd + OFF_CD_ACTIVE_THREAD);
    if (!is_kptr(thr)) return 0;
    uint64_t task = rdq(cpu, thr + OFF_THREAD_T_TASK);
    if (!is_kptr(task)) return 0;
    uint64_t proc = rdq(cpu, task + OFF_TASK_BSD_INFO);
    return is_kptr(proc) ? proc : 0;
}

/* Extract a process name from the p_comm/p_name window: longest printable run. */
static char *read_proc_name(CPUState *cpu, uint64_t proc) {
    uint8_t buf[NAME_WIN_LEN];
    if (!rd(cpu, proc + NAME_WIN_OFF, buf, NAME_WIN_LEN))
        return g_strdup("");
    int best_start = 0, best_len = 0, cur_start = 0, cur_len = 0;
    for (int i = 0; i < NAME_WIN_LEN; i++) {
        if (buf[i] >= 0x20 && buf[i] < 0x7f) {
            if (cur_len == 0) cur_start = i;
            cur_len++;
            if (cur_len > best_len) { best_len = cur_len; best_start = cur_start; }
        } else {
            cur_len = 0;
        }
    }
    char out[NAME_WIN_LEN + 1];
    int n = best_len > NAME_WIN_LEN ? NAME_WIN_LEN : best_len;
    memcpy(out, buf + best_start, n);
    out[n] = 0;
    return g_strdup(out);
}

/* Walk allproc into an array of (proc-ptr). Caller iterates. */
static void fill_proc(CPUState *cpu, uint64_t proc, OsiProc *p) {
    memset(p, 0, sizeof(*p));
    p->taskd = proc;   /* handle == the BSD proc kernel address */
    p->pid   = (target_pid_t) rdw(cpu, proc + OFF_P_PID);
    p->ppid  = (target_pid_t) rdw(cpu, proc + OFF_P_PPID);
    p->name  = read_proc_name(cpu, proc);
    /* address space: proc -> task -> vm_map -> pmap -> pm_cr3 */
    uint64_t cr3 = 0, task = rdq(cpu, proc + OFF_P_TASK);
    if (is_kptr(task)) {
        uint64_t map = rdq(cpu, task + OFF_TASK_MAP);
        if (is_kptr(map)) {
            uint64_t pmap = rdq(cpu, map + OFF_VMMAP_PMAP);
            if (is_kptr(pmap)) cr3 = rdq(cpu, pmap + OFF_PMAP_CR3);
        }
    }
    p->asid = cr3;
    p->pgd  = cr3;
    p->create_time = 0;
}

/* ---- OSI provider callbacks ---- */
void on_get_processes(CPUState *cpu, GArray **out) {
    if (!resolve(cpu)) return;
    if (*out == NULL) {
        *out = g_array_new(false, false, sizeof(OsiProc));
        g_array_set_clear_func(*out, (GDestroyNotify)free_osiproc_contents);
    }
    uint64_t p = rdq(cpu, allproc_rt);   /* allproc.lh_first */
    int n = 0;
    while (is_kptr(p) && n < MAX_PROCS) {
        OsiProc proc;
        fill_proc(cpu, p, &proc);
        g_array_append_val(*out, proc);
        uint64_t next = rdq(cpu, p + OFF_P_LIST_NEXT);
        if (next == p) break;
        p = next;
        n++;
    }
}

void on_get_process_handles(CPUState *cpu, GArray **out) {
    if (!resolve(cpu)) return;
    if (*out == NULL) {
        *out = g_array_new(false, false, sizeof(OsiProcHandle));
        g_array_set_clear_func(*out, (GDestroyNotify)free_osiprochandle_contents);
    }
    uint64_t p = rdq(cpu, allproc_rt);
    int n = 0;
    while (is_kptr(p) && n < MAX_PROCS) {
        OsiProcHandle h;
        memset(&h, 0, sizeof(h));
        h.taskd = p;   /* the BSD proc kernel address */
        h.asid = 0;
        g_array_append_val(*out, h);
        uint64_t next = rdq(cpu, p + OFF_P_LIST_NEXT);
        if (next == p) break;
        p = next;
        n++;
    }
}

void on_get_current_process(CPUState *cpu, OsiProc **out) {
    if (!resolve(cpu)) return;
    uint64_t proc = current_proc(cpu);
    if (!proc) return;
    *out = (OsiProc *)g_malloc0(sizeof(OsiProc));
    fill_proc(cpu, proc, *out);
}

void on_get_current_process_handle(CPUState *cpu, OsiProcHandle **out) {
    if (!resolve(cpu)) return;
    uint64_t proc = current_proc(cpu);
    if (!proc) return;
    *out = (OsiProcHandle *)g_malloc0(sizeof(OsiProcHandle));
    (*out)->taskd = proc;
    (*out)->asid = 0;
}

void on_get_process(CPUState *cpu, const OsiProcHandle *h, OsiProc **out) {
    if (!resolve(cpu) || h == NULL || !is_kptr(h->taskd)) return;
    *out = (OsiProc *)g_malloc0(sizeof(OsiProc));
    fill_proc(cpu, h->taskd, *out);
}

void on_get_process_pid(CPUState *cpu, const OsiProcHandle *h, target_pid_t *pid) {
    if (!resolve(cpu) || h == NULL || !is_kptr(h->taskd)) { *pid = (target_pid_t)-1; return; }
    *pid = (target_pid_t) rdw(cpu, h->taskd + OFF_P_PID);
}

void on_get_process_ppid(CPUState *cpu, const OsiProcHandle *h, target_pid_t *ppid) {
    if (!resolve(cpu) || h == NULL || !is_kptr(h->taskd)) { *ppid = (target_pid_t)-1; return; }
    *ppid = (target_pid_t) rdw(cpu, h->taskd + OFF_P_PPID);
}

void on_get_current_thread(CPUState *cpu, OsiThread **out) {
    if (!resolve(cpu)) return;
    uint64_t cd = rdq(cpu, cpu_data_ptr_rt);
    if (!is_kptr(cd)) return;
    uint64_t thr = rdq(cpu, cd + OFF_CD_ACTIVE_THREAD);
    if (!is_kptr(thr)) return;
    uint64_t task = rdq(cpu, thr + OFF_THREAD_T_TASK);
    uint64_t proc = is_kptr(task) ? rdq(cpu, task + OFF_TASK_BSD_INFO) : 0;
    *out = (OsiThread *)g_malloc0(sizeof(OsiThread));
    (*out)->tid = (target_pid_t) rdq(cpu, thr + OFF_THREAD_ID);
    (*out)->pid = is_kptr(proc) ? (target_pid_t) rdw(cpu, proc + OFF_P_PID) : (target_pid_t)-1;
}

/* Kernel extensions. On Monterey the kexts are prelinked into the boot
 * kernelcache, so the legacy kmod_info list (`kmod`) is empty; instead each
 * kext is an LC_FILESET_ENTRY in the KC Mach-O header (bundle-id + load addr).
 */
#define LC_FILESET_ENTRY 0x35   /* matched modulo LC_REQ_DYLD */
void on_get_modules(CPUState *cpu, GArray **out) {
    if (!resolve(cpu) || !kc_header_rt) return;
    if (*out == NULL) {
        *out = g_array_new(false, false, sizeof(OsiModule));
        g_array_set_clear_func(*out, (GDestroyNotify)free_osimodule_contents);
    }
    uint32_t ncmds = rdw(cpu, kc_header_rt + 16);
    uint64_t off = kc_header_rt + 32;
    for (uint32_t i = 0; i < ncmds && i < 8192; i++) {
        uint32_t cmd = rdw(cpu, off), cmdsize = rdw(cpu, off + 4);
        if (cmdsize == 0) break;
        if ((cmd & 0x7fffffff) == LC_FILESET_ENTRY) {
            OsiModule m;
            memset(&m, 0, sizeof(m));
            uint64_t vmaddr = rdq(cpu, off + 8);
            uint32_t str_off = rdw(cpu, off + 24);   /* entry_id (lc_str) */
            char nm[160] = {0};
            if (str_off < cmdsize)
                rd(cpu, off + str_off, nm, sizeof(nm) - 1);
            m.modd = off;
            m.base = vmaddr;
            m.size = 0;   /* not directly in the fileset entry */
            m.name = g_strdup(nm);
            m.file = g_strdup(nm);
            g_array_append_val(*out, m);
        }
        off += cmdsize;
    }
}

bool init_plugin(void *self) {
    panda_require("osi");
    PPP_REG_CB("osi", on_get_processes, on_get_processes);
    PPP_REG_CB("osi", on_get_process_handles, on_get_process_handles);
    PPP_REG_CB("osi", on_get_current_process, on_get_current_process);
    PPP_REG_CB("osi", on_get_current_process_handle, on_get_current_process_handle);
    PPP_REG_CB("osi", on_get_process, on_get_process);
    PPP_REG_CB("osi", on_get_process_pid, on_get_process_pid);
    PPP_REG_CB("osi", on_get_process_ppid, on_get_process_ppid);
    PPP_REG_CB("osi", on_get_current_thread, on_get_current_thread);
    PPP_REG_CB("osi", on_get_modules, on_get_modules);
    printf("osi_mac: initialized (XNU/Monterey provider)\n");
    return true;
}

void uninit_plugin(void *self) { }
