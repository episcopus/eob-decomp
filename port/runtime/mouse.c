/* mouse.c: the machine's INT 33h driver (machine.h), as tools/oracle/input.py's Mouse has it: the
   functions the game uses (0, 1, 2, 3, 4, 7, 8, 0Ch), driver coordinates 0..639 by 0..199. The events
   the host's mouse makes, and their calls of the game's handler, come with the interrupts (M3). */
#include "tcport.h"
#include "machine.h"

machine_mouse mouse;

/* the event trampoline at F000:3000 (input.py TRAMPOLINE): save the registers, load the event from
   F000:3080, far call the game's handler at F000:3090, restore, IRET */
static const u8 trampoline[] = {
    0x50, 0x53, 0x51, 0x52, 0x56, 0x57, 0x55, 0x1E, 0x06,
    0x2E, 0xA1, 0x80, 0x30,
    0x2E, 0x8B, 0x1E, 0x82, 0x30,
    0x2E, 0x8B, 0x0E, 0x84, 0x30,
    0x2E, 0x8B, 0x16, 0x86, 0x30,
    0x2E, 0x8B, 0x36, 0x88, 0x30,
    0x2E, 0x8B, 0x3E, 0x8A, 0x30,
    0x2E, 0xFF, 0x1E, 0x90, 0x30,
    0x07, 0x1F, 0x5D, 0x5F, 0x5E, 0x5A, 0x59, 0x5B, 0x58,
    0xCF,
};

void mouse_reset(void)
{
    unsigned i;
    static const u8 stub[4] = {0x90, 0xCF, 0x90, 0x90};
    mouse.x = mouse.y = 0;
    mouse.buttons = 0;
    mouse.has_handler = 0;
    mouse.range_x0 = 0;
    mouse.range_x1 = 639;
    mouse.range_y0 = 0;
    mouse.range_y1 = 199;
    mouse.visible = 0;
    for (i = 0; i < sizeof trampoline; i++) {
        tc_st_u8(TC_MKFP(MACHINE_ROM_SEG, (u16)(0x3000u + i)), trampoline[i]);
    }
    /* a program tests for a driver by its vector: not null, not at an IRET (1A72:010E) */
    for (i = 0; i < 4; i++) {
        tc_st_u8(TC_MKFP(MACHINE_ROM_SEG, (u16)(MACHINE_STUB_OFF0 + 0x33u * 4u + i)), stub[i]);
    }
}

void mouse_int33(tc_regs *r)
{
    switch (r->ax) {
    case 0x00:                                  /* reset: two buttons, the pointer centred */
        mouse.has_handler = 0;
        mouse.visible = 0;
        mouse.x = 320;
        mouse.y = 100;
        r->ax = 0xFFFF;
        r->bx = 2;
        return;
    case 0x01:
        mouse.visible++;
        return;
    case 0x02:
        mouse.visible--;
        return;
    case 0x03:
        r->bx = mouse.buttons;
        r->cx = mouse.x;
        r->dx = mouse.y;
        return;
    case 0x04:
        mouse.x = r->cx;
        mouse.y = r->dx;
        return;
    case 0x07:
        mouse.range_x0 = r->cx;
        mouse.range_x1 = r->dx;
        return;
    case 0x08:
        mouse.range_y0 = r->cx;
        mouse.range_y1 = r->dx;
        return;
    case 0x0C:
        mouse.handler_mask = r->cx;
        mouse.has_handler = r->cx != 0;
        mouse.handler_seg = r->es;
        mouse.handler_off = r->dx;
        return;
    default:
        machine_unimplemented("INT 33h", r);
    }
}
