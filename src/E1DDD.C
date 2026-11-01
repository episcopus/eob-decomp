/* Unit at EOB.EXE 1DDD:0006 (106 bytes, 2 functions). Both set the word at ds:FF46 and
   then call the runtime routine at 0000:60BD with three arguments, the count 16 and the
   far addresses ds:FF46 and ds:FF36. f1DDD_0006 sets ds:FF46 to 0xF00 and returns the low
   byte of the word at ds:FF36. f1DDD_002D takes an index: for i below 7 it uses the 7 word
   table at ds:84A2, otherwise the word at ds:84A0; the call happens only when ds:84A2[i]
   differs from ds:84A0 (the caller at 07C8:01C4 passes 7, so it always calls). Placeholder
   names; the runtime routine at 0000:60BD is not in build/re/rtl_compare.json. */

/*@flags -ml -O -Z */

extern void f60BD(unsigned n, void far *src, void far *dst); /*@ 0000:60BD */
extern unsigned g84A0; /*@ ds:84A0 */
extern unsigned g84A2[]; /*@ ds:84A2 */
extern unsigned gFF46; /*@ ds:FF46 */
extern unsigned gFF36; /*@ ds:FF36 */

/*@ 1DDD:0006 */ int f1DDD_0006(void)
{
    gFF46 = 0xF00;
    f60BD(0x10, &gFF46, &gFF36);
    return gFF36 & 0xff;
}

/*@ 1DDD:002D */ void f1DDD_002D(int i)
{
    if (i < 7)
        gFF46 = g84A2[i];
    else
        gFF46 = g84A0;
    if (g84A2[i] != g84A0)
        f60BD(0x10, &gFF46, &gFF36);
}
