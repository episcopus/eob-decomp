/* dos.c: the machine's DOS (machine.h): INT 21h and its neighbours, the PSP and the environment,
   the memory arena and the game's files, as tools/oracle/dos.py and vfs.py have them.

   DOS 3.30 is reported. The files: the game directory is C:\EOB (the current directory); a file's
   bytes come from the host (platform_file_size, platform_file_read) the first time it is opened, and
   every write lands in a layer kept here, as the oracle's VFS does (copy on write: the host's file is
   never changed). The memory arena is a list of blocks, as the oracle keeps it; like the oracle, the
   machine writes no MCBs into the arena. A function the oracle does not have either stops; one the
   oracle has and the game has not been seen to call is left for when it is (machine_unimplemented,
   which the shadow test would report). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcport.h"
#include "machine.h"
#include "driver.h"
#include "platform.h"

#define MAX_HANDLES 20

#define PATH_MAX_ 160
#define FILE_DATE (((1988 - 1980) << 9) | (11 << 5) | 1)
#define FILE_TIME (12 << 11)

/* ---- the file system (vfs.py) ----------------------------------------------------------------- */

typedef struct {
    u8 *data;
    u32 len, cap;
} vbuf;

typedef struct {
    char full[PATH_MAX_];
    vbuf *layer;            /* created or opened for writing this run */
    vbuf *cache;            /* the host's bytes, read once */
    int deleted;
} ventry;

typedef struct {
    int device;
    char path[PATH_MAX_];
    vbuf *data;
    s64 pos;
    int writable;
    int refs;
} dosfile;

static ventry *entries;
static unsigned n_entries, cap_entries;
static const char *dirs[] = {"\\", "\\EOB", "\\EOB\\SAVE"};
static const char *cwd = "\\EOB";

static vbuf *vbuf_new(u32 len)
{
    vbuf *b = calloc(1, sizeof *b);
    if (b == NULL || (len && (b->data = malloc(len)) == NULL)) {
        tc_fail(70, "dos: out of host memory");
    }
    b->len = b->cap = len;
    return b;
}

static void vbuf_resize(vbuf *b, u32 len)
{
    if (len > b->cap) {
        u32 cap = b->cap ? b->cap : 256u;
        while (cap < len) {
            cap *= 2u;
        }
        b->data = realloc(b->data, cap);
        if (b->data == NULL) {
            tc_fail(70, "dos: out of host memory");
        }
        memset(b->data + b->cap, 0, cap - b->cap);
        b->cap = cap;
    }
    if (len > b->len) {
        memset(b->data + b->len, 0, len - b->len);
    }
    b->len = len;
}

static ventry *entry(const char *full)
{
    unsigned i;
    for (i = 0; i < n_entries; i++) {
        if (strcmp(entries[i].full, full) == 0) {
            return &entries[i];
        }
    }
    if (n_entries == cap_entries) {
        cap_entries = cap_entries ? cap_entries * 2u : 64u;
        entries = realloc(entries, cap_entries * sizeof *entries);
        if (entries == NULL) {
            tc_fail(70, "dos: out of host memory");
        }
    }
    memset(&entries[n_entries], 0, sizeof entries[n_entries]);
    strncpy(entries[n_entries].full, full, PATH_MAX_ - 1);
    return &entries[n_entries++];
}

/* the host's name of a file of the game directory, or null */
static const char *host_name(const char *full)
{
    if (strncmp(full, "\\EOB\\", 5) == 0 && strchr(full + 5, '\\') == NULL) {
        return full + 5;
    }
    return NULL;
}

static int is_dir(const char *full)
{
    unsigned i;
    for (i = 0; i < sizeof dirs / sizeof dirs[0]; i++) {
        if (strcmp(dirs[i], full) == 0) {
            return 1;
        }
    }
    return 0;
}

static int exists(const char *full)
{
    ventry *e = entry(full);
    const char *h;
    if (e->deleted) {
        return 0;
    }
    if (e->layer != NULL) {
        return 1;
    }
    h = host_name(full);
    return h != NULL && platform_file_size(h) >= 0;
}

