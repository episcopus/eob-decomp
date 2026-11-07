/* arena.c: the arena, the program loader and the code address table (tcport.h). */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcport.h"
#include "driver.h"

u8 tc_mem[TC_MEM_SIZE] = {0};
u16 tc_load_seg;
u16 tc_ds;
u16 tc_ss;
u16 tc_sp;
u32 tc_changes;
u32 tc_hot_reads;
u32 tc_accesses;
u8 tc_in_handler;
u8 tc_hot[TC_MEM_SIZE / 8u] = {0};
u8 tc_hot_watch;

/* a loop's idle turn outside a replay (the self-test, the per-function test): nothing to deliver */
__attribute__((weak)) void tc_spin_idle(void) {}
/* the replay's trace of reads of handler-written bytes (irq.c); nothing outside it */
__attribute__((weak)) void tc_hot_read(u32 a) { (void)a; }

void (*tc_fail_hook)(int code, const char *msg);

void tc_fail(int code, const char *fmt, ...)
{
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (tc_fail_hook != NULL) {
        tc_fail_hook(code, msg);
    }
    fprintf(stderr, "tcport: %s\n", msg);
    exit(code);
}

farptr tc_copy(farptr dst, farptr src, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++) {
        tc_st_u8(tc_fp_add(dst, i), tc_ld_u8(tc_fp_add(src, i)));
    }
    return dst;
}

void tc_put(farptr dst, const u8 *src, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++) {
        tc_st_u8(tc_fp_add(dst, i), src[i]);
    }
}

void tc_zero(farptr dst, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++) {
        tc_st_u8(tc_fp_add(dst, i), 0);
    }
}

tc_frame tc_frame_enter(u16 size)
{
    tc_frame f;
    u16 need = (u16)((size + 1u) & ~1u);
    if (tc_ss == 0 || tc_sp < need) {
        tc_fail(70, "the arena stack (%04X:%04X) has no room for a frame of %u bytes", tc_ss, tc_sp, size);
    }
    f.sp = tc_sp;
    tc_sp = (u16)(tc_sp - need);
    f.base = TC_MKFP(tc_ss, tc_sp);
    return f;
}

#define CODE_MAX 4096u

static struct {
    farptr addr;
    tc_fn fn;
    const char *name;
} code[CODE_MAX];
static unsigned code_count;

void tc_code_register(farptr addr, tc_fn fn, const char *name)
{
    unsigned i;
    for (i = 0; i < code_count; i++) {
        if (code[i].addr == addr) {
            code[i].fn = fn;
            code[i].name = name;
            return;
        }
    }
    if (code_count == CODE_MAX) {
        tc_fail(70, "code table full at %s", name);
    }
    code[code_count].addr = addr;
    code[code_count].fn = fn;
    code[code_count].name = name;
    code_count++;
}

tc_fn tc_code_find(farptr addr)
{
    unsigned i;
    for (i = 0; i < code_count; i++) {
        if (code[i].addr == addr) {
            return code[i].fn;
        }
    }
    return 0;
}

/* the routines entered with their argument bytes (driver.h) */
static struct {
    farptr addr;
    tc_argfn fn;
    const char *name;
} argcode[CODE_MAX];
static unsigned argcode_count;

void tc_code_register_args(farptr addr, tc_argfn fn, const char *name)
{
    unsigned i;
    for (i = 0; i < argcode_count; i++) {
        if (argcode[i].addr == addr) {
            argcode[i].fn = fn;
            argcode[i].name = name;
            return;
        }
    }
    if (argcode_count == CODE_MAX) {
        tc_fail(70, "code table full at %s", name);
    }
    argcode[argcode_count].addr = addr;
    argcode[argcode_count].fn = fn;
    argcode[argcode_count].name = name;
    argcode_count++;
}

tc_argfn tc_code_find_args(farptr addr)
{
    unsigned i;
    for (i = 0; i < argcode_count; i++) {
        if (argcode[i].addr == addr) {
            return argcode[i].fn;
        }
    }
    return 0;
}

