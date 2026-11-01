/* Unit at EOB.EXE 19EC:0002 (124 bytes, 3 functions): lookups in the object arena segment table
   at ds:FC0C. ObjAlloc returns a far pointer to a whole arena segment (offset 0) selected by
   1-based kind; the third function transfers between two arenas through the far handler at
   ds:8046. Placeholder names except ObjAlloc (named by its callers in E07A9.C and E1B82.C). */

/*@flags -ml -O -Z */

#include <dos.h>

extern unsigned gSegsFC0C[]; /*@ ds:FC0C */
extern void (far *gHandler8046)(); /*@ ds:8046 */

/*@ 19EC:0002 */ void far *ObjAlloc(int kind)
{
    return MK_FP(gSegsFC0C[kind | 1], 0);
}

/*@ 19EC:0016 */ void far *f19EC_0016(int kind)
{
    return MK_FP(gSegsFC0C[kind & ~1], 0);
}

/*@ 19EC:002A */ void f19EC_002A(int a, int b, int c, int d)
{
    if (gSegsFC0C[a] && gSegsFC0C[b] && gSegsFC0C[a] != gSegsFC0C[b])
        gHandler8046(gSegsFC0C[a], gSegsFC0C[b], c, d);
}
