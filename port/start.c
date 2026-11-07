/* start.c: the program's start in the port (playbook 80, M5, "The program's start").

   In the original, DOS loads EOB.EXE at 1000:0000 and the Borland start-up code (C0L), the library's
   initialisers and the overlay manager (src/E1E8B.ASM) run before main. The port has none of their code
   (the library is library, AGENTS.md; the overlays are plain code), so this writes what they leave
   that the game or the port's runtime reads, and makes their DOS calls, as the oracle shows them at
   main's entry (EOB.EXE, `3 4 0 homie 3`, 2026-11-07): every service call before main, with its
   registers, and the memory that differs from the loaded image.

   - C0: DOS 30h (the version, ds:007B); the vectors 0, 4, 5 and 6 read (35h) and INT 0 set to C0's
     divide handler (1000:012D, 25h); _psp (ds:0079); the program's block shrunk to the end of the
     stack segment (4Ah, BX 323Eh: 0FF0h + 323Eh = 422Eh); BIOS 1Ah 00h (clock()'s start); the heap
     from 422E:0000 (ds:0085 its base, ds:0087 the break level, ds:008B its top, A000:0000).
   - The overlay manager: DOS 30h; the program file opened, its header and FBOV records read, closed,
     opened again and kept (handle 5), its first overlay read into the buffer; the INT 3Fh vector read
     and set to its handler (2E8B:04D0); its buffer from the far heap (farmalloc 2CC0h: _ovrbuffer,
     02CBh paragraphs, src/E07C8.C, and a paragraph). The port keeps handle 5 taken (dos_reserve_handle)
     and does not read the file: its accesses are counted as the oracle counts them, the library's
     (tools/oracle/cclock.py), and what they read is masked in the comparison (tools/port/state.py).
   - The library: IOCTL 4400h on handles 0 and 1 (stdio's start); a 200h-byte block from the far heap
     (a stream's buffer, 0000:5727) and the environment array (0000:7A5A, 10h bytes: a far pointer to
     each string of the environment, then a null one).
   - main's arguments, as C0 builds them at the top of the stack segment: the program's path from the
     environment (argv[0]) and the words of the command tail.

   The library's own data (stdio's streams, the saved vectors, argc and argv kept in DGROUP) is not
   written: no function of the port reads it, and the comparison masks it. */
#include <string.h>

#include "tcport.h"
#include "tclib.h"
#include "machine.h"
#include "ported.h"
#include "start.h"

/* the layout of EOB.EXE (build/re/eob.json) */
#define EOB_DGROUP_PARA 0x212Eu
#define STACK_PARA 0x312Du         /* the stack segment */
#define HEAP_SEG 0x422Eu           /* the heap's first block: the end of the stack segment */

/* C0's and the heap's words in DGROUP */
#define C0_PSP 0x0079u
#define C0_VERSION 0x007Bu
#define C0_HEAPBASE 0x0085u
#define C0_BRKLVL 0x0087u
#define C0_HEAPTOP 0x008Bu

#define DOS_LIB(r) machine_library_int(0x21, (r))

static void regs0(tc_regs *r) { memset(r, 0, sizeof *r); }

/* a DOS call of the start-up's that the port does not make: its place among the library's accesses */
static void counted_only(unsigned n)
{
    while (n--) {
        machine_library_access();
    }
}

static farptr ds_at(u16 off) { return TC_MKFP(tc_ds, off); }

/* What the interrupt handlers write, marked before their first write: a loop waiting on one of these
   bytes waits for an interrupt (tc_spin, tcport.h) even before any handler has run. A handler's
   stores mark the bytes they write anyway; these are the ones a wait can read first.
     - the ROM's INT 8 (port/runtime/machine.c): the BIOS tick count and its midnight flag;
     - TickIsr, INT 1Ch (port/asm/E1D8F.c): the counters ds:8488 to 849D (whitepaper 3.6.2);
     - KeyboardIsr, INT 9 (port/asm/E1AAD.c): the event ring and its indexes, the modifier and shift
       state, the held-key bits and flags, the key mode and scancode words, the mouse buttons, and
       gSoundOn and gTunesOn, which Alt-S and Alt-M toggle (whitepaper 3.6.3).
   The mouse callback and the sound drivers' timers are not here yet (M6). */
static void hot_seed(void)
{
    const u16 ds = (u16)(MACHINE_LOAD_SEG + EOB_DGROUP_PARA), kb = (u16)(MACHINE_LOAD_SEG + 0x1AADu);
    tc_hot_range(TC_MKFP(0x0040u, 0x006Cu), 5);
    tc_hot_range(TC_MKFP(ds, 0x8488u), 0x16);
    tc_hot_range(TC_MKFP(kb, 0x007Cu), 0x010Au);
    tc_hot_range(TC_MKFP(kb, 0x01FFu), 0x0013u);
    tc_hot_range(TC_MKFP(kb, 0x0222u), 1);
    tc_hot_range(TC_MKFP(kb, 0x09F2u), 3);
    tc_hot_range(TC_MKFP(kb, 0x0C16u), 2);
    tc_hot_range(TC_MKFP(ds, 0x80E6u), 4);
}