/* resolve: upper case, backslashes, no drive, relative to the current directory, . and .. */
static void resolve(const char *path, char *out)
{
    char p[PATH_MAX_], tmp[2 * PATH_MAX_];
    char *parts[64];
    unsigned n = 0, i;
    size_t k;
    char *tok;

    strncpy(p, path, PATH_MAX_ - 1);
    p[PATH_MAX_ - 1] = 0;
    for (k = 0; p[k]; k++) {
        if (p[k] == '/') {
            p[k] = '\\';
        } else if (p[k] >= 'a' && p[k] <= 'z') {
            p[k] = (char)(p[k] - 32);
        }
    }
    tok = p;
    if (strlen(p) >= 2 && p[1] == ':') {
        tok = p + 2;
    }
    if (tok[0] != '\\') {
        if (tok[0]) {
            snprintf(tmp, sizeof tmp, "%s\\%s", cwd, tok);
        } else {
            snprintf(tmp, sizeof tmp, "%s", cwd);
        }
    } else {
        snprintf(tmp, sizeof tmp, "%s", tok);
    }
    for (tok = strtok(tmp, "\\"); tok != NULL && n < 64; tok = strtok(NULL, "\\")) {
        if (strcmp(tok, ".") == 0) {
            continue;
        }
        if (strcmp(tok, "..") == 0) {
            if (n) {
                n--;
            }
            continue;
        }
        parts[n++] = tok;
    }
    out[0] = 0;
    for (i = 0; i < n; i++) {
        strncat(out, "\\", PATH_MAX_ - strlen(out) - 1);
        strncat(out, parts[i], PATH_MAX_ - strlen(out) - 1);
    }
    if (n == 0) {
        strcpy(out, "\\");
    }
}

static vbuf *host_bytes(const char *full)
{
    const char *h = host_name(full);
    s32 size = h ? platform_file_size(h) : -1;
    vbuf *b;
    if (size < 0) {
        tc_fail(70, "dos: %s vanished from the host", full);
    }
    b = vbuf_new((u32)size);
    if (size && platform_file_read(h, b->data, (u32)size) != 0) {
        tc_fail(70, "dos: cannot read %s from the host", full);
    }
    return b;
}

/* VFS.open: null when the file is not there */
static dosfile *vfs_open(const char *path, int write, int create)
{
    char full[PATH_MAX_];
    ventry *e;
    dosfile *f;
    vbuf *data;

    resolve(path, full);
    e = entry(full);
    if (create) {
        e->layer = vbuf_new(0);
        e->deleted = 0;
        data = e->layer;
        write = 1;
    } else {
        if (!exists(full)) {
            return NULL;
        }
        if (e->layer != NULL) {
            data = e->layer;
        } else if (write) {                     /* copy on write: the host's file stays as it is */
            data = e->layer = host_bytes(full);
        } else {
            if (e->cache == NULL) {
                e->cache = host_bytes(full);
            }
            data = e->cache;
        }
    }
    f = calloc(1, sizeof *f);
    if (f == NULL) {
        tc_fail(70, "dos: out of host memory");
    }
    strcpy(f->path, full);
    f->data = data;
    f->writable = write;
    return f;
}

static int vfs_delete(const char *path)
{
    char full[PATH_MAX_];
    ventry *e;
    resolve(path, full);
    if (!exists(full)) {
        return 0;
    }
    e = entry(full);
    e->layer = NULL;
    e->deleted = 1;
    return 1;
}

/* ---- DOS state (dos.py Dos) ----------------------------------------------------------------- */

static dosfile devices[5];
static dosfile *handles[MAX_HANDLES];
static u16 dta_seg, dta_off, cur_psp, strategy, last_error;
static u8 break_flag, verify;
static struct {
    u16 start, paras, owner;
} arena[64];
static unsigned n_arena;
static u16 arena_top;

static void release(dosfile *f)
{
    unsigned h;
    if (f == NULL || f->device) {
        return;
    }
    for (h = 0; h < MAX_HANDLES; h++) {
        if (handles[h] == f) {
            return;
        }
    }
    free(f);                                    /* the buffer stays: the VFS owns it */
}

static int alloc_handle(dosfile *f)
{
    int h;
    for (h = 5; h < MAX_HANDLES; h++) {
        if (handles[h] == NULL) {
            handles[h] = f;
            return h;
        }
    }
    return -1;
}

/* A handle held as the overlay manager holds the program file (port/start.c): taken, read-only and
   empty, so the game's own opens get the handles they got in the original; the file is not read. */
int dos_reserve_handle(const char *path)
{
    dosfile *f = calloc(1, sizeof *f);
    if (f == NULL) {
        tc_fail(70, "dos: out of host memory");
    }
    resolve(path, f->path);
    f->data = vbuf_new(0);
    return alloc_handle(f);
}

