/* Unit at EOB.EXE 1AAA:0005 (55 bytes, 1 function): a bounded random number. It calls the
   runtime rand() (0000:55B7, the LCG), scales its result into [lo, hi] and rejects the one
   overflow value hi+1. */

extern int rand(void); /*@ 0000:55B7 */

/* 1AAA:0005: a random number from lo to hi inclusive: rand() scaled into the range, drawn again
   in the one case the scaling gives hi + 1. */
/*@ 1AAA:0005 */ unsigned RandomRange(unsigned lo, unsigned hi)
{
    unsigned roll;

    do {
        roll = (unsigned)((long)rand() * (hi - lo + 1) / 32767) + lo;
    } while (roll > hi);
    return roll;
}