s16 tc_start(farptr *argv_out)
{
    tc_regs r;
    u16 psp = MACHINE_PSP_SEG, env, i, n, sp;
    farptr environ_block;
    static const u8 vecs[4] = {0, 4, 5, 6};
    char words[16][64];
    unsigned nwords = 0;

    hot_seed();
    tc_load_seg = MACHINE_LOAD_SEG;
    tc_ds = (u16)(MACHINE_LOAD_SEG + EOB_DGROUP_PARA);
    tc2c_init();
    tc2c_data();
    tc_library_data();

    /* ---- C0 */
    regs0(&r);
    r.ax = 0x3000u;
    DOS_LIB(&r);
    tc_st_u16(ds_at(C0_VERSION), r.ax);
    for (i = 0; i < 4; i++) {
        regs0(&r);
        r.ax = (u16)(0x3500u | vecs[i]);
        DOS_LIB(&r);
    }
    regs0(&r);
    r.ax = 0x2500u;
    r.ds = tc_load_seg;
    r.dx = 0x012Du;
    DOS_LIB(&r);
    tc_st_u16(ds_at(C0_PSP), psp);
    regs0(&r);
    r.ax = 0x4A00u;
    r.bx = (u16)(HEAP_SEG - psp);
    r.es = psp;
    DOS_LIB(&r);
    regs0(&r);
    machine_library_int(0x1A, &r);
    tc_st_u16(ds_at(C0_HEAPBASE), HEAP_SEG);
    tc_st_u32(ds_at(C0_BRKLVL), TC_MKFP(HEAP_SEG, 0));
    tc_st_u32(ds_at(C0_HEAPTOP), TC_MKFP(MACHINE_MEM_TOP, 0));

    /* ---- the overlay manager: its buffer first (the heap's first block), then its DOS calls */
    tcl_farmalloc(0x2CC0u);
    regs0(&r);
    r.ax = 0x3000u;
    DOS_LIB(&r);
    if (dos_reserve_handle("EOB.EXE") != 5) {
        tc_fail(70, "the overlay manager's handle is not 5");
    }
    counted_only(5);                /* open, read, seek, read, close */
    regs0(&r);
    r.ax = 0x353Fu;
    DOS_LIB(&r);
    regs0(&r);
    r.ax = 0x253Fu;
    r.ds = (u16)(tc_load_seg + 0x1E8Bu);
    r.dx = 0x04D0u;
    DOS_LIB(&r);
    counted_only(3);                /* open again (handle 5), seek, read: its first overlay */

    /* ---- the library: stdio, then the environment array */
    for (i = 0; i < 2; i++) {
        regs0(&r);
        r.ax = 0x4400u;
        r.bx = i;
        DOS_LIB(&r);
    }
    tcl_farmalloc(0x200u);
    environ_block = tcl_farmalloc(0x10u);
    env = tc_ld_u16(TC_MKFP(psp, 0x2Cu));
    {
        u16 off = 0, k = 0;
        while (tc_ld_u8(TC_MKFP(env, off)) != 0) {
            tc_st_u32(tc_fp_add(environ_block, (u16)(4u * k++)), TC_MKFP(env, off));
            while (tc_ld_u8(TC_MKFP(env, off)) != 0) {
                off++;
            }
            off++;
        }
        tc_st_u32(tc_fp_add(environ_block, (u16)(4u * k)), 0);
        /* the library keeps the array's pointer at ds:88A2 and 20h at ds:88A0 (0000:7A5E and 0000:7938,
           at the start-up, observed in the oracle; what the 20h counts is not established) */
        tc_st_u16(ds_at(0x88A0u), 0x0020u);
        tc_st_u32(ds_at(0x88A2u), environ_block);

        /* ---- argv: the program's path after the environment's end (a count word, then the path) */
        off = (u16)(off + 3u);
        n = 0;
        while ((words[0][n] = (char)tc_ld_u8(TC_MKFP(env, off++))) != 0 && n < sizeof words[0] - 1) {
            n++;
        }
        words[0][n] = 0;
        nwords = 1;
    }
    {
        u16 len = tc_ld_u8(TC_MKFP(psp, 0x80u));
        u16 off = 0x81u, end = (u16)(0x81u + len);
        while (off < end && nwords < 16) {
            while (off < end && tc_ld_u8(TC_MKFP(psp, off)) == ' ') {
                off++;
            }
            if (off >= end) {
                break;
            }
            n = 0;
            while (off < end && tc_ld_u8(TC_MKFP(psp, off)) != ' ' && n < sizeof words[0] - 1) {
                words[nwords][n++] = (char)tc_ld_u8(TC_MKFP(psp, off++));
            }
            words[nwords][n] = 0;
            nwords++;
        }
    }
    /* the strings at the stack segment's top, the array below them, the frames below that */
    tc_ss = (u16)(tc_load_seg + STACK_PARA);
    {
        u16 at = 0x0FF0u;
        farptr strs[16];
        for (i = 0; i < nwords; i++) {
            u16 len = (u16)strlen(words[i]);
            strs[i] = TC_MKFP(tc_ss, at);
            tc_put(strs[i], (const u8 *)words[i], (u16)(len + 1u));
            at = (u16)(at + len + 1u);
        }
        sp = (u16)(0x0FF0u - 4u * (nwords + 1u));
        for (i = 0; i < nwords; i++) {
            tc_st_u32(TC_MKFP(tc_ss, (u16)(sp + 4u * i)), strs[i]);
        }
        tc_st_u32(TC_MKFP(tc_ss, (u16)(sp + 4u * nwords)), 0);
        *argv_out = TC_MKFP(tc_ss, sp);
        tc_sp = (u16)(sp & ~1u);
    }
    return (s16)nwords;
}
