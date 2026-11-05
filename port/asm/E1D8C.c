/* E1D8C.c: src/E1D8C.ASM in C (playbook 80, M3): RandomByte, the 24-bit random step. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

/* The state is three bytes at DGROUP:8484 (the routine loads ES with DGROUP, `mov ax,212Eh` relocated
   at load, and SI with 8484h; it never reads DS). Each instruction's carry is kept as the source keeps
   it. Returns AL = b0 ^ b1 with AH = 0. */
/*@ 1D8C:0000 */ u8 RandomByte(void)
{
    farptr s = TC_MKFP(tc_load_seg + 0x212Eu, 0x8484u);
    u8 b0 = tc_ld_u8(s), b1 = tc_ld_u8(tc_fp_add(s, 1)), b2 = tc_ld_u8(tc_fp_add(s, 2));
    u8 al, cf;

    al = b0;
    al >>= 1;                                   /* shr al,1 */
    cf = al & 1u;                               /* shr al,1: CF = bit 1 of b0 */
    al >>= 1;
    {
        u8 out = b2 >> 7;                       /* rcl byte ptr es:[si+2],1 */
        b2 = (u8)((b2 << 1) | cf);
        cf = out;
    }
    {
        u8 out = b1 >> 7;                       /* rcl byte ptr es:[si+1],1 */
        b1 = (u8)((b1 << 1) | cf);
        cf = out;
    }
    cf ^= 1u;                                   /* cmc */
    al = (u8)(al - b0 - cf);                    /* sbb al,es:[si] */
    cf = al & 1u;                               /* shr al,1 */
    b0 = (u8)((b0 >> 1) | (cf << 7));           /* rcr byte ptr es:[si],1 */
    tc_st_u8(s, b0);
    tc_st_u8(tc_fp_add(s, 1), b1);
    tc_st_u8(tc_fp_add(s, 2), b2);
    return (u8)(b0 ^ b1);
}
