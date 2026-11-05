/* E1DE4.c: src/E1DE4.ASM in C (playbook 80, M3): the VGA retrace, read once or waited on.

   Both read the Input Status Register 1 (port 3DAh), whose bit 3 is the vertical retrace. Each read is
   one access of the machine's port, which charges the clock as the oracle's does (a poll costs 40
   instructions), so a wait ends after as many reads as the original's. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define STATUS 0x3DAu
#define RETRACE 0x08u

/*@ 1DE4:0000 */ s16 IsInVerticalRetrace(void)
{
    return (s16)(x86_inb(STATUS) & RETRACE);    /* in al,dx; and al,8; xor ah,ah */
}

/*@ 1DE4:000D */ void WaitRetrace(s16 mode)
{
    if ((u8)mode == 1) {                        /* cmp bl,1 (the low byte of the argument) */
        while (x86_inb(STATUS) & RETRACE) {     /* endwait: while in retrace */
        }
        while (!(x86_inb(STATUS) & RETRACE)) {  /* startloop: until the next retrace begins */
        }
    } else {
        while (!(x86_inb(STATUS) & RETRACE)) {  /* startwait: until a retrace begins */
        }
        while (x86_inb(STATUS) & RETRACE) {     /* endloop: while it lasts */
        }
    }
}
