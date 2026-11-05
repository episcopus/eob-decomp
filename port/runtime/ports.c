/* ports.c: the machine's I/O ports (machine.h), as tools/oracle/machine.py (port_read, port_write),
   ega.py (port_read, port_write) and adlib.py have them: the interrupt controller's mask and EOI, the
   timer's channels 0 and 2, port 61h, the keyboard's data port, the MCGA's DAC and status ports and
   the AdLib. A port none of them models reads as all ones and takes any write, as in the oracle.

   Time: the free-running counters (40h, 42h), the retrace bit (3DAh) and the AdLib's timers read the
   clock, and a poll of 3DAh or of the AdLib's status charges it (machine_time.extra), so a polling
   loop ends as it does in the oracle. The AdLib's timers are compared in instructions, not in seconds
   as the oracle's floats do (the port has no floating point, playbook 80 rule 4); the shadow test
   would show a poll on the boundary that the two decide differently. */
#include "tcport.h"
#include "machine.h"
#include "platform.h"

machine_hw hw;

static u32 now(void) { return machine_now(); }

/* Machine._irq0_period: instructions between two timer interrupts */
u32 machine_irq0_period(void)
{
    u32 p = (u32)(((unsigned long long)hw.pit_reload * machine_time.insn_per_sec) / 1193182u);
    return p ? p : 1u;
}

void ports_reset(int adlib)
{
    unsigned i;
    hw.pic_mask = 0;
    hw.port61 = 0;
    hw.irq0_in_service = hw.irq1_in_service = 0;
    hw.kbd_data = -1;
    hw.pit_reload = 0x10000u;
    hw.pit_latch_lo = 1;
    hw.pit_mode_access = 3;
    hw.pit_lo = 0;
    hw.pit2_reload = 0;
    hw.pit2_lo = 1;
    hw.next_irq0 = machine_irq0_period();
    hw.dac_read = hw.dac_write = 0;
    hw.attr_ff = 0;
    hw.adlib = (u8)adlib;
    hw.adlib_index = 0;
    hw.adlib_flags = 0;
    for (i = 0; i < 256; i++) {
        hw.adlib_regs[i] = 0;
    }
    hw.adlib_running[0] = hw.adlib_running[1] = 0;
}

/* ---- the AdLib (adlib.py) ---- */

static void adlib_timers(void)
{
    static const u32 step_us[2] = {80u, 320u};
    static const u8 bit[2] = {0x40, 0x20};
    unsigned t;
    for (t = 0; t < 2; t++) {
        u64 elapsed_us, period_us;
        if (!hw.adlib_running[t]) {
            continue;
        }
        elapsed_us = ((u64)(now() - hw.adlib_started[t]) * 1000000u) / machine_time.insn_per_sec;
        period_us = (u64)(256u - hw.adlib_regs[2 + t]) * step_us[t];
        if (elapsed_us >= period_us && !(hw.adlib_regs[4] & bit[t])) {
            hw.adlib_flags |= (u8)(bit[t] | 0x80u);
        }
    }
}

static u16 adlib_read(u16 port)
{
    if (port == 0x388) {
        adlib_timers();
        machine_time.extra += 20;               /* a status poll costs time, so delay loops end */
        return (u16)(hw.adlib_flags | 0x06u);   /* bits 2 and 1 read as 1 on an OPL2 */
    }
    return 0xFF;
}

static void adlib_write(u16 port, u8 val)
{
    unsigned t;
    u8 r;
    if (port == 0x388) {
        hw.adlib_index = val;
        return;
    }
    r = hw.adlib_index;
    platform_adlib_write(r, val);
    if (r == 0x04) {
        if (val & 0x80) {                       /* reset the IRQ and timer flags; the other bits are ignored */
            hw.adlib_flags = 0;
            return;
        }
        for (t = 0; t < 2; t++) {
            if (val & (1u << t)) {
                if (!hw.adlib_running[t]) {
                    hw.adlib_running[t] = 1;
                    hw.adlib_started[t] = now();
                }
            } else {
                hw.adlib_running[t] = 0;
            }
        }
    }
    hw.adlib_regs[r] = val;
}

/* ---- the MCGA's ports (ega.py, the part an MCGA has) ---- */

static u16 video_read(u16 port, int size)
{
    if (port == 0x3DA || port == 0x3BA) {
        /* a frame is 1/60 s of the clock, its last tenth vertical retrace; a poll charges 40 */
        u32 frame, ph;
        int vret, hret;
        hw.attr_ff = 0;
        machine_time.extra += 40;
        frame = machine_time.insn_per_sec / 60u;
        if (frame < 1000u) {
            frame = 1000u;
        }
        ph = now() % frame;
        vret = ph >= frame - frame / 10u;
        hret = (int)((ph / 30u) & 1u);
        return (u16)((vret ? 0x08u : 0u) | ((vret || hret) ? 0x01u : 0u));
    }
    if (port == 0x3C9) {
        u8 v = video.dac[hw.dac_read % 768u];
        hw.dac_read = (u16)((hw.dac_read + 1u) % 768u);
        return v;
    }
    if (port == 0x3C6) {
        return 0xFF;
    }
    machine_unimplemented_port("in", port, size);
}

