/* video.c: the machine's video BIOS, INT 10h (machine.h), as tools/oracle/bios.py and ega.py have
   it, for the one adapter the port has: MCGA (decision 4). Mode 13h's 64 KB are the arena at
   A000:0000; the DAC is kept here, six bits a component, for the host to show. Another mode is
   remembered with its BIOS data area, without its memory: the game sets mode 13h, and the text mode
   only when it ends. */
#include <string.h>

#include "tcport.h"
#include "machine.h"

machine_video video;

/* the BIOS's EGA colours (ega.py DEFAULT_ATTR) as the DAC holds them after a mode 13h set */
static const u8 default_attr[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
                                    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F};

static u8 ega6(u8 v, int shift_hi, int shift_lo)
{
    return (u8)(((((v >> shift_hi) & 1u) << 1) | ((v >> shift_lo) & 1u)) * 85u);
}

static void bda8(u16 off, u8 v) { tc_st_u8(TC_MKFP(0x40, off), v); }
static void bda16(u16 off, u16 v) { tc_st_u16(TC_MKFP(0x40, off), v); }

void video_reset(void)
{
    memset(&video, 0, sizeof video);
    video.mode = 3;
}

/* ega.py set_mode */
static void set_mode(u8 mode, int clear)
{
    u16 cols, page_bytes;
    unsigned i;
    mode &= 0x7F;
    video.mode = mode;
    if (mode == 0x13) {
        memset(video.dac, 0, sizeof video.dac);
        for (i = 0; i < 16; i++) {          /* the BIOS loads the EGA colours first */
            u8 v = default_attr[i];
            video.dac[3 * i] = (u8)(ega6(v, 2, 5) >> 2);
            video.dac[3 * i + 1] = (u8)(ega6(v, 1, 4) >> 2);
            video.dac[3 * i + 2] = (u8)(ega6(v, 0, 3) >> 2);
        }
    }
    if (clear) {
        memset(tc_mem + 0xA0000u, 0, 0x10000u);
    }
    video.page = 0;
    cols = (mode == 0 || mode == 1 || mode == 0x0D) ? 40 : 80;
    page_bytes = mode == 0x13 ? 0xFA00u : mode == 0x0D ? 0x2000u : mode == 0x0E ? 0x4000u
               : (mode == 0x0F || mode == 0x10) ? 0x8000u : 0x1000u;
    bda8(0x49, mode);
    bda16(0x4A, cols);
    bda16(0x4C, page_bytes);
    bda16(0x4E, 0);
    bda16(0x63, 0x3D4);
    bda8(0x84, 24);
    bda16(0x85, (mode == 0x0D || mode == 0x0E) ? 8 : 14);
}

static void palette(tc_regs *r, u8 al)
{
    u16 i = (u16)(r->bx & 0xFFu), n = r->cx, k;
    switch (al) {
    case 0x10:                                  /* one DAC register: BX index, DH/CH/CL = R/G/B */
        video.dac[3 * i] = (u8)((r->dx >> 8) & 0x3F);
        video.dac[3 * i + 1] = (u8)((r->cx >> 8) & 0x3F);
        video.dac[3 * i + 2] = (u8)(r->cx & 0x3F);
        return;
    case 0x12:                                  /* a block: BX first, CX count, ES:DX the table */
        for (k = 0; k < n && i + k < 256u; k++) {
            u32 a = TC_LINEAR(TC_MKFP(r->es, r->dx)) + 3u * k;
            video.dac[3 * (i + k)] = (u8)(tc_mem[a & 0xFFFFFu] & 0x3F);
            video.dac[3 * (i + k) + 1] = (u8)(tc_mem[(a + 1u) & 0xFFFFFu] & 0x3F);
            video.dac[3 * (i + k) + 2] = (u8)(tc_mem[(a + 2u) & 0xFFFFFu] & 0x3F);
        }
        return;
    case 0x15:
        r->dx = (u16)((r->dx & 0xFFu) | (video.dac[3 * i] << 8));
        r->cx = (u16)((video.dac[3 * i + 1] << 8) | video.dac[3 * i + 2]);
        return;
    case 0x17: {
        u32 a = TC_LINEAR(TC_MKFP(r->es, r->dx));
        for (k = 0; k < 3u * n && 3u * i + k < sizeof video.dac; k++) {
            tc_mem[(a + k) & 0xFFFFFu] = video.dac[3 * i + k];
        }
        return;
    }
    case 0x13:
    case 0x1A:
    case 0x1B:                                  /* colour paging, grey summing: no effect */
        return;
    default:
        machine_unimplemented("INT 10h AH=10h (the attribute palette is EGA's; the port is MCGA)", r);
    }
}

void bios_int10(tc_regs *r)
{
    u8 ah = (u8)(r->ax >> 8), al = (u8)r->ax;
    switch (ah) {
    case 0x00:
        set_mode(al, !(al & 0x80));
        return;
    case 0x0B:                                  /* CGA palette select: ignored */
    case 0xFE:
    case 0xFF:                                  /* DESQview probes: not present */
        return;
    case 0x0F:
        r->ax = (u16)((tc_ld_u16(TC_MKFP(0x40, 0x4A)) << 8) | tc_ld_u8(TC_MKFP(0x40, 0x49)));
        r->bx = (u16)((video.page << 8) | (r->bx & 0xFFu));
        return;
    case 0x10:
        palette(r, al);
        return;
    case 0x1A:
        r->ax = 0;                              /* the oracle's answer: no display combination code */
        return;
    default:
        machine_unimplemented("INT 10h (text and EGA services: the port is MCGA)", r);
    }
}
