/* E1C00.c: src/E1C00.ASM in C (playbook 80, M3): the game's loader for the graphics driver overlays
   (CGA.OVL, TGA.OVL, EGA.OVL, MCGA.OVL), which it loads with DOS function 4B03h (load overlay) into a
   0FA00h byte block from MemAlloc, then calls the driver's entry at segment:0000, shrinks the block
   to the size the entry returns (MemRealloc) and calls the routine the entry points at (the driver's
   link routine) with the request's table.

   Written from the TASM source, which stays the reference. The segments are the source's: the
   request's segment (its far pointer's) where the source uses DS or ES as RunOverlayRequest set them,
   DGROUP (`mov ax,212Eh`, relocated at load) where it loads it. LoadOverlayImage (1C00:0002) is near
   (`push cs; call`, register arguments) and only RunOverlayRequest calls it: a static function.

   The driver's code returns in registers, which a C prototype cannot state: its entry gives AX (the
   link routine's offset) and BX (its size in paragraphs), its link routine gives CF. The driver in C
   is M4's (decision 4); until then a call to it goes where the port sends code it has no C for
   (tc_code_far: the hybrid build runs the original's code on the bench), and driver_call reads the
   registers that code returned (tc_hybrid_far_regs). */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define DGROUP ((u16)(tc_load_seg + 0x212Eu))           /* the value `mov ax,212Eh` gives */
#define WAIT_SEG ((u16)(tc_load_seg + 0x1A16u))         /* `mov bx,1A16h` at 1C00:0089, relocated at load */
#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (off)))
#define ST16(seg, off, v) tc_st_u16(TC_MKFP((seg), (off)), (u16)(v))

/* the words of the request's segment and of DGROUP the loader uses (the same segment at run time) */
#define LOAD_BLOCK 0x818Au          /* the 4B03h parameter block: the load segment, the relocation factor */
#define DRIVER_PTR 0x818Eu          /* the far pointer called: offset, then segment at 8190h */
#define SAVED_SP 0x8192u
#define SAVED_SS 0x8194u
#define G_MOUSE_HANDLER_INSTALLED 0x8148u
#define G_MOUSE_DISABLED 0x814Au

#ifdef TC_HYBRID
#include "hybrid.h"                 /* tc_hybrid_far_regs: what the original's code left on the bench */
#endif

/* A far call to the driver's code with the n bytes of arguments as Turbo C pushes them; *r gets the
   registers and flags it returns. */
static void driver_call(farptr target, const u8 *args, u16 n, tc_regs *r, const char *site)
{
    u32 v;
    if (tc_code_find(target) != 0) {
        tc_fail(70, "%s: the driver at %04X:%04X has C, whose register convention is M4's", site,
                TC_SEG(target), TC_OFF(target));
    }
    v = tc_code_far(target, args, n, site);
#ifdef TC_HYBRID
    *r = tc_hybrid_far_regs;
#endif
    r->ax = (u16)v;
    r->dx = (u16)(v >> 16);
}

/* 1C00:0002 _LoadOverlayImage: DS:DX the file's name, ES:BX the 4B03h parameter block; sp is SP at
   the `mov ds:[8192h],sp` (1C00:0032). Returns BX: 0, or 0FFFFh on failure; *ds is DS on return
   (DGROUP after the load, the caller's DS when MemAlloc fails). */
static u16 load_overlay_image(u16 *ds, u16 es, u16 bx, u16 dx, u16 sp)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    farptr p;
    u16 ax;

    tc_ds = DGROUP;                                 /* mov ax,212Eh; mov ds,ax */
    p = MemAlloc(0xFA00u, 0x20);
    tc_ds = *ds;                                    /* pop es; pop ds; pop bx */
    ax = (u16)(TC_OFF(p) | TC_SEG(p));              /* or ax,dx */
    if (ax == 0) {
        return 0xFFFFu;                             /* pop dx; jmp loadexit */
    }
    ST16(es, bx, ax);                               /* loaded: the load segment */
    ST16(*ds, SAVED_SP, sp);
    ST16(*ds, SAVED_SS, tc_ss);
    r.ax = 0x4B03u;
    r.bx = bx;
    r.dx = dx;
    r.ds = *ds;
    r.es = es;
    x86_int(0x21, &r);
    *ds = DGROUP;                                   /* mov ax,212Eh; mov ds,ax; SP and SS are the C's own */
    return (r.flags & TC_FLAG_CF) ? 0xFFFFu : 0;    /* mov bx,0FFFFh; jb loadexit; xor bx,bx */
}

/*@ 1C00:0055 */ s16 RunOverlayRequest(farptr req)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const u16 caller_ds = tc_ds;
    u16 si = TC_OFF(req), ds = TC_SEG(req), dx, bx, cx, ax, pushed_ds, pushed_si;
    u8 args[6];
    farptr p;

    dx = LD16(ds, si);                              /* the name's offset */
    si = (u16)(si + 4u);
    pushed_ds = ds;                                 /* push ds; push si */
    pushed_si = si;
    /* SP at the save in _LoadOverlayImage: 8 words pushed here, the two above, the near call's CS and IP,
       then its bp and dx (popped back to it after MemAlloc) */
    bx = load_overlay_image(&ds, ds, LOAD_BLOCK, dx, (u16)(tc_sp - 26u));
    if (bx == 0xFFFFu) {
        goto fail;
    }
    ax = LD16(ds, LOAD_BLOCK);                      /* the overlay's segment */
    ST16(ds, DRIVER_PTR, 0);
    ST16(ds, DRIVER_PTR + 2u, ax);
    tc_va16(args, 0x27u);                           /* push ax; push 1A16h; push 27h */
    tc_va16(args + 2, WAIT_SEG);
    tc_va16(args + 4, ax);
    tc_ds = ds;
    driver_call(TC_MKFP(LD16(ds, DRIVER_PTR + 2u), LD16(ds, DRIVER_PTR)), args, 6, &r, "1C00:0091");
    cx = ax;                                        /* pop cx three times: the segment pushed first */
    ST16(ds, DRIVER_PTR, r.ax);
    bx = (u16)(r.bx << 4);                          /* the size in bytes */
    p = MemRealloc(TC_MKFP(cx, 0), bx);
    if ((TC_OFF(p) | TC_SEG(p)) == 0) {             /* or ax,dx */
        goto fail;
    }
    ST16(ds, DRIVER_PTR + 2u, TC_SEG(p));
    si = pushed_si;                                 /* pop si; pop ds */
    ds = pushed_ds;
    ax = x86_lodsw(ds, &si);                        /* word +4, then word +6 */
    tc_va16(args, ax);
    ax = x86_lodsw(ds, &si);
    tc_va16(args + 2, ax);
    tc_ds = ds;
    driver_call(TC_MKFP(LD16(ds, DRIVER_PTR + 2u), LD16(ds, DRIVER_PTR)), args, 4, &r, "1C00:00C0");
    tc_ds = caller_ds;
    if (r.flags & TC_FLAG_CF) {
        return -1;                                  /* jb failret */
    }
    ds = DGROUP;
    if (LD16(ds, G_MOUSE_HANDLER_INSTALLED) != 0) {
        ST16(ds, G_MOUSE_DISABLED, 0);
    }
    return 0;                                       /* succeeded */
fail:
    tc_ds = caller_ds;
    return -1;
}