static void video_write(u16 port, int size, u16 val)
{
    if (size == 2) {                            /* OUT DX,AX writes the index, then the data */
        video_write(port, 1, (u16)(val & 0xFFu));
        video_write((u16)(port + 1u), 1, (u16)(val >> 8));
        return;
    }
    switch (port) {
    case 0x3C7:
        hw.dac_read = (u16)((val & 0xFFu) * 3u);
        return;
    case 0x3C8:
        hw.dac_write = (u16)((val & 0xFFu) * 3u);
        return;
    case 0x3C9:
        video.dac[hw.dac_write % 768u] = (u8)(val & 0x3Fu);
        hw.dac_write = (u16)((hw.dac_write + 1u) % 768u);
        return;
    case 0x3C6:
    case 0x3BA:
    case 0x3DA:
    case 0x3CA:
    case 0x3C3:
    case 0x3D8:
    case 0x3D9:
    case 0x3B8:                                 /* PEL mask, CGA ports: accepted, not modelled */
        return;
    default:
        machine_unimplemented_port("out", port, size);
    }
}

/* ---- the dispatch (machine.py) ---- */

u16 machine_in(u16 port, int size)
{
    if (hw.adlib && (port == 0x388 || port == 0x389)) {
        return adlib_read(port);
    }
    if (port >= 0x3B0 && port <= 0x3DF) {
        return video_read(port, size);
    }
    switch (port) {
    case 0x40:
        return (u16)((now() / 3u) & 0xFFu);     /* free-running counter low byte; deterministic */
    case 0x42:
        return (u16)((now() / 5u) & 0xFFu);
    case 0x60:
        return hw.kbd_data >= 0 ? (u16)hw.kbd_data : 0u;
    case 0x61:
        return hw.port61;
    case 0x21:
        return hw.pic_mask;
    case 0x20:
    case 0xA0:
        return 0;
    case 0x201:
        return 0xFF;                            /* no joystick */
    default:
        return size == 1 ? 0xFFu : 0xFFFFu;     /* not modelled, as the oracle: all ones */
    }
}

static void pit0_write(u8 val)
{
    u8 acc = hw.pit_mode_access;
    if (acc == 1) {
        hw.pit_reload = val ? val : 0x100u;
    } else if (acc == 2) {
        hw.pit_reload = val ? (u32)val << 8 : 0x10000u;
    } else {
        if (hw.pit_latch_lo) {
            hw.pit_lo = val;
        } else {
            u32 v = ((u32)val << 8) | hw.pit_lo;
            hw.pit_reload = v ? v : 0x10000u;
            hw.next_irq0 = now() + machine_irq0_period();
        }
        hw.pit_latch_lo = (u8)!hw.pit_latch_lo;
    }
}

void machine_out(u16 port, int size, u16 val)
{
    if (hw.adlib && (port == 0x388 || port == 0x389)) {
        adlib_write(port, (u8)val);
        return;
    }
    if (port >= 0x3B0 && port <= 0x3DF) {
        video_write(port, size, val);
        return;
    }
    switch (port) {
    case 0x20:
        if ((val & 0xE0u) == 0x20u) {           /* EOI: the highest priority request in service */
            if (hw.irq0_in_service) {
                hw.irq0_in_service = 0;
            } else {
                hw.irq1_in_service = 0;
            }
        }
        return;
    case 0x21:
        hw.pic_mask = (u8)val;
        return;
    case 0x43: {
        u8 ch = (u8)((val >> 6) & 3u);
        if (ch == 0) {
            hw.pit_mode_access = (u8)((val >> 4) & 3u);
            hw.pit_latch_lo = 1;
        } else if (ch == 2) {
            hw.pit2_lo = 1;
        }
        return;
    }
    case 0x40:
        pit0_write((u8)val);
        return;
    case 0x42:
        if (hw.pit2_lo) {
            hw.pit2_reload = (u16)((hw.pit2_reload & 0xFF00u) | (val & 0xFFu));
        } else {
            hw.pit2_reload = (u16)((hw.pit2_reload & 0x00FFu) | ((val & 0xFFu) << 8));
            platform_speaker(hw.pit2_reload, (u8)(hw.port61 & 3u));
        }
        hw.pit2_lo = (u8)!hw.pit2_lo;
        return;
    case 0x61:
        hw.port61 = (u8)val;
        platform_speaker(hw.pit2_reload, (u8)(val & 3u));
        return;
    default:                                    /* the Tandy chip (C0h) and the rest: taken */
        return;
    }
}
