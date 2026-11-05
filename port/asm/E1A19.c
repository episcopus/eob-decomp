/* E1A19.c: src/E1A19.ASM in C (playbook 80, M3): the drives. FindLastDrive records the current
   default drive and the highest drive number IOCTL 0Eh (get logical drive map) accepts;
   IsLastLogicalDrive tests one drive number with the same call. DOS is the machine's (x86_int,
   INT 21h 19h, 0Eh and 440Eh). The two DGROUP bytes are reached through the caller's DS (tc_ds), as
   the source reaches them: gDefaultDrive ds:FF34, gLastDrive ds:FF00. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define G_DEFAULT_DRIVE 0xFF34u
#define G_LAST_DRIVE 0xFF00u

/*@ 1A19:0006 */ void FindLastDrive(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 dx;

    r.ds = tc_ds;
    r.ax = 0x1900u;
    x86_int(0x21, &r);
    tc_st_u8(TC_MKFP(tc_ds, G_DEFAULT_DRIVE), (u8)r.ax);
    r.dx = (u16)((r.dx & 0xFF00u) | (r.ax & 0xFFu));   /* mov dl,al */
    r.ax = (u16)(0x0E00u | (r.ax & 0xFFu));             /* mov ah,0Eh */
    x86_int(0x21, &r);
    dx = (u16)((r.ax - 1u) & 0xFFu);                    /* dec al; xor ah,ah; mov dx,ax */
    for (;;) {                                          /* scan */
        r.dx = dx;
        r.bx = (u16)((r.bx & 0xFF00u) | (u8)(dx + 1u)); /* mov bl,dl; inc bl */
        r.ax = 0x440Eu;
        x86_int(0x21, &r);
        if (!(r.flags & TC_FLAG_CF) || (u8)r.ax != 0x0Fu) {
            break;                                      /* found */
        }
        dx--;
    }
    tc_st_u8(TC_MKFP(tc_ds, G_LAST_DRIVE), (u8)dx);
}

/*@ 1A19:003B */ s16 IsLastLogicalDrive(s16 drive)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u8 dl = (u8)drive, al;

    r.ds = tc_ds;
    r.dx = (u16)(u8)drive;                              /* mov dl,[bp+6] (DH is the caller's) */
    r.bx = (u16)(u8)(dl + 1u);                          /* mov bl,dl; inc bl */
    r.ax = 0x440Eu;
    x86_int(0x21, &r);
    al = (u8)r.ax;
    if (r.flags & TC_FLAG_CF) {
        return al == 1u ? 1 : 0;                        /* invalid function: yes; any other error: no */
    }
    if (al == 0) {                                      /* mapped */
        return 1;
    }
    al--;
    return al == dl ? 1 : 0;
}
