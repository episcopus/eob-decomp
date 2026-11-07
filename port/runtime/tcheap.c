/* tcheap.c: the Borland far heap (playbook 80, M3, "The library on the machine"): farmalloc, farfree,
   realloc, farcoreleft, farheapwalk.

   Written from the library's documented behaviour and the original's observed effects, not translated
   from its code (the library is library, AGENTS.md); each definition carries the library routine's
   address, so the per-function test calls the original's and this one on the same state
   (tests/port_asm/heap.py, and the goldens).

   The heap's blocks are paragraph aligned, from the break level up, each with a header at seg:0000:
     +0 its size in paragraphs, header included
     +2 in use: the previous block's segment (the first block's own); free: 0
     +4 free: a link of the free list (the user's data starts here in a block in use)
     +6 free: the other link
     +8 free: the previous block's segment, kept from +2 while the block is free
   The pointer returned is seg:0004. The free list is a ring through +4 and +6 entered at the rover: a
   block freed goes in just after the rover on the +6 side, an allocation looks at the rover first and
   then along +6, taking the first block that is big enough (from its high end, the rest staying free in
   place, or the whole block when it fits exactly), and a block taken out of the ring moves the rover
   to its +4 neighbour. A freed block joins a free neighbour on either side; the last block is given
   back to the break level instead, with a free block before it. The break level moves the program's
   DOS memory block (INT 21h 4Ah) in 1 KB steps, up to the heap's top. */
#include "tcport.h"
#include "tclib.h"
#include "machine.h"

#include <string.h>

/* the library's data in DGROUP */
#define LIB(off) TC_MKFP(tc_ds, (off))
#define PSP 0x0079u                /* _psp */
#define BRKLVL 0x0087u             /* the break level (far pointer: 0, segment) */
#define HEAPTOP 0x008Bu            /* the heap's top (far pointer) */
#define HEAP_KB 0x85C6u            /* the program's DOS memory in 1 KB units, as the heap last set it */

/* the heap's own words, in the library's code segment at 0000:70B0 */
#define HV(off) TC_MKFP(tc_load_seg, (u16)(0x70B0u + (off)))
#define H_FIRST 0u
#define H_LAST 2u
#define H_ROVER 4u
#define H_DS 6u                    /* DS, saved by each call */
#define H_SIZE 8u                  /* realloc's size, as a long */

static u16 hv(u16 off) { return tc_ld_u16(HV(off)); }
static void set_hv(u16 off, u16 v) { tc_st_u16(HV(off), v); }

static u16 hdr(u16 seg, u16 off) { return tc_ld_u16(TC_MKFP(seg, off)); }
static void set_hdr(u16 seg, u16 off, u16 v) { tc_st_u16(TC_MKFP(seg, off), v); }

#define SIZE 0u
#define PREV 2u
#define LINK4 4u
#define LINK6 6u
#define FREE_PREV 8u

static int is_free(u16 seg) { return hdr(seg, PREV) == 0; }

/* a block's link to the one before it: +2 in use, +8 free */
static void set_prev(u16 seg, u16 prev)
{
    set_hdr(seg, is_free(seg) ? FREE_PREV : PREV, prev);
}

static void enter(void)
{
    set_hv(H_DS, tc_ds);
}

/* paragraphs for a request of `size` bytes, the header included; 0 when it cannot be had */
static u16 paragraphs(u32 size)
{
    u32 p;
    if (size == 0) {
        return 0;
    }
    p = (size + 0x13u) >> 4;
    return p > 0xFFFFu ? 0 : (u16)p;
}

/* ---- the free ring ----------------------------------------------------------------------------- */

static void ring_insert(u16 b)
{
    u16 r = hv(H_ROVER), n;
    if (r == 0) {
        set_hv(H_ROVER, b);
        set_hdr(b, LINK4, b);
        set_hdr(b, LINK6, b);
        return;
    }
    n = hdr(r, LINK6);
    set_hdr(b, LINK6, n);
    set_hdr(b, LINK4, r);
    set_hdr(r, LINK6, b);
    set_hdr(n, LINK4, b);
}

