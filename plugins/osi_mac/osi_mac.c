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
/* struct proc field offsets (validated against the live kernel) */
#define OFF_P_LIST_NEXT    0     /* proc.p_list.le_next */
#define OFF_P_TASK         16    /* proc.task           */
#define OFF_P_PPID         40    /* proc.p_ppid         */
#define OFF_P_PID          104   /* proc.p_pid          */
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
    /* rebase allproc (in dSYM __DATA) */
    for (size_t s = 0; s < N_DSYM_SEGS; s++) {
        if (SYM_ALLPROC >= dsym_segs[s].base &&
            SYM_ALLPROC <  dsym_segs[s].base + dsym_segs[s].size) {
            if (!kc_seg_rt[s]) return false;
            allproc_rt = SYM_ALLPROC - dsym_segs[s].base + kc_seg_rt[s];
            break;
        }
    }
    if (!allproc_rt) return false;
    resolved = true;
    printf("osi_mac: resolved KC @ 0x%llx, allproc @ 0x%llx\n",
           (unsigned long long)kc, (unsigned long long)allproc_rt);
    return true;
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
    p->taskd = rdq(cpu, proc + OFF_P_TASK);
    p->pid   = (target_pid_t) rdw(cpu, proc + OFF_P_PID);
    p->ppid  = (target_pid_t) rdw(cpu, proc + OFF_P_PPID);
    p->name  = read_proc_name(cpu, proc);
    p->asid  = 0;   /* TODO: proc->task->map->pmap->pm_cr3 */
    p->pgd   = 0;
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
        h.taskd = rdq(cpu, p + OFF_P_TASK);
        h.asid = 0;
        g_array_append_val(*out, h);
        uint64_t next = rdq(cpu, p + OFF_P_LIST_NEXT);
        if (next == p) break;
        p = next;
        n++;
    }
}

bool init_plugin(void *self) {
    panda_require("osi");
    PPP_REG_CB("osi", on_get_processes, on_get_processes);
    PPP_REG_CB("osi", on_get_process_handles, on_get_process_handles);
    printf("osi_mac: initialized (XNU/Monterey provider)\n");
    return true;
}

void uninit_plugin(void *self) { }