static void path_of(u16 seg, u16 off, char *out)
{
    u16 i;
    for (i = 0; i < 128; i++) {
        u8 c = tc_ld_u8(TC_MKFP(seg, (u16)(off + i)));
        if (c == 0) {
            break;
        }
        out[i] = (char)c;
    }
    out[i] = 0;
}

/* Machine.read and Machine.write: n bytes from the linear address of seg:off on, not wrapping in
   the segment */
static void mem_write(u16 seg, u16 off, const u8 *b, u32 n)
{
    u32 a = TC_LINEAR(TC_MKFP(seg, off)), i;
    for (i = 0; i < n; i++) {
        tc_mem[(a + i) & 0xFFFFFu] = b[i];
    }
}

static void mem_read(u16 seg, u16 off, u8 *b, u32 n)
{
    u32 a = TC_LINEAR(TC_MKFP(seg, off)), i;
    for (i = 0; i < n; i++) {
        b[i] = tc_mem[(a + i) & 0xFFFFFu];
    }
}

static void out_char(u8 c, const tc_regs *r)
{
    (void)c;
    machine_unimplemented("console output (the oracle writes it to the text screen; the machine has none yet)", r);
}

static void make_psp(const u8 *cmdline, u16 cmdlen, const char *exe_name)
{
    u8 psp[256];
    char env[160];
    u16 i, n = cmdlen > 126 ? 126 : cmdlen, len;
    static const char env_head[] = "COMSPEC=C:\\COMMAND.COM\0PATH=C:\\\0\0\1\0C:\\EOB\\";

    memset(psp, 0, sizeof psp);
    psp[0] = 0xCD;
    psp[1] = 0x20;
    tc_va16(psp + 2, MACHINE_MEM_TOP);
    tc_va16(psp + 0x16, MACHINE_PSP_SEG);       /* parent: itself */
    for (i = 0; i < MAX_HANDLES; i++) {
        psp[0x18 + i] = (u8)(i < 5 ? i : 0xFF);
    }
    tc_va16(psp + 0x2C, MACHINE_ENV_SEG);
    tc_va16(psp + 0x32, MAX_HANDLES);
    tc_va16(psp + 0x34, 0x18);
    tc_va16(psp + 0x36, MACHINE_PSP_SEG);
    psp[0x50] = 0xCD;
    psp[0x51] = 0x21;
    psp[0x52] = 0xCB;
    psp[0x80] = (u8)n;
    memcpy(psp + 0x81, cmdline, n);
    psp[0x81 + n] = 0x0D;
    mem_write(MACHINE_PSP_SEG, 0, psp, sizeof psp);
    /* DOS 3+ puts the program's path after the environment; the overlay manager opens it */
    len = (u16)(sizeof env_head - 1);
    memcpy(env, env_head, len);
    for (i = 0; exe_name[i] && len < sizeof env - 1; i++) {
        char c = exe_name[i];
        env[len++] = (char)(c >= 'a' && c <= 'z' ? c - 32 : c);
    }
    env[len++] = 0;
    mem_write(MACHINE_ENV_SEG, 0, (const u8 *)env, len);
}

void dos_reset(const u8 *cmdline, u16 cmdlen, const char *exe_name)
{
    unsigned i;
    static const char *names[5] = {"CON", "CON", "CON", "AUX", "PRN"};
    for (i = 0; i < MAX_HANDLES; i++) {
        if (i >= 5 && handles[i] != NULL) {
            dosfile *f = handles[i];
            handles[i] = NULL;
            release(f);
        }
        handles[i] = NULL;
    }
    for (i = 0; i < 5; i++) {
        memset(&devices[i], 0, sizeof devices[i]);
        devices[i].device = 1;
        devices[i].writable = 1;
        strcpy(devices[i].path, names[i]);
        handles[i] = &devices[i];
    }
    dta_seg = MACHINE_PSP_SEG;
    dta_off = 0x80;
    cur_psp = MACHINE_PSP_SEG;
    break_flag = verify = 0;
    strategy = last_error = 0;
    n_entries = 0;
    make_psp(cmdline, cmdlen, exe_name);
    arena_top = MACHINE_MEM_TOP;               /* arena_init(PSP_SEG - 1, MEM_TOP_SEG) */
    n_arena = 2;
    arena[0].start = MACHINE_ENV_SEG;
    arena[0].paras = 0x40;
    arena[0].owner = MACHINE_PSP_SEG;
    arena[1].start = MACHINE_PSP_SEG;
    arena[1].paras = (u16)(MACHINE_MEM_TOP - MACHINE_PSP_SEG);
    arena[1].owner = MACHINE_PSP_SEG;
}