static void ring_remove(u16 b)
{
    u16 n4 = hdr(b, LINK4), n6 = hdr(b, LINK6);
    if (n4 == b) {
        set_hv(H_ROVER, 0);
        return;
    }
    set_hdr(n4, LINK6, n6);
    set_hdr(n6, LINK4, n4);
    set_hv(H_ROVER, n4);
}

/* ---- the break level ---------------------------------------------------------------------------- */

/* the program's DOS memory for a break level at `brk`: 1 KB past it, but not past the heap's top;
   0 when DOS refuses */
static int set_memory(u16 brk)
{
    u16 psp = tc_ld_u16(LIB(PSP));
    u16 top = tc_ld_u16(LIB((u16)(HEAPTOP + 2u)));
    u16 units = (u16)(((u16)(brk - psp) >> 6) + 1u);
    u16 paras = (u16)(units << 6);
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if ((u32)psp + paras > top) {
        paras = (u16)(top - psp);
    }
    if ((u16)(paras >> 6) == tc_ld_u16(LIB(HEAP_KB))) {
        return 1;
    }
    r.ax = 0x4A00u;
    r.bx = paras;
    r.es = psp;
    machine_library_int(0x21, &r);
    if (r.flags & 1u) {
        return 0;
    }
    tc_st_u16(LIB(HEAP_KB), (u16)(paras >> 6));
    return 1;
}

static void set_brk(u16 seg)
{
    tc_st_u16(LIB(BRKLVL), 0);
    tc_st_u16(LIB((u16)(BRKLVL + 2u)), seg);
}

static u16 brk(void)
{
    return tc_ld_u16(LIB((u16)(BRKLVL + 2u)));
}

/* a new block of `need` paragraphs at the break level; 0 when the heap's top or DOS refuses */
static u16 grow(u16 need)
{
    u16 b = brk(), top = tc_ld_u16(LIB((u16)(HEAPTOP + 2u)));
    u32 end = (u32)b + need;
    if (end > top) {
        return 0;
    }
    if (!set_memory((u16)end)) {
        return 0;
    }
    set_brk((u16)end);
    set_hdr(b, SIZE, need);
    if (hv(H_FIRST) == 0) {
        set_hv(H_FIRST, b);
        set_hdr(b, PREV, b);
    } else {
        set_hdr(b, PREV, hv(H_LAST));
    }
    set_hv(H_LAST, b);
    return b;
}

/* the last block given back to the break level, with a free block before it */
static void release_last(u16 b)
{
    u16 p;
    if (b == hv(H_FIRST)) {
        set_hv(H_FIRST, 0);
        set_hv(H_LAST, 0);
        set_hv(H_ROVER, 0);
    } else {
        p = hdr(b, PREV);
        if (is_free(p)) {
            ring_remove(p);
            if (p == hv(H_FIRST)) {
                set_hv(H_FIRST, 0);
                set_hv(H_LAST, 0);
                set_hv(H_ROVER, 0);
            } else {
                set_hv(H_LAST, hdr(p, FREE_PREV));
            }
            b = p;
        } else {
            set_hv(H_LAST, p);
        }
    }
    set_brk(b);
    set_memory(b);
}

/* ---- ALLOC.H ----------------------------------------------------------------------------------- */

static u16 take(u16 need)
{
    u16 r = hv(H_ROVER), b;
    if (r != 0) {
        b = r;
        do {
            u16 size = hdr(b, SIZE);
            if (size >= need) {
                if (size == need) {
                    ring_remove(b);
                    set_hdr(b, PREV, hdr(b, FREE_PREV));
                    return b;
                } else {
                    u16 n = (u16)(b + size - need);
                    set_hdr(b, SIZE, (u16)(size - need));
                    set_hdr(n, SIZE, need);
                    set_hdr(n, PREV, b);
                    set_hdr((u16)(n + need), PREV, n);
                    return n;
                }
            }
            b = hdr(b, LINK6);
        } while (b != r);
    }
    return grow(need);
}

/*@ 0000:72DE library */ farptr tcl_farmalloc(u32 size)
{
    u16 need = paragraphs(size), b;
    enter();
    if (need == 0) {
        return 0;
    }
    b = take(need);
    return b ? TC_MKFP(b, 4) : 0;
}

