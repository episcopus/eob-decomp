/* Unit at EOB.EXE 1A65:000E (200 bytes, 1 function): measure the display vertical
   retrace bit and set gFE6C, the mode E1A16's WaitFrames passes to its frame wait.
   It waits for the BIOS tick counter (0000:046C, the linear alias of 0040:006C) to
   change, then for four more ticks (about 220 ms), then samples the retrace bit of
   port 3DAh (through 1DE4:0000) 10000 times and sets gFE6C = 1 when it read the bit
   set more often than clear. `timer` is a far pointer into segment 0. Placeholder
   name; the two chained assignments are what the compiler needs to merge the two
   zero stores into one `xor ax,ax` (-Z). */

#include <dos.h>

extern int gFE6C; /*@ ds:FE6C */

extern void far DisableInterrupts(void); /*@ 1A72:02FC */
extern void far EnableInterrupts(void); /*@ 1A72:02FA */
extern int f1DE4_0000(void); /*@ 1DE4:0000 */

/*@ 1A65:000E */ void CalibrateRetrace(void)
{
    unsigned long far *timer = (unsigned long far *)MK_FP(0, 0x46c);
    unsigned long deadline;
    int clear, set, n;
    register int i, k;

    i = gFE6C = 0;
    DisableInterrupts();
    deadline = *timer;
    EnableInterrupts();
    while (deadline == *timer)
        ;
    DisableInterrupts();
    deadline = *timer + 4;
    EnableInterrupts();
    while (*timer < deadline) {
        i++;
        for (k = 200; k; k--)
            ;
    }
    clear = set = 0;
    for (n = 0; n < 10000; n++) {
        if (f1DE4_0000())
            set++;
        else
            clear++;
    }
    if (set > clear)
        gFE6C = 1;
}
