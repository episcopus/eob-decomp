/* Unit at EOB.EXE 1AAA:0005 (55 bytes, 1 function): a bounded random number. It calls the
   runtime rand() (0000:55B7, the LCG), scales its result into [lo, hi] and rejects the one
   overflow value hi+1. Names and types are placeholders until the callers are understood. */

extern int rand(void); /*@ 0000:55B7 */

/*@ 1AAA:0005 */ unsigned RandomRange(unsigned lo, unsigned hi)
{
    unsigned r;

    do {
        r = (unsigned)((long)rand() * (hi - lo + 1) / 32767) + lo;
    } while (r > hi);
    return r;
}
