/* E19EA.c: src/E19EA.ASM in C (playbook 80, M3): print a NUL-terminated far string, one character at
   a time with DOS function 06h (direct console output), through the machine's INT 21h. Every register
   the routine uses is saved in the original, so it returns nothing. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

/*@ 19EA:0000 */ void PrintString(farptr s)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 si = TC_OFF(s), ds = TC_SEG(s);
    u8 al;

    for (;;) {                                      /* next */
        al = x86_lodsb(ds, &si);
        if (al == 0) {
            break;                                  /* done */
        }
        r.dx = (u16)((r.dx & 0xFF00u) | al);        /* mov dl,al */
        r.ax = (u16)(0x0600u | al);                 /* mov ah,6 (AL is the character lodsb read) */
        r.ds = ds;
        r.si = si;
        x86_int(0x21, &r);
    }
}