static void terminate(int code)
{
    machine_exit_code = code;
    tc_fail(76, "the program ended (exit code %d)", code);
}

/* ---- INT 20h, 29h, 2Fh ---------------------------------------------------------------------- */

void dos_int20(tc_regs *r)
{
    (void)r;
    terminate(0);
}

void dos_int29(tc_regs *r) { out_char((u8)r->ax, r); }

void dos_int2f(tc_regs *r)
{
    u16 ax = r->ax;
    u8 ah = (u8)(ax >> 8);
    if (ax == 0x1600 || ax == 0x4300 || ax == 0x4A01 || ax == 0x4A02) {   /* Windows, XMS, HMA: absent */
        r->ax = ax != 0x4300 ? (u16)(ax & 0xFF00u) : 0;
        return;
    }
    if (ah == 0x11 || ah == 0x12 || ah == 0x14 || ah == 0x15 || ah == 0x16 || ah == 0x43 || ah == 0xB7
        || ah == 0xAD) {
        r->ax = (u16)(ax & 0xFF00u);
        return;
    }
    machine_unimplemented("INT 2Fh", r);
}

/* ---- INT 21h --------------------------------------------------------------------------------- */

static void f3F(tc_regs *r)
{
    dosfile *f = r->bx < MAX_HANDLES ? handles[r->bx] : NULL;
    u32 n = r->cx, avail;
    if (f == NULL) {
        machine_err(r, 6);
        return;
    }
    if (f->device) {
        machine_unimplemented("INT 21h AH=3Fh read from device handle", r);
    }
    avail = f->pos < (s64)f->data->len ? f->data->len - (u32)f->pos : 0;
    if (n > avail) {
        n = avail;
    }
    if (n) {
        mem_write(r->ds, r->dx, f->data->data + f->pos, n);
    }
    f->pos += n;
    r->ax = (u16)n;
    machine_ok(r);
}

static void f40(tc_regs *r)
{
    dosfile *f = r->bx < MAX_HANDLES ? handles[r->bx] : NULL;
    u16 n = r->cx;
    u8 *data;
    if (f == NULL) {
        machine_err(r, 6);
        return;
    }
    data = malloc(n ? n : 1u);
    mem_read(r->ds, r->dx, data, n);
    if (f->device) {
        u16 i;
        for (i = 0; i < n; i++) {
            out_char(data[i], r);
        }
    } else {
        if (!f->writable) {
            free(data);
            machine_err(r, 5);
            return;
        }
        if (n == 0) {                            /* a zero-length write truncates at the position */
            if (f->pos < (s64)f->data->len) {
                f->data->len = (u32)f->pos;
            }
        } else {
            s64 end = f->pos + n;
            if (end > (s64)f->data->len) {
                vbuf_resize(f->data, (u32)end);
            }
            memcpy(f->data->data + f->pos, data, n);
            f->pos = end;
        }
    }
    free(data);
    r->ax = n;
    machine_ok(r);
}

static void f42(tc_regs *r, u8 al)
{
    dosfile *f = r->bx < MAX_HANDLES ? handles[r->bx] : NULL;
    s64 off = (s32)(((u32)r->cx << 16) | r->dx), base;
    if (f == NULL || f->device) {
        machine_err(r, 6);
        return;
    }
    if (al == 0) {
        base = 0;
    } else if (al == 1) {
        base = f->pos;
    } else if (al == 2) {
        base = f->data->len;
    } else {
        machine_err(r, 1);
        return;
    }
    f->pos = base + off < 0 ? 0 : base + off;
    r->dx = (u16)((u64)f->pos >> 16);
    r->ax = (u16)f->pos;
    machine_ok(r);
}