int tc_code_call_args(farptr addr, const u8 *args, u16 n, tc_regs *r, u32 *ret)
{
    u8 b[TC_ARG_MAX];
    tc_argfn fn = tc_code_find_args(addr);
    if (fn == 0) {
        return 0;
    }
    if (n > TC_ARG_MAX) {
        tc_fail(70, "%u argument bytes for the routine at %04X:%04X", n, TC_SEG(addr), TC_OFF(addr));
    }
    memset(b, 0, sizeof b);
    if (n) {
        memcpy(b, args, n);
    }
    memset(r, 0, sizeof *r);
    *ret = fn(b, r);
    return 1;
}

#ifdef TC_HYBRID
#include "hybrid.h"
#endif

u32 tc_code_far(farptr addr, const u8 *args, u16 n, const char *site)
{
    tc_regs r;
    u32 v;
    if (tc_code_call_args(addr, args, n, &r, &v)) {
        return v;
    }
#ifdef TC_HYBRID
    (void)site;
    return tc_hybrid_far(addr, args, n);
#else
    (void)args;
    (void)n;
    tc_fail(70, "no C function at %04X:%04X (load-relative %04X:%04X), called from %s",
            TC_SEG(addr), TC_OFF(addr), (u16)(TC_SEG(addr) - tc_load_seg), TC_OFF(addr), site);
#endif
}

void tc_unported(const char *name, const char *addr)
{
    tc_fail(71, "%s (%s) is not ported yet", name, addr);
}

static u16 word_at(const u8 *b) { return (u16)(b[0] | (b[1] << 8)); }

int tc_load_program(const char *path, u16 load_seg, u16 ds_para)
{
    FILE *f;
    long size;
    u8 *file;
    u32 header, image, pages, last, nrel, relofs, i;

    f = fopen(path, "rb");
    if (f == NULL) {
        fprintf(stderr, "tcport: cannot open %s\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    file = malloc((size_t)size);
    if (file == NULL || fread(file, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "tcport: cannot read %s\n", path);
        fclose(f);
        free(file);
        return -1;
    }
    fclose(f);
    if (size < 0x1C || file[0] != 'M' || file[1] != 'Z') {
        fprintf(stderr, "tcport: %s is not an MZ program\n", path);
        free(file);
        return -1;
    }
    last = word_at(file + 2);
    pages = word_at(file + 4);
    nrel = word_at(file + 6);
    header = (u32)word_at(file + 8) * 16u;
    relofs = word_at(file + 0x18);
    /* The load module: pages of 512 bytes, the last one `last` bytes long (0: full), less the
       header. The FBOV overlay data after it stays in the file, as DOS leaves it. */
    image = pages * 512u - (last ? 512u - last : 0u) - header;
    if ((u32)load_seg * 16u + image > TC_MEM_SIZE || header + image > (u32)size) {
        fprintf(stderr, "tcport: %s does not fit\n", path);
        free(file);
        return -1;
    }
    memcpy(tc_mem + (u32)load_seg * 16u, file + header, image);
    for (i = 0; i < nrel; i++) {
        const u8 *r = file + relofs + i * 4u;
        farptr at = TC_MKFP((u16)(word_at(r + 2) + load_seg), word_at(r));
        tc_st_u16(at, (u16)(tc_ld_u16(at) + load_seg));
    }
    tc_load_seg = load_seg;
    tc_ds = (u16)(load_seg + ds_para);
    /* the stack the header names (EOB.EXE: 312D:0080 in the load module); its top is where Turbo
       C's start-up puts SP, _stklen bytes in [H: 1000h, the default; M3 reads the oracle's] */
    tc_ss = (u16)(word_at(file + 0x0E) + load_seg);
    tc_sp = 0x1000u;
    free(file);
    return 0;
}

void tc_hot_range(farptr p, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++) {
        u32 a = TC_LINEAR(tc_fp_add(p, i));
        tc_hot[a >> 3] |= (u8)(1u << (a & 7u));
    }
}
