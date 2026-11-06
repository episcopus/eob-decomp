/* bios.c: the BIOS data area and the BIOS services of the machine (machine.h), as
   tools/oracle/bios.py has them. INT 10h (video), 15h and 16h come with the video and keyboard of
   M3; until then their vectors point at their stubs, as in the oracle, and a call stops. */
#include "tcport.h"
#include "machine.h"

#define BDA_SEG 0x0040u

static void w16(u16 off, u16 v) { tc_st_u16(TC_MKFP(BDA_SEG, off), v); }
static void w8(u16 off, u8 v) { tc_st_u8(TC_MKFP(BDA_SEG, off), v); }
static u16 r16(u16 off) { return tc_ld_u16(TC_MKFP(BDA_SEG, off)); }

void bios_reset(void)
{
    w16(0x10, 0x0061);         /* equipment: floppy, 80x25 colour */
    w16(0x13, 640);            /* KB of conventional memory */
    w16(0x1A, 0x1E);           /* keyboard buffer head and tail */
    w16(0x1C, 0x1E);
    w16(0x80, 0x1E);
    w16(0x82, 0x3E);
    w16(0x49, 0x03);
    w16(0x4A, 80);
    w16(0x63, 0x3D4);
    w8(0x84, 24);
    w16(0x85, 14);
    w8(0x87, 0x60);            /* EGA active, 256 KB */
    w8(0x88, 0x09);            /* switch settings: EGA colour display */
    w8(0x89, 0x00);
}

void bios_int11(tc_regs *r) { r->ax = r16(0x10); }

void bios_int12(tc_regs *r) { r->ax = r16(0x13); }

void bios_int13(tc_regs *r)
{
    if ((r->ax >> 8) == 0x00) {                 /* reset: success */
        r->ax = 0;
        machine_ok(r);
        return;
    }
    machine_unimplemented("INT 13h (disk services are not modelled)", r);
}

static u8 bcd(u32 v) { return (u8)(((v / 10u) << 4) | (v % 10u)); }

void bios_int1a(tc_regs *r)
{
    u8 ah = (u8)(r->ax >> 8);
    if (ah == 0x00) {                           /* the tick count of the BIOS data area */
        r->cx = r16(0x6E);
        r->dx = r16(0x6C);
        r->ax &= 0xFF00u;
    } else if (ah == 0x01) {
        w16(0x6C, r->dx);
        w16(0x6E, r->cx);
    } else if (ah == 0x02) {
        u32 s = machine_time_of_day() / 100u;
        r->cx = (u16)((bcd((s / 3600u) % 24u) << 8) | bcd((s / 60u) % 60u));
        r->dx = (u16)(bcd(s % 60u) << 8);
        machine_ok(r);
    } else if (ah == 0x04) {
        r->cx = 0x1991;
        r->dx = 0x0611;
        machine_ok(r);
    } else {
        machine_unimplemented("INT 1Ah", r);
    }
}