static void f44(tc_regs *r, u8 al)
{
    dosfile *f = r->bx < MAX_HANDLES ? handles[r->bx] : NULL;
    switch (al) {
    case 0x00:                                  /* device information */
        if (f == NULL) {
            machine_err(r, 6);
            return;
        }
        r->dx = f->device ? 0x80D3u : 0x0002u;
        break;
    case 0x01:
        break;
    case 0x06:                                  /* input status */
        r->ax = (u16)((r->ax & 0xFF00u) | (f && (f->device || f->pos < (s64)f->data->len) ? 0xFFu : 0u));
        break;
    case 0x07:
        r->ax = (u16)((r->ax & 0xFF00u) | 0xFFu);
        break;
    case 0x08:                                  /* removable? 1: fixed */
        r->ax = 1;
        break;
    case 0x09:
        r->dx = 0;
        break;
    case 0x0E:
        r->ax &= 0xFF00u;
        break;
    default:
        machine_unimplemented("INT 21h AH=44h", r);
    }
    machine_ok(r);
}

static void f48(tc_regs *r)
{
    u16 want = r->bx, prev_end = 0x1000, best = 0;
    unsigned order[64], i, j, n = n_arena;
    struct {
        u16 start, size;
    } gaps[66];
    unsigned ng = 0;
    for (i = 0; i < n; i++) {
        order[i] = i;
    }
    for (i = 1; i < n; i++) {                   /* sorted by start, stable */
        for (j = i; j > 0 && arena[order[j - 1]].start > arena[order[j]].start; j--) {
            unsigned t = order[j];
            order[j] = order[j - 1];
            order[j - 1] = t;
        }
    }
    for (i = 0; i < n; i++) {
        u32 s = arena[order[i]].start, e = s + arena[order[i]].paras;
        if (s > prev_end) {
            gaps[ng].start = prev_end;
            gaps[ng++].size = (u16)(s - prev_end);
        }
        if (e > prev_end) {
            prev_end = (u16)e;
        }
    }
    if (arena_top > prev_end) {
        gaps[ng].start = prev_end;
        gaps[ng++].size = (u16)(arena_top - prev_end);
    }
    for (i = 0; i < ng; i++) {
        if (gaps[i].size >= want) {
            if (n_arena == 64) {
                tc_fail(70, "dos: the memory arena list is full");
            }
            arena[n_arena].start = gaps[i].start;
            arena[n_arena].paras = want;
            arena[n_arena++].owner = cur_psp;
            r->ax = gaps[i].start;
            machine_ok(r);
            return;
        }
        if (gaps[i].size > best) {
            best = gaps[i].size;
        }
    }
    r->bx = best;
    machine_err(r, 8);
}

static void f49(tc_regs *r)
{
    unsigned i;
    for (i = 0; i < n_arena; i++) {
        if (arena[i].start == r->es) {
            memmove(&arena[i], &arena[i + 1], (n_arena - i - 1) * sizeof arena[0]);
            n_arena--;
            machine_ok(r);
            return;
        }
    }
    machine_err(r, 9);
}

static void f4A(tc_regs *r)
{
    u16 seg = r->es, want = r->bx;
    unsigned i, j;
    for (i = 0; i < n_arena; i++) {
        if (arena[i].start == seg) {
            u16 limit = arena_top;
            for (j = 0; j < n_arena; j++) {
                if (j != i && arena[j].start > seg && arena[j].start < limit) {
                    limit = arena[j].start;
                }
            }
            if (want <= (u16)(limit - seg)) {
                arena[i].paras = want;
                machine_ok(r);
                return;
            }
            r->bx = (u16)(limit - seg);
            machine_err(r, 8);
            return;
        }
    }
    machine_err(r, 9);
}

