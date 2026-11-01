/* Unit at EOB.EXE 19EC:0002 (124 bytes, 3 functions): lookups in the page/buffer segment table
   at ds:FC0C. f19EC_0002 returns MK_FP(slot kind|1, 0) and f19EC_0016 is its even twin
   (slot kind & ~1); neither allocates, both only read a segment word from the table.
   f19EC_002A calls the far handler at ds:8046 when two distinct slots hold non-zero segments.
   The table is filled per video mode in main at 07C8:02A4..03D9 (switch on ds:8102, jump table
   cs:0580): screen segments (0xB800, 0xA000) and off-screen page/buffer segments (0xA200/0xA400/
   0xA800, or MemAlloc'd 0xFA00, 0x7D00, 0xCB20, 0xFDE8 byte buffers). The fill writes both
   members of a pair with the same segment (07C8:02A7/02AA, 02C0/02C6, 02DC/02E2, 02F8/02FE),
   hence the low bit forced on or off. Placeholder names. */

/*@flags -ml -O -Z */

#include <dos.h>

extern unsigned gSegsFC0C[]; /*@ ds:FC0C */
extern void (far *gHandler8046)(); /*@ ds:8046 */

/*@ 19EC:0002 */ void far *f19EC_0002(int kind)
{
    return MK_FP(gSegsFC0C[kind | 1], 0);
}

/*@ 19EC:0016 */ void far *f19EC_0016(int kind)
{
    /* The even twin of f19EC_0002 (slot kind & ~1); no direct caller found. */
    return MK_FP(gSegsFC0C[kind & ~1], 0);
}

/*@ 19EC:002A */ void f19EC_002A(int a, int b, int c, int d)
{
    /* Calls the far handler at ds:8046 (cdecl, caller pops 8). c and d are unused by the
       tests below, so their roles are unknown; ds:8046 is not traced to a direct store in
       the image (probably copied from a per-mode handler table) [H]. */
    if (gSegsFC0C[a] && gSegsFC0C[b] && gSegsFC0C[a] != gSegsFC0C[b])
        gHandler8046(gSegsFC0C[a], gSegsFC0C[b], c, d);
}