static void free_block(u16 b)
{
    u16 p, n;
    if (b == hv(H_LAST)) {
        release_last(b);
        return;
    }
    p = hdr(b, PREV);
    set_hdr(b, FREE_PREV, p);                  /* marked free, even when it then joins the one before */
    set_hdr(b, PREV, 0);
    if (is_free(p)) {
        set_hdr(p, SIZE, (u16)(hdr(p, SIZE) + hdr(b, SIZE)));
        set_prev((u16)(p + hdr(p, SIZE)), p);
        b = p;
    } else {
        ring_insert(b);
    }
    n = (u16)(b + hdr(b, SIZE));
    if (is_free(n)) {
        set_hdr(b, SIZE, (u16)(hdr(b, SIZE) + hdr(n, SIZE)));
        set_prev((u16)(b + hdr(b, SIZE)), b);
        ring_remove(n);
    }
}

/*@ 0000:71EE library */ void tcl_farfree(farptr block)
{
    enter();
    if (block == 0) {
        return;
    }
    free_block(TC_SEG(block));
}

/* the large model's realloc, farrealloc with the size as a long: NULL allocates; a size of 0 frees
   (the original then returns what its free left in DX:AX; the game never asks, and this one returns
   NULL); fewer paragraphs split the block and free the tail; more allocate, copy the old data and free
   the old block */
/*@ 0000:743A library */ farptr tcl_realloc(farptr block, u16 size_)
{
    u32 size = size_;
    u16 b, have, need, n;
    enter();
    set_hv(H_SIZE, (u16)(size >> 16));
    set_hv((u16)(H_SIZE + 2u), (u16)size);
    if (block == 0) {
        return tcl_farmalloc(size);
    }
    if (size == 0) {
        tcl_farfree(block);
        return 0;
    }
    b = TC_SEG(block);
    have = hdr(b, SIZE);
    need = paragraphs(size);
    if (need == have) {
        return block;
    }
    if (need < have) {
        u16 t = (u16)(b + need);
        set_hdr(b, SIZE, need);
        if (b == hv(H_LAST)) {                 /* the last block: the break level comes down to it */
            set_brk(t);
            set_memory(t);
            return block;
        }
        set_hdr(t, SIZE, (u16)(have - need));
        set_hdr(t, PREV, b);
        set_prev((u16)(t + have - need), t);
        free_block(t);
        return block;
    }
    n = take(need);
    if (n == 0) {
        return 0;
    }
    {
        u32 len = (u32)have * 16u - 4u, i;
        for (i = 0; i < len; i++) {
            tc_st_u8(TC_MKFP((u16)(n + (u16)(i >> 4)), (u16)(4u + (i & 15u))),
                     tc_ld_u8(TC_MKFP((u16)(b + (u16)(i >> 4)), (u16)(4u + (i & 15u)))));
        }
    }
    free_block(b);
    return TC_MKFP(n, 4);
}

/* the memory between the break level and the heap's top, less a paragraph */
/*@ 0000:5244 library */ u32 tcl_farcoreleft(void)
{
    u16 top = tc_ld_u16(LIB((u16)(HEAPTOP + 2u)));
    return ((u32)(u16)(top - brk()) << 4) - 16u;
}

/* struct farheapinfo { void huge *ptr; unsigned long size; int in_use; }: the block after the one
   given (the first for NULL); 2 with it, 5 at the end, 1 for an empty heap [H: the empty heap not
   seen] */
/*@ 0000:76C0 library */ s16 tcl_farheapwalk(farptr hi)
{
    farptr p = tc_ld_u32(hi);
    u16 b;
    if (hv(H_FIRST) == 0) {
        return 1;
    }
    if (p == 0) {
        b = hv(H_FIRST);
    } else {
        b = TC_SEG(p);
        if (b == hv(H_LAST)) {
            return 5;
        }
        b = (u16)(b + hdr(b, SIZE));
    }
    tc_st_u32(hi, TC_MKFP(b, 4));
    tc_st_u32(tc_fp_add(hi, 4), (u32)hdr(b, SIZE) << 4);
    tc_st_u16(tc_fp_add(hi, 8), is_free(b) ? 0u : 1u);
    return 2;
}