static void f4B(tc_regs *r, u8 al)
{
    char path[PATH_MAX_];
    u8 blk[4];
    u16 seg, factor;
    dosfile *f;
    const u8 *d;
    u32 size, hdr, i;

    path_of(r->ds, r->dx, path);
    if (al == 0) {
        tc_fail(74, "INT 21h 4B00h: the program runs %s (the machine does not run programs)", path);
    }
    if (al != 3) {
        machine_unimplemented("INT 21h AH=4Bh EXEC", r);
    }
    /* load overlay: ES:BX -> (u16 segment to load at, u16 relocation factor); no PSP, no run */
    mem_read(r->es, r->bx, blk, 4);
    seg = tc_vr16(blk);
    factor = tc_vr16(blk + 2);
    f = vfs_open(path, 0, 0);
    if (f == NULL) {
        machine_err(r, 2);
        return;
    }
    d = f->data->data;
    size = f->data->len;
    if (size >= 0x1C && ((d[0] == 'M' && d[1] == 'Z') || (d[0] == 'Z' && d[1] == 'M'))) {
        u16 last = tc_vr16(d + 2), pages = tc_vr16(d + 4), nrel = tc_vr16(d + 6), hdrp = tc_vr16(d + 8);
        u16 reloff = tc_vr16(d + 0x18);
        u32 end = last ? (u32)(pages - 1u) * 512u + last : (u32)pages * 512u;
        u8 *image;
        hdr = (u32)hdrp * 16u;
        if (end > size) {
            end = size;
        }
        image = malloc(end > hdr ? end - hdr : 1u);
        memcpy(image, d + hdr, end > hdr ? end - hdr : 0);
        for (i = 0; i < nrel; i++) {
            u32 p = (u32)tc_vr16(d + reloff + 4u * i + 2u) * 16u + tc_vr16(d + reloff + 4u * i);
            tc_va16(image + p, (u16)(tc_vr16(image + p) + factor));
        }
        mem_write(seg, 0, image, end > hdr ? end - hdr : 0);
        free(image);
    } else {
        mem_write(seg, 0, d, size);
    }
    release(f);
    machine_ok(r);
    /* the graphics driver the port has in C (decision 4: MCGA only) answers at the segment it was
       loaded at; the game shrinks the block in place (MemRealloc), so the segment stays */
    {
        const char *base = strrchr(path, '\\');
        char name[13];
        unsigned k;
        base = base ? base + 1 : path;
        for (k = 0; k < sizeof name - 1 && base[k]; k++) {
            name[k] = (char)(base[k] >= 'a' && base[k] <= 'z' ? base[k] - 32 : base[k]);
        }
        name[k] = 0;
        if (strcmp(name, "MCGA.OVL") == 0) {
            mcga_register(seg);
        }
    }
}

