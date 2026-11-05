/* machine.c: the vector table, the ROM and service dispatch (machine.h), as tools/oracle/machine.py
   builds them (_build_rom, register_service). */
#include <stdio.h>
#include <string.h>

#include "tcport.h"
#include "machine.h"

machine_clock machine_time = {0, 0, 1000000u, 0, 0, 0, 0};
int machine_exit_code = -1;

typedef void (*service_fn)(tc_regs *r);
static service_fn services[256];

static void set_ivt(u8 n, u16 seg, u16 off)
{
    tc_st_u16(TC_MKFP(0, (u16)(n * 4u)), off);
    tc_st_u16(TC_MKFP(0, (u16)(n * 4u + 2u)), seg);
}

static void put(u16 seg, u16 off, const u8 *b, u16 n)
{
    u16 i;
    for (i = 0; i < n; i++) {
        tc_st_u8(TC_MKFP(seg, (u16)(off + i)), b[i]);
    }
}

static void register_service(u8 n, service_fn fn)
{
    services[n] = fn;
    set_ivt(n, MACHINE_ROM_SEG, (u16)(MACHINE_STUB_OFF0 + n * 4u));
}

void machine_boot(const char *exe_name, const u8 *cmdline, u16 cmdlen, int with_mouse, int adlib)
{
    static const u8 stub[4] = {0xCF, 0x90, 0x90, 0x90};
    /* default INT 8: inc the BIOS tick count, chain INT 1Ch, EOI (machine.py _build_rom) */
    static const u8 int8[] = {0x50, 0x1E, 0xB8, 0x40, 0x00, 0x8E, 0xD8, 0xFF, 0x06, 0x6C, 0x00, 0x75, 0x04,
                              0xFF, 0x06, 0x6E, 0x00, 0xCD, 0x1C, 0xB0, 0x20, 0xE6, 0x20, 0x1F, 0x58, 0xCF};
    static const u8 eoi[] = {0x50, 0xB0, 0x20, 0xE6, 0x20, 0x58, 0xCF};
    static const u8 iret = 0xCF;
    unsigned n;

    memset(services, 0, sizeof services);
    machine_exit_code = -1;
    machine_time.tod_offset_set = 0;
    put(MACHINE_ROM_SEG, 0, &iret, 1);
    for (n = 0; n < 256; n++) {
        put(MACHINE_ROM_SEG, (u16)(MACHINE_STUB_OFF0 + n * 4u), stub, 4);
    }
    put(MACHINE_ROM_SEG, MACHINE_ROM_INT8, int8, sizeof int8);
    put(MACHINE_ROM_SEG, MACHINE_ROM_EOI, eoi, sizeof eoi);
    for (n = 0; n < 256; n++) {
        set_ivt((u8)n, MACHINE_ROM_SEG, 0);
    }
    for (n = 0x08; n < 0x10; n++) {
        set_ivt((u8)n, MACHINE_ROM_SEG, MACHINE_ROM_EOI);
    }
    set_ivt(0x08, MACHINE_ROM_SEG, MACHINE_ROM_INT8);
    for (n = 0x70; n < 0x78; n++) {
        set_ivt((u8)n, MACHINE_ROM_SEG, MACHINE_ROM_EOI);
    }
    /* bios.py install, then dos.py install (the order the oracle registers them) */
    bios_reset();
    video_reset();
    register_service(0x10, bios_int10);
    register_service(0x11, bios_int11);
    register_service(0x12, bios_int12);
    register_service(0x15, 0);
    register_service(0x16, 0);
    register_service(0x1A, bios_int1a);
    register_service(0x13, bios_int13);
    register_service(0x17, 0);
    register_service(0x14, 0);
    register_service(0x21, dos_int21);
    register_service(0x20, dos_int20);
    register_service(0x25, 0);
    register_service(0x26, 0);
    register_service(0x27, 0);
    register_service(0x29, dos_int29);
    register_service(0x2F, dos_int2f);
    dos_reset(cmdline, cmdlen, exe_name);
    /* run.py build_machine: the mouse driver, or "no driver" */
    ports_reset(adlib);
    if (with_mouse) {
        register_service(0x33, mouse_int33);
        mouse_reset();
    } else {
        register_service(0x33, mouse_none_int33);
    }
}

int machine_service(u8 n, tc_regs *r)
{
    if (services[n] == 0) {
        return -1;
    }
    services[n](r);
    return 0;
}

void machine_int(u8 n, tc_regs *r)
{
    farptr v = tc_ld_u32(TC_MKFP(0, (u16)(n * 4u)));
    u16 seg = TC_SEG(v), off = TC_OFF(v);
    if (seg == MACHINE_ROM_SEG && off >= MACHINE_STUB_OFF0 && off < MACHINE_STUB_OFF0 + 0x400u
        && (off - MACHINE_STUB_OFF0) % 4u == 0) {
        u8 k = (u8)((off - MACHINE_STUB_OFF0) / 4u);
        if (machine_service(k, r) != 0) {
            machine_unimplemented("INT has no service (vector points at its stub)", r);
        }
        return;
    }
    if (seg == MACHINE_ROM_SEG && off == 0) {
        return;                                /* the generic IRET */
    }
    tc_fail(74, "INT %02Xh goes to the game's handler at %04X:%04X, which the machine cannot call yet (M3)",
            n, seg, off);
}

u32 machine_time_of_day(void)
{
    /* Machine.time_of_day: 12:00:00.00 plus the clock; a pinned clock's first read returns the pin */
    s32 t = (s32)(12u * 360000u + (u32)(((unsigned long long)machine_now() * 100u) / machine_time.insn_per_sec));
    if (machine_time.pinned) {
        if (!machine_time.tod_offset_set) {
            machine_time.tod_offset = (s32)machine_time.clock_pin - t;
            machine_time.tod_offset_set = 1;
        }
        t += machine_time.tod_offset;
    }
    t %= 8640000;
    if (t < 0) {
        t += 8640000;
    }
    return (u32)t;
}

void machine_unimplemented_port(const char *dir, u16 port, int size)
{
    tc_fail(74, "UNIMPLEMENTED in the machine: %s port %04Xh (size %d)", dir, port, size);
}

void machine_unimplemented(const char *what, const tc_regs *r)
{
    tc_fail(74, "UNIMPLEMENTED in the machine: %s | AX=%04X BX=%04X CX=%04X DX=%04X", what, r->ax, r->bx,
            r->cx, r->dx);
}

/* ---- INT 33h with no driver (input.py no_mouse): a reset answers AX=0 ---- */
void mouse_none_int33(tc_regs *r)
{
    if (r->ax == 0) {
        r->ax = 0;
        r->bx = 0;
    }
}
