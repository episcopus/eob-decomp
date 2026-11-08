/* intro.c: INTRO.EXE's data for the intro (playbook 80, M7, "The intro").

   The intro's own code (src/I0892.C, src/IOV0.C, translated with tools/port/introsrc.py) runs on the game's
   copies of the units the two programs share, and reads what is its alone (the logo tables, the
   strings, its variables) in the segment TC_INTRO_SEG, at the offsets it has in INTRO.EXE's DGROUP. So
   that segment starts as the DGROUP of INTRO.EXE in the game directory, relocated: a far pointer to its
   own data goes to TC_INTRO_SEG. */
#include <stdlib.h>
#include <string.h>

#include "tcport.h"
#include "platform.h"
#include "start.h"

void intro_load_data(void)
{
    s32 size = platform_file_size("INTRO.EXE");
    u8 *f;
    u32 h, n, dg_para, dg, nrel, rel, i;
    if (size < 64 || (f = malloc((size_t)size)) == NULL || platform_file_read("INTRO.EXE", f, (u32)size) != 0) {
        tc_fail(70, "intro: cannot read INTRO.EXE");
    }
    h = (u32)(f[8] | f[9] << 8) * 16u;
    n = (u32)(f[4] | f[5] << 8) * 512u - h;
    if ((f[2] | f[3] << 8) != 0) {
        n -= 512u - (u32)(f[2] | f[3] << 8);
    }
    dg_para = (u32)(f[h + 1u] | f[h + 2u] << 8);        /* C0's first instruction: mov dx, DGROUP */
    dg = dg_para * 16u;
    memcpy(tc_mem + (u32)TC_INTRO_SEG * 16u, f + h + dg, n - dg);
    nrel = (u32)(f[6] | f[7] << 8);
    rel = (u32)(f[24] | f[25] << 8);
    for (i = 0; i < nrel; i++) {
        const u8 *e = f + rel + i * 4u;
        u32 at = (u32)(e[0] | e[1] << 8) + (u32)(e[2] | e[3] << 8) * 16u;
        if (at >= dg && at + 1u < n) {
            u32 a = (u32)TC_INTRO_SEG * 16u + (at - dg);
            u32 v = (u32)(tc_mem[a] | tc_mem[a + 1u] << 8);
            if (v == dg_para) {
                v = TC_INTRO_SEG;
                tc_mem[a] = (u8)v;
                tc_mem[a + 1u] = (u8)(v >> 8);
            }
        }
    }
    free(f);
}