void dos_int21(tc_regs *r)
{
    u8 ah = (u8)(r->ax >> 8), al = (u8)r->ax;
    char path[PATH_MAX_], full[PATH_MAX_];
    dosfile *f;
    int h;

    switch (ah) {
    case 0x06:                                  /* direct console output (DL), or input for DL = 0FFh */
        if ((u8)r->dx != 0xFFu) {
            out_char((u8)r->dx, r);
            return;
        }
        machine_unimplemented("INT 21h AH=06h input (the machine has no DOS keyboard input yet)", r);
        return;
    case 0x0E:                                  /* LASTDRIVE: 3 drives (A, B, C) */
        r->ax = (u16)((r->ax & 0xFF00u) | 3u);
        return;
    case 0x19:                                  /* current drive C: */
        r->ax = (u16)((r->ax & 0xFF00u) | 2u);
        return;
    case 0x1A:
        dta_seg = r->ds;
        dta_off = r->dx;
        return;
    case 0x2F:
        r->es = dta_seg;
        r->bx = dta_off;
        return;
    case 0x25:
        tc_st_u16(TC_MKFP(0, (u16)(al * 4u)), r->dx);
        tc_st_u16(TC_MKFP(0, (u16)(al * 4u + 2u)), r->ds);
        return;
    case 0x35:
        r->bx = tc_ld_u16(TC_MKFP(0, (u16)(al * 4u)));
        r->es = tc_ld_u16(TC_MKFP(0, (u16)(al * 4u + 2u)));
        return;
    case 0x30:                                  /* DOS 3.30 */
        r->ax = 0x1E03;
        r->bx = 0;
        r->cx = 0;
        return;
    case 0x33:
        if (al == 0) {
            r->dx = (u16)((r->dx & 0xFF00u) | break_flag);
        } else if (al == 1) {
            break_flag = (u8)r->dx;
        } else if (al == 5) {
            r->dx = (u16)((r->dx & 0xFF00u) | 3u);
        } else if (al == 6) {
            r->bx = 0x1E03;
            r->dx = 0;
        } else {
            machine_unimplemented("INT 21h AH=33h", r);
        }
        return;
    case 0x2A: {                                /* the date: 1991-06-11 and the days the clock has run */
        u32 days = (machine_now() / machine_time.insn_per_sec) / 86400u;
        r->cx = 1991;
        r->dx = (u16)((6u << 8) | (11u + days));
        r->ax = (u16)((r->ax & 0xFF00u) | ((2u + days) % 7u));
        return;
    }
    case 0x2C: {
        u32 t = machine_time_of_day();
        r->cx = (u16)((((t / 360000u) % 24u) << 8) | ((t / 6000u) % 60u));
        r->dx = (u16)((((t / 100u) % 60u) << 8) | (t % 100u));
        return;
    }
    case 0x2E:
    case 0x54:
        verify = al;
        return;
    case 0x34:                                  /* the InDOS flag's address */
        r->es = 0xF000;
        r->bx = 0x0010;
        return;
    case 0x3C:
    case 0x5B:                                  /* create or truncate */
        path_of(r->ds, r->dx, path);
        f = vfs_open(path, 1, 1);
        h = alloc_handle(f);
        if (h < 0) {
            release(f);
            machine_err(r, 4);
            return;
        }
        r->ax = (u16)h;
        machine_ok(r);
        return;
    case 0x3D:                                  /* open */
        path_of(r->ds, r->dx, path);
        f = vfs_open(path, (al & 7) != 0, 0);
        if (f == NULL) {
            char *slash;
            resolve(path, full);
            slash = strrchr(full, '\\');
            if (slash != NULL && slash != full) {
                *slash = 0;
                machine_err(r, is_dir(full) ? 2 : 3);
            } else {
                machine_err(r, 2);
            }
            return;
        }
        h = alloc_handle(f);
        if (h < 0) {
            release(f);
            machine_err(r, 4);
            return;
        }
        r->ax = (u16)h;
        machine_ok(r);
        return;
    case 0x3E:
        f = r->bx < MAX_HANDLES ? handles[r->bx] : NULL;
        if (f == NULL) {
            machine_err(r, 6);
            return;
        }
        if (r->bx >= 5) {
            handles[r->bx] = NULL;
            release(f);
        }
        machine_ok(r);
        return;
    case 0x3F:
        f3F(r);
        return;
    case 0x40:
        f40(r);
        return;
    case 0x41:
        path_of(r->ds, r->dx, path);
        if (vfs_delete(path)) {
            machine_ok(r);
        } else {
            machine_err(r, 2);
        }
        return;
    case 0x42:
        f42(r, al);
        return;
    case 0x43:
        path_of(r->ds, r->dx, path);
        resolve(path, full);
        if (!(exists(full) || is_dir(full))) {
            machine_err(r, 2);
            return;
        }
        if (al == 0) {
            r->cx = is_dir(full) ? 0x10u : 0x20u;
        }
        machine_ok(r);
        return;
    case 0x44:
        f44(r, al);
        return;
    case 0x45:
        f = r->bx < MAX_HANDLES ? handles[r->bx] : NULL;
        if (f == NULL) {
            machine_err(r, 6);
            return;
        }
        h = alloc_handle(f);
        if (h < 0) {
            machine_err(r, 4);
            return;
        }
        r->ax = (u16)h;
        machine_ok(r);
        return;
    case 0x46:
        f = r->bx < MAX_HANDLES ? handles[r->bx] : NULL;
        if (f == NULL) {
            machine_err(r, 6);
            return;
        }
        if (r->cx < MAX_HANDLES) {
            dosfile *old = handles[r->cx];
            handles[r->cx] = f;
            release(old);
        } else {
            machine_unimplemented("INT 21h AH=46h to a handle past the table", r);
        }
        machine_ok(r);
        return;
    case 0x48:
        f48(r);
        return;
    case 0x49:
        f49(r);
        return;
    case 0x4A:
        f4A(r);
        return;
    case 0x4B:
        f4B(r, al);
        return;
    case 0x4C:
        terminate(al);
        return;
    case 0x00:
        terminate(0);
        return;
    case 0x4D:
        r->ax = 0;
        return;
    case 0x50:
        cur_psp = r->bx;
        return;
    case 0x51:
    case 0x62:
        r->bx = cur_psp;
        return;
    case 0x57:
        if (al == 0) {
            r->cx = FILE_TIME;
            r->dx = FILE_DATE;
        }
        machine_ok(r);
        return;
    case 0x58:
        if (al == 0) {
            r->ax = strategy;
        } else {
            strategy = r->bx;
        }
        machine_ok(r);
        return;
    case 0x59:
        r->ax = last_error;
        r->bx = 0x0105;
        r->cx = 0x0100;
        return;
    case 0x67:
    case 0x68:
        machine_ok(r);
        return;
    default:
        machine_unimplemented("INT 21h (a function the game has not been seen to call)", r);
    }
}
