/* E17A6.c: src/E17A6.ASM in C (playbook 80, M3): the graphics support segment. The two map-cell
   helpers (DirectionFromPoints, Distance) and the neighbour search, the wall plane tables
   (SetPlaneTables), the tile blitter DrawTile, the EGA nibble expander, the three sprite unpackers,
   the CGA mask builder and the icon translator.

   Written from the TASM source, which stays the reference: each routine follows its labels, with
   16-bit register variables named as the source names them. All the unit's tables and scratch stay
   in the arena in the unit's code segment, where the original keeps them, and are read there at
   each place the source reads them (CS_VAR): the three 256-byte tables (17A6:0000, 0100, 0200), the
   saved far pointer and EGA port shadows (0300..0313), the plane tables (03AF, 03BF), DrawTile's
   scratch words (03CF..03E6), the sprite scratch (0FEF..0FF9), the CGA table (10CD), the EGA sprite
   records, their pointers and table (1352..1714) and BuildMask's two table offsets (18E6, 18E8).
   The DGROUP variables are read through the caller's DS (tc_ds), as the source reads them with DS as
   it comes; DrawTile's own `mov cx,212Eh` and `mov ax,212Eh` are relocated at load (DGROUP).

   The near helpers with register conventions (_BlitMaskedByte, _BlitMaskedWord and the three sprite
   rows) are static functions over the registers they read and write. DrawTile needs no register
   record: it takes its far pointer on the stack and saves every register it uses, BP included.

   What the MCGA game reaches (gGraphicsChoice 3, playbook decision 4): the two cell helpers,
   SetPlaneTables, DrawTile's MCGA branch (L0ADA), BlitSpriteDefault and TranslateImage's mode 3.
   The rest serves CGA (DrawTile L063E, BlitSpriteCga, BuildMask, TranslateImage mode 0), Tandy
   (DrawTile L0856, TranslateImage mode 1) or EGA (BlitSpriteEga, ExpandNibbles, TranslateImage mode
   2); all of it works on conventional memory only, so it is written here and proven on the bench
   (tests/port_asm/e17a6.py). The one exception is DrawTile's EGA branch (L0446 to L063E, with
   _SetPlaneAndWrite at 17A6:0EEE): it draws through the EGA's sequencer, graphics controller and
   latches into the planes at A200h and reads the tile planes at A400h and A600h, which the port does
   not model, so it stops (tc_fail 74). */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x17A6u
#define CSEG ((u16)(tc_load_seg + SEG))                 /* the value `push cs` gives */
#define DGROUP ((u16)(tc_load_seg + 0x212Eu))           /* the value `mov ax,212Eh` gives */
#define CB(off) tc_ld_u8(CS_VAR(SEG, (u16)(off)))
#define CW(off) tc_ld_u16(CS_VAR(SEG, (u16)(off)))
#define SCB(off, v) tc_st_u8(CS_VAR(SEG, (u16)(off)), (u8)(v))
#define SCW(off, v) tc_st_u16(CS_VAR(SEG, (u16)(off)), (u16)(v))
#define LD8(seg, off) tc_ld_u8(TC_MKFP((seg), (u16)(off)))
#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (u16)(off)))
#define ST8(seg, off, v) tc_st_u8(TC_MKFP((seg), (u16)(off)), (u8)(v))
#define MOVSB(ds, si, es, di) x86_stosb((es), (di), x86_lodsb((ds), (si)))
#define MOVSW(ds, si, es, di) x86_stosw((es), (di), x86_lodsw((ds), (si)))

/* DGROUP variables (src/E17A6.ASM:25-28) */
#define gDirectionFromMask 0x7FBCu
#define gNeighbourOffsets 0x7FCCu
#define gAheadOffsets2And3 0x7FDCu
#define gRowOffsetTableOff 0x80CEu
#define gRowOffsetTableSeg 0x80D0u
#define gGraphicsChoice 0x8102u
#define gArtPage 0xEFC2u

/* the unit's code-segment tables and scratch */
#define gBitRevPairTable 0x0100u
#define gNibbleSwapTable 0x0200u
#define gPlaneTableA 0x03AFu
#define gPlaneTableB 0x03BFu
#define gPlaneSelect 0x03CFu
#define gSrcPlaneSelect 0x03D1u
#define gTileRows 0x03D3u
#define gTileRows2 0x03D5u
#define gSrcSeg 0x03DBu
#define gDstSeg 0x03DDu
#define gSrcOff 0x03DFu
#define gSrcSeg2 0x03E1u
#define gFlipPlanes 0x03E3u
#define gSecondPlane 0x03E5u
#define gSpriteRows 0x0FEFu
#define gSpriteWidth 0x0FF0u
#define gSpriteStride 0x0FF2u
#define gSpriteGroups 0x0FF4u
#define gSpriteRemainder 0x0FF6u
#define gSpriteMode 0x0FF8u
#define gTable10CD 0x10CDu
#define gSpriteHeight 0x1352u
#define gSpriteDst 0x1353u
#define gSpritePlaneUsed 0x1355u
#define gSpritePlaneIndex 0x1365u
#define gSpritePlanePtrs 0x15F5u
#define gTable1615 0x1615u
#define gMaskTableOff 0x18E6u
#define gMaskTableOff2 0x18E8u

/* xlatb: AL from DS:[BX + AL] */
static u8 xlat(u16 ds, u16 bx, u8 al) { return LD8(ds, bx + al); }

/* `div dl` on AX: AL the quotient, AH the remainder. QUIRK 17A6:0322 and 032B: a quotient above FFh
   raises INT 0 (a cell of 2000h or more; the game's cells are 10 bits). The oracle's CPU returns
   from INT 0 to the div itself, so it is tried again with what the handler leaves; Turbo C's handler
   ends the program. The handler gets AX, DX and DS; the C has no other registers of the caller. */
static u16 div_dl(u16 ax, u8 dl)
{
    for (;;) {
        tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        if (ax / dl <= 0xFFu) {
            return (u16)(((ax % dl) << 8) | (ax / dl));
        }
        r.ax = ax;
        r.dx = dl;
        r.ds = tc_ds;
        x86_int(0, &r);
        ax = r.ax;
        dl = (u8)r.dx;
    }
}

/* 17A6:0314: the direction 0..7 (FFh none) from cell `point` to cell `cell`, through the table at
   ds:7FBC */
/*@ 17A6:0314 */ s16 DirectionFromPoints(s16 point, s16 cell)
{
    u16 ax, bx, cx, dx, di, si;

    ax = div_dl((u16)point, 0x20u);             /* AL = y, AH = x */
    bx = ax;
    ax = div_dl((u16)cell, 0x20u);
    cx = (u16)(ax >> 8);                        /* cl = ah */
    dx = (u16)(ax & 0xFFu);                     /* dl = al */
    ax = (u16)(bx >> 8);                        /* al = bh */
    bx &= 0x00FFu;
    si = 0;
    bx = (u16)(bx - dx);
    di = bx;
    if (bx & 0x8000u) {
        di = (u16)(0u - di);
    }
    cx = (u16)(cx - ax);                        /* L0343 */
    ax = cx;
    if (cx & 0x8000u) {
        ax = (u16)(0u - ax);
    }
    dx = (u16)(bx << 1);                        /* L034B */
    if (!((s16)dx < (s16)ax)) {
        si |= 8u;
    }
    dx = (u16)(0u - dx);                        /* L0356 */
    if (!((s16)dx < (s16)ax)) {
        si |= 4u;
    }
    dx = (u16)(cx << 1);                        /* L035F */
    if (!((s16)dx < (s16)di)) {
        si |= 2u;
    }
    dx = (u16)(0u - dx);                        /* L036A */
    if (!((s16)dx < (s16)di)) {
        si++;
    }
    return (s16)LD8(tc_ds, gDirectionFromMask + si);   /* L0371 */
}

/* 17A6:037E: the two 16-byte plane tables into CS:03AF and CS:03BF */
/*@ 17A6:037E */ void SetPlaneTables(farptr t1, farptr t2)
{
    u16 si, di, cx;

    si = TC_OFF(t1);
    di = gPlaneTableA;
    cx = 8;
    x86_rep_movsw(TC_SEG(t1), &si, CSEG, &di, &cx);
    si = TC_OFF(t2);
    di = gPlaneTableB;
    cx = 8;
    x86_rep_movsw(TC_SEG(t2), &si, CSEG, &di, &cx);
}

/* ---- DrawTile's helpers */

/* 17A6:0D20 _BlitMaskedByte (the Tandy branch): four bytes, the destination's at gDstSeg:BX and the
   source's at gSrcSeg:SI, each read backwards through the table at CS:0200 when its plane flag is 1;
   a source nibble that is set replaces the destination's. BX comes back as DX ends (BX + 4). QUIRK
   17A6:0E39: the fourth source byte of plane 1 is indexed with BL only (`mov bl,[si]` with no
   `xor bx,bx`), so with the destination's plane 0 BH is the destination offset's high byte and the
   byte read is CS:(BX + 200h), past the table; kept. */
static void blit_masked_byte(u16 *si, u16 *bx, u16 es, u16 *di)
{
    u16 dx = *bx, b = *bx, ds;
    u8 ah, al;
    int k;

    for (k = 3; k >= 0; k--) {                  /* the bytes at +3, +2, +1, +0 when mirrored */
        ds = CW(gDstSeg);
        if (CW(gSrcPlaneSelect) == 1) {
            ah = LD8(ds, b + (u16)k);
            if (k == 0) {
                dx = (u16)(dx + 4u);
            }
            b = ah;
            ah = CB(b + gNibbleSwapTable);
        } else {                                /* L0D3E, L0D89, L0DD4, L0E21 */
            ah = LD8(ds, b);
            dx++;
        }
        ds = CW(gSrcSeg);                       /* L0D41, L0D8C, L0DD7, L0E24 */
        if (CW(gPlaneSelect) == 1) {
            if (k != 0) {
                b = LD8(ds, *si + (u16)k);
            } else {
                b = (u16)((b & 0xFF00u) | LD8(ds, *si));
                *si = (u16)(*si + 4u);
            }
            al = CB(b + gNibbleSwapTable);
        } else {
            al = x86_lodsb(ds, si);
        }
        if (al & 0xF0u) {                       /* L0D5B */
            ah &= 0x0Fu;
        }
        if (al & 0x0Fu) {
            ah &= 0xF0u;
        }
        al |= ah;
        x86_stosb(es, di, al);
        b = dx;
    }
    *bx = b;
}

/* 17A6:0E53 _BlitMaskedWord (the CGA branch): two bytes, each a source word's low byte with its
   high byte masked by a destination byte, the words read swapped through the table at CS:0100 when
   their plane flag is 1. Leaves the last mask in CH. */
static void blit_masked_word(u16 *si, u16 *bx, u16 es, u16 *di, u16 *cx)
{
    u16 ds, ax;
    u8 al, ah, ch;

    ds = CW(gSrcSeg);
    if (CW(gPlaneSelect) == 1) {
        *si = (u16)(*si + 2u);
        ax = x86_lodsw(ds, si);
        *si = (u16)(*si - 4u);
        ah = CB(gBitRevPairTable + (ax >> 8));
        al = CB(gBitRevPairTable + (ax & 0xFFu));
    } else {                                    /* L0E7B */
        ax = x86_lodsw(ds, si);
        ah = (u8)(ax >> 8);
        al = (u8)ax;
    }
    ds = CW(gDstSeg);                           /* L0E7C */
    if (CW(gSrcPlaneSelect) == 1) {
        ch = LD8(ds, *bx + 2u);
        ch = CB(gBitRevPairTable + ch);
    } else {                                    /* L0E99 */
        ch = LD8(ds, *bx);
        *bx = (u16)(*bx + 2u);
    }
    ah &= ch;                                   /* L0E9E */
    al |= ah;
    x86_stosb(es, di, al);
    if (CW(gSrcPlaneSelect) == 1) {
        ch = LD8(ds, *bx);
        *bx = (u16)(*bx + 4u);
        ch = CB(gBitRevPairTable + ch);
    } else {                                    /* L0EBD */
        ch = LD8(ds, *bx);
        *bx = (u16)(*bx + 2u);
    }
    ds = CW(gSrcSeg);                           /* L0EC2 */
    if (CW(gPlaneSelect) == 1) {
        ax = x86_lodsw(ds, si);
        *si = (u16)(*si + 2u);
        ah = CB(gBitRevPairTable + (ax >> 8));
        al = CB(gBitRevPairTable + (ax & 0xFFu));
    } else {                                    /* L0EE7 */
        ax = x86_lodsw(ds, si);
        ah = (u8)(ax >> 8);
        al = (u8)ax;
    }
    ah &= ch;                                   /* L0EE8 */
    al |= ah;
    x86_stosb(es, di, al);
    *cx = (u16)((*cx & 0x00FFu) | ((u16)ch << 8));
}

/* the destination steps between the eight rows of a CGA and of a Tandy tile (the interleaved
   banks of 2000h) */
static const u16 cga_step[8] = {0x1FFEu, 0xE04Eu, 0x1FFEu, 0xE04Eu, 0x1FFEu, 0xE04Eu, 0x1FFEu, 0xDF10u};
static const u16 tandy_step[8] = {0x1FFCu, 0x1FFCu, 0x1FFCu, 0xA09Cu, 0x1FFCu, 0x1FFCu, 0x1FFCu, 0x9F60u};

/* the tile's word in the map at DS:BX (BX steps by 2) and its second layer at DS:BX+294h: bit 15 a
   tile drawn over another, bit 14 the mirrored plane, the rest a paragraph count from the source */

/* DrawTile's MCGA branch, one tile of 8 by 8 pixels (L0B37..L0C0A): each source byte two pixels,
   its nibbles through plane table A (gFlipPlanes 1) or B; mirrored (gPlaneSelect 1) each row of four
   bytes is read backwards with the nibbles swapped */
static void mcga_tile(u16 ds, u16 *si, u16 es, u16 *di)
{
    u16 bx = 0, dx = 8, cx;
    u8 al, ah;
    int mirrored = CW(gPlaneSelect) == 1;

    do {                                        /* L0B56, L0BB0 */
        cx = 4;
        if (mirrored) {
            *si = (u16)(*si + 3u);
        }
        do {                                    /* L0B59, L0BB6 */
            bx = (u16)((bx & 0xFF00u) | LD8(ds, *si));
            if (mirrored) {
                (*si)--;
                ah = (u8)((bx & 0xF0u) >> 4);
                al = (u8)(bx & 0x0Fu);
                bx = (u16)((bx & 0xFF00u) | ah);
            } else {
                (*si)++;
                al = (u8)((bx & 0xF0u) >> 4);
                ah = (u8)(bx & 0x0Fu);
                bx = (u16)((bx & 0xFF00u) | ah);
            }
            ah = CB(bx + (CW(gFlipPlanes) == 1 ? gPlaneTableA : gPlaneTableB));
            bx = (u16)((bx & 0xFF00u) | al);
            al = CB(bx + (CW(gFlipPlanes) == 1 ? gPlaneTableA : gPlaneTableB));
            x86_stosw(es, di, (u16)(((u16)ah << 8) | al));
        } while (--cx != 0);
        if (mirrored) {
            *si = (u16)(*si + 5u);
        }
        *di = (u16)(*di + 0x138u);
    } while (--dx != 0);
    *di = (u16)(*di - 0x9F8u);
}

/* the second tile drawn over the first (L0C32..L0CBB): plane table B, pixel 0 transparent */
static void mcga_tile_over(u16 ds, u16 *si, u16 es, u16 *di, int mirrored)
{
    u16 bx = 0, dx = 8, cx;
    u8 al, ah, b;

    do {                                        /* L0C6B, L0CC1 */
        cx = 4;
        if (mirrored) {
            *si = (u16)(*si + 3u);
        }
        do {                                    /* L0C6E, L0CC7 */
            b = LD8(ds, *si);
            bx = (u16)((bx & 0xFF00u) | b);
            if (mirrored) {
                (*si)--;
                al = (u8)((b & 0xF0u) >> 4);
                ah = (u8)(b & 0x0Fu);
            } else {
                (*si)++;
                ah = (u8)((b & 0xF0u) >> 4);
                al = (u8)(b & 0x0Fu);
            }
            bx = (u16)((bx & 0xFF00u) | ah);
            ah = CB(bx + gPlaneTableB);
            bx = (u16)((bx & 0xFF00u) | al);
            al = CB(bx + gPlaneTableB);
            if (ah != 0) {
                ST8(es, *di, ah);
            }
            (*di)++;                            /* L0C98, L0CF1 */
            if (al != 0) {
                ST8(es, *di, al);
            }
            (*di)++;                            /* L0CA0, L0CF9 */
            bx = 0;
        } while (--cx != 0);
        if (mirrored) {
            *si = (u16)(*si + 5u);
        }
        *di = (u16)(*di + 0x138u);
    } while (--dx != 0);
    *di = (u16)(*di - 0x9F8u);
}

/* 17A6:03E7: the 22 by 15 tiles of the map at ds:ADB0 from the far pointer `src` into the screen
   buffer (gArtPage), by gGraphicsChoice: 2 EGA, 0 CGA, 1 Tandy, any other MCGA/VGA */
/*@ 17A6:03E7 */ void DrawTile(farptr src)
{
    u16 si, dx, bx, cx, bp, di, ax, es, ds, keep_si, keep_bx;
    int k;

    si = TC_OFF(src);
    dx = TC_SEG(src);
    SCW(gSrcOff, si);
    SCW(gSrcSeg2, dx);
    bx = 0xADB0u;
    cx = DGROUP;
    bp = cx;
    if (LD16(tc_ds, gGraphicsChoice) == 2) {
        ax = 0xA200u;                           /* L0419 */
        di = 0;
    } else {
        di = LD16(tc_ds, gArtPage);
        ax = LD16(tc_ds, gArtPage + 2u);
    }
    es = ax;                                    /* L041F */
    SCW(gTileRows, 0x0F);
    if (LD16(tc_ds, gGraphicsChoice) == 2) {
        tc_fail(74, "DrawTile 17A6:0446: the EGA branch (gGraphicsChoice 2) draws through the EGA's "
                    "sequencer, graphics controller and latches into the planes at A200h, which the "
                    "port (MCGA only, playbook 80 decision 4) does not model");
    }
    if (LD16(tc_ds, gGraphicsChoice) == 0) {
        do {                                    /* L063E: CGA */
            cx = 0x16;
            do {                                /* L0641 */
                SCW(gPlaneSelect, 0);
                ds = bp;
                ax = LD16(ds, bx);
                if ((ax >> 8) >= 0x80u) {
                    ax = (u16)(ax - 0x8000u);   /* L07C7: a tile drawn masked over another */
                    if (ax & 0x4000u) {
                        ax &= 0x3FFFu;
                        SCW(gPlaneSelect, 1);
                    }
                    ax = (u16)((ax << 1) + CW(gSrcSeg2));   /* L07D6 */
                    SCW(gSrcSeg, ax);
                    si = CW(gSrcOff);
                    keep_si = si;
                    SCW(gSrcPlaneSelect, 0);
                    ax = LD16(ds, bx + 0x294u);
                    if (ax & 0x4000u) {
                        ax &= 0x3FFFu;
                        SCW(gSrcPlaneSelect, 1);
                    }
                    bx = (u16)(bx + 2u);        /* L0801 */
                    keep_bx = bx;
                    bx = CW(gSrcOff);
                    ax = (u16)((ax << 1) + CW(gSrcSeg2));
                    SCW(gDstSeg, ax);
                    for (k = 0; k < 8; k++) {
                        blit_masked_word(&si, &bx, es, &di, &cx);
                        di = (u16)(di + cga_step[k]);
                    }
                    bx = keep_bx;
                    si = keep_si;
                    cx &= 0x00FFu;
                } else {
                    if (ax & 0x4000u) {         /* L0657 */
                        SCW(gPlaneSelect, 1);
                        ax &= 0x3FFFu;
                    }
                    if (ax == 0) {              /* L0666 */
                        ax = LD16(ds, bx + 0x294u);
                        if (ax & 0x4000u) {
                            SCW(gPlaneSelect, 1);
                            ax &= 0x3FFFu;
                        }
                    }
                    bx = (u16)(bx + 2u);        /* L067E */
                    keep_si = si;
                    ax = (u16)((ax << 1) + dx);
                    ds = ax;
                    if (CW(gPlaneSelect) == 1) {
                        si = (u16)(si + 2u);    /* L06D6 */
                        for (k = 0; k < 8; k++) {
                            x86_stosb(es, &di, CB(gBitRevPairTable + LD8(ds, si)));
                            si = (u16)(si - 2u);
                            x86_stosb(es, &di, CB(gBitRevPairTable + LD8(ds, si)));
                            if (k != 7) {
                                si = (u16)(si + 6u);
                            }
                            di = (u16)(di + cga_step[k]);
                        }
                    } else {
                        for (k = 0; k < 8; k++) {   /* L0692 */
                            MOVSB(ds, &si, es, &di);
                            si++;
                            MOVSB(ds, &si, es, &di);
                            si++;
                            di = (u16)(di + cga_step[k]);
                        }
                    }
                    si = keep_si;
                }
            } while (--cx != 0);                /* L07AB */
            di = (u16)(di + 0x114u);            /* L07B1 */
            SCW(gTileRows, CW(gTileRows) - 1u);
        } while (CW(gTileRows) != 0);
        return;
    }
    if (LD16(tc_ds, gGraphicsChoice) == 1) {
        do {                                    /* L0856: Tandy */
            cx = 0x16;
            do {                                /* L0859 */
                ds = bp;
                ax = LD16(ds, bx);
                SCW(gPlaneSelect, 0);
                if (ax >= 0x8000u) {
                    ax = (u16)(ax - 0x8000u);   /* L0A4F: a tile drawn masked over another */
                    if (ax & 0x4000u) {
                        ax &= 0x3FFFu;
                        SCW(gPlaneSelect, 1);
                    }
                    ax = (u16)((ax << 1) + CW(gSrcSeg2));   /* L0A5E */
                    SCW(gSrcSeg, ax);
                    si = CW(gSrcOff);
                    keep_si = si;
                    SCW(gSrcPlaneSelect, 0);
                    ax = LD16(ds, bx + 0x294u);
                    if (ax & 0x4000u) {
                        ax &= 0x3FFFu;
                        SCW(gSrcPlaneSelect, 1);
                    }
                    bx = (u16)(bx + 2u);        /* L0A89 */
                    keep_bx = bx;
                    bx = CW(gSrcOff);
                    ax = (u16)((ax << 1) + CW(gSrcSeg2));
                    SCW(gDstSeg, ax);
                    for (k = 0; k < 8; k++) {
                        blit_masked_byte(&si, &bx, es, &di);
                        di = (u16)(di + tandy_step[k]);
                    }
                    bx = keep_bx;
                    si = keep_si;
                } else {
                    if (ax & 0x4000u) {         /* L086F */
                        SCW(gPlaneSelect, 1);
                        ax &= 0x3FFFu;
                    }
                    if (ax == 0) {              /* L087E */
                        ax = LD16(ds, bx + 0x294u);
                        if (ax & 0x4000u) {
                            SCW(gPlaneSelect, 1);
                            ax &= 0x3FFFu;
                        }
                    }
                    bx = (u16)(bx + 2u);        /* L0896 */
                    keep_si = si;
                    ax = (u16)((ax << 1) + dx);
                    ds = ax;
                    if (CW(gPlaneSelect) == 1) {
                        for (k = 0; k < 8; k++) {   /* L08DE */
                            x86_stosb(es, &di, CB(gNibbleSwapTable + LD8(ds, si + 3u)));
                            x86_stosb(es, &di, CB(gNibbleSwapTable + LD8(ds, si + 2u)));
                            x86_stosb(es, &di, CB(gNibbleSwapTable + LD8(ds, si + 1u)));
                            x86_stosb(es, &di, CB(gNibbleSwapTable + LD8(ds, si)));
                            si = (u16)(si + 4u);
                            di = (u16)(di + tandy_step[k]);
                        }
                    } else {
                        for (k = 0; k < 8; k++) {   /* L08AA */
                            MOVSW(ds, &si, es, &di);
                            MOVSW(ds, &si, es, &di);
                            di = (u16)(di + tandy_step[k]);
                        }
                    }
                    si = keep_si;
                }
            } while (--cx != 0);                /* L0A33 */
            di = (u16)(di + 0xE8u);             /* L0A39 */
            SCW(gTileRows, CW(gTileRows) - 1u);
        } while (CW(gTileRows) != 0);
        return;
    }
    do {                                        /* L0ADA: MCGA, VGA and any other value */
        SCW(gTileRows2, 0x16);
        do {                                    /* L0AE1 */
            SCW(gFlipPlanes, 0);
            ds = bp;
            ax = LD16(ds, bx);
            SCW(gPlaneSelect, 0);
            SCW(gSecondPlane, 0);
            if (ax >= 0x8000u) {
                ax = (u16)(ax - 0x8000u);
                SCW(gSecondPlane, ax);
                ax = 0;
            }
            if (ax & 0x4000u) {                 /* L0B09 */
                SCW(gPlaneSelect, 1);
                ax &= 0x3FFFu;
            }
            if (ax == 0) {                      /* L0B18 */
                SCW(gFlipPlanes, 1);
                ax = LD16(ds, bx + 0x294u);
                if (ax & 0x4000u) {
                    SCW(gPlaneSelect, 1);
                    ax &= 0x3FFFu;
                }
            }
            bx = (u16)(bx + 2u);                /* L0B37 */
            ax = (u16)((ax << 1) + CW(gSrcSeg2));
            ds = ax;
            si = CW(gSrcOff);
            mcga_tile(ds, &si, es, &di);
            while (CW(gSecondPlane) != 0) {     /* L0C0A */
                di = (u16)(di - 8u);            /* L0C32 */
                SCW(gPlaneSelect, 0);
                ax = CW(gSecondPlane);
                if (ax & 0x4000u) {
                    ax &= 0x3FFFu;
                    SCW(gPlaneSelect, 1);
                }
                ax = (u16)((ax << 1) + CW(gSrcSeg2));   /* L0C4F */
                ds = ax;
                si = CW(gSrcOff);
                mcga_tile_over(ds, &si, es, &di, CW(gPlaneSelect) == 1);
                SCW(gSecondPlane, 0);
            }
            SCW(gTileRows2, CW(gTileRows2) - 1u);
        } while (CW(gTileRows2) != 0);
        di = (u16)(di + 0x950u);                /* L0C1C */
        SCW(gTileRows, CW(gTileRows) - 1u);
    } while (CW(gTileRows) != 0);
}

/* 17A6:0F1F: 1F40h groups of four bytes from `src` to `dst`, each group backwards, each byte's
   nibbles swapped through the table at CS:0200 */
/*@ 17A6:0F1F */ void ExpandNibbles(farptr src, farptr dst)
{
    u16 si = TC_OFF(src), ds = TC_SEG(src), di = TC_OFF(dst), es = TC_SEG(dst), cx = 0x1F40u, dx;

    do {                                        /* L0F3E */
        si = (u16)(si + 3u);
        dx = 4;
        do {                                    /* L0F44 */
            ST8(es, di, CB(gNibbleSwapTable + LD8(ds, si)));
            di++;
            si--;
        } while (--dx != 0);
        si = (u16)(si + 5u);
    } while (--cx != 0);
}

/* 17A6:0F61: the larger of the two cell differences plus half the smaller */
/*@ 17A6:0F61 */ s16 Distance(s16 x, s16 y)
{
    u16 ax = (u16)x, bx = (u16)y;
    u8 al, ah, dl, dh, t;

    dl = (u8)(ax & 0x1Fu);
    ax = (u16)(ax << 3);
    dh = (u8)(ax >> 8);
    al = (u8)(bx & 0x1Fu);
    bx = (u16)(bx << 3);
    ah = (u8)(bx >> 8);
    ah = (u8)(ah - dh);
    if (ah & 0x80u) {
        ah = (u8)(0u - ah);
    }
    al = (u8)(al - dl);                         /* L0F86 */
    if (al & 0x80u) {
        al = (u8)(0u - al);
    }
    if (!((s8)al < (s8)ah)) {                   /* L0F8C */
        t = al;
        al = ah;
        ah = t;
    }
    al = (u8)((al >> 1) + ah);                  /* L0F92 */
    return (s16)al;
}

/* 17A6:0F9D: the neighbour offset of the 8 at ds:7FCC, then of a pair of the 4 at ds:7FDC, that
   maps `from` to `to` modulo 400h; FFFFh if none. No caller in this build (src/E17A6.ASM). */
/*@ 17A6:0F9D */ s16 FindNeighbour(s16 from, s16 to, s16 pair)
{
    u16 si = (u16)from, di = (u16)to, cx = 8, bx = 0, ax;

    do {                                        /* L0FB6 */
        ax = (u16)((si + LD16(tc_ds, gNeighbourOffsets + bx)) & 0x3FFu);
        if (ax == di) {
            return (s16)ax;
        }
        bx = (u16)(bx + 2u);
    } while (--cx != 0);
    cx = 2;
    bx = (u16)((u16)pair << 2);
    do {                                        /* L0FD1 */
        ax = (u16)((si + LD16(tc_ds, gAheadOffsets2And3 + bx)) & 0x3FFu);
        if (ax == di) {
            return (s16)ax;
        }
        bx = (u16)(bx + 2u);
    } while (--cx != 0);
    return (s16)0xFFFFu;
}

/* ---- the sprite unpackers. A sprite: a byte copied, the row count, the width, a byte skipped;
   the output header: the byte, two thirds of the rows, two thirds of the width rounded up, the
   rows' two thirds again (every third source row is dropped) */

/* 17A6:109A _BlitSpriteRowDefault: CS:0FF4 groups of three source bytes to two, then the remainder */
static void row_default(u16 ds, u16 *si, u16 es, u16 *di)
{
    u16 bx = CW(gSpriteGroups), ax;
    u8 al, ah;

    do {                                        /* L10A1 */
        MOVSB(ds, si, es, di);
        ax = x86_lodsw(ds, si);
        ax = (u16)((ax << 8) | (ax >> 8));      /* xchg ah,al */
        ax = (u16)(ax >> 4);
        x86_stosb(es, di, (u8)ax);
    } while (--bx != 0);
    if (CW(gSpriteRemainder) == 1) {
        MOVSB(ds, si, es, di);
        x86_stosb(es, di, 0);
        return;
    }
    if (CW(gSpriteRemainder) == 2) {            /* L10B8 */
        ax = x86_lodsw(ds, si);
        al = (u8)(ax & 0xF0u);
        ah = (u8)((ax >> 8) >> 4);
        al |= ah;
        x86_stosb(es, di, al);
        x86_stosw(es, di, 0);
        x86_stosb(es, di, 0);
    }
}

/* 17A6:0FFA: a sprite for the Tandy and MCGA modes (the 16-byte palette copied but for Tandy) */
/*@ 17A6:0FFA */ void BlitSpriteDefault(farptr src, farptr dst)
{
    u16 si = TC_OFF(src), ds = TC_SEG(src), di = TC_OFF(dst), es = TC_SEG(dst), ax, cx;
    u8 cl;

    SCW(gSpriteMode, LD16(tc_ds, gGraphicsChoice));
    MOVSB(ds, &si, es, &di);
    ax = (u8)(x86_lodsb(ds, &si) + 1u);         /* inc al: AH stays 0 */
    SCB(gSpriteRows, ax);
    ax = (u16)(ax << 1);
    ax = (u16)(ax / 3u);
    cl = (u8)ax;
    x86_stosb(es, &di, (u8)ax);
    ax = x86_lodsb(ds, &si);
    SCW(gSpriteWidth, ax);
    ax = (u16)(ax << 1);
    SCW(gSpriteStride, ax);
    SCW(gSpriteStride, CW(gSpriteStride) << 1);
    if ((ax % 3u) & 0xFFu) {
        ax = (u16)(ax / 3u + 1u);
    } else {
        ax = (u16)(ax / 3u);
    }
    x86_stosb(es, &di, (u8)ax);                 /* L104B */
    x86_stosb(es, &di, cl);
    si++;
    if (CW(gSpriteMode) != 1) {
        cx = 0x10;
        x86_rep_movsb(ds, &si, es, &di, &cx);
    }
    ax = CW(gSpriteStride);                     /* L105D */
    SCW(gSpriteGroups, ax / 3u);
    SCW(gSpriteRemainder, ax % 3u);
    for (;;) {                                  /* L1071 */
        row_default(ds, &si, es, &di);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        row_default(ds, &si, es, &di);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        si = (u16)(si + CW(gSpriteStride));
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
    }
}

/* 17A6:128B _BlitSpriteRowCga and 17A6:12EC _BlitSpriteRowCga2 (mask 1: the pad bits set): a row
   of the CGA sprite, skipped once DI reaches BP; groups of three source bytes to two through the
   table at CS:10CD */
static void row_cga(u16 ds, u16 *si, u16 es, u16 *di, u16 bp, int mask)
{
    u16 cx, bx, w;
    u8 al, ah;

    if (!(*di < bp)) {
        *si = (u16)(*si + CW(gSpriteStride));
        return;
    }
    cx = CW(gSpriteGroups);                     /* L1295, L12F6 */
    bx = gTable10CD;
    do {                                        /* L129D, L12FE */
        al = x86_lodsb(ds, si);
        al = CB(bx + al);                       /* xlatb cs:[bx] */
        ah = al;
        al = x86_lodsb(ds, si);
        w = (u16)((((u16)ah << 8) | al) << 2);  /* shl al,1; rcl ah,1 twice */
        al = (u8)(w >> 8);                      /* xchg ah,al */
        ah = (u8)w;
        x86_stosb(es, di, al);
        ah = (u8)(ah << 2);
        al = x86_lodsb(ds, si);
        al = (u8)((al >> 2) & 0x0Fu);
        al |= ah;
        x86_stosb(es, di, al);
    } while (--cx != 0);
    if (CW(gSpriteRemainder) == 1) {
        al = x86_lodsb(ds, si);
        al = CB(bx + al);
        al = (u8)(al << 2);
        if (mask) {
            al |= 3u;
        }
        x86_stosb(es, di, al);
        x86_stosb(es, di, mask ? 0xFFu : 0u);
        return;
    }
    if (CW(gSpriteRemainder) == 2) {            /* L12D2, L1335 */
        al = x86_lodsb(ds, si);
        al = CB(bx + al);
        ah = al;
        al = x86_lodsb(ds, si);
        w = (u16)((((u16)ah << 8) | al) << 2);
        al = (u8)(w >> 8);
        ah = (u8)w;
        if (mask) {
            ah |= 3u;
        }
        x86_stosw(es, di, (u16)(((u16)ah << 8) | al));
    }
}

/* 17A6:11CD: a sprite for the CGA mode: the rows, then their masks from the source that follows */
/*@ 17A6:11CD */ void BlitSpriteCga(farptr src, farptr dst)
{
    u16 si = TC_OFF(src), ds = TC_SEG(src), di = TC_OFF(dst), es = TC_SEG(dst), ax, bp;
    u8 cl, rows;

    MOVSB(ds, &si, es, &di);
    ax = x86_lodsb(ds, &si);
    SCB(gSpriteRows, ax);
    ax = (u16)(ax << 1);
    ax = (u16)(ax / 3u);
    cl = (u8)ax;
    x86_stosb(es, &di, (u8)ax);
    ax = x86_lodsb(ds, &si);
    SCW(gSpriteWidth, ax);
    ax = (u16)(ax << 1);
    SCW(gSpriteStride, ax);
    if ((ax % 3u) & 0xFFu) {
        ax = (u16)(ax / 3u + 1u);
    } else {
        ax = (u16)(ax / 3u);
    }
    x86_stosb(es, &di, (u8)ax);                 /* L1214 */
    ax = (u16)((u8)ax * cl);                    /* mul cl */
    ax = (u16)(ax << 1);
    ax = (u16)(ax + di);
    ax++;
    bp = ax;
    x86_stosb(es, &di, cl);
    si++;
    ax = CW(gSpriteStride);
    SCW(gSpriteGroups, ax / 3u);
    SCW(gSpriteRemainder, ax % 3u);
    rows = CB(gSpriteRows);
    for (;;) {                                  /* L123B */
        row_cga(ds, &si, es, &di, bp, 0);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        row_cga(ds, &si, es, &di, bp, 0);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        si = (u16)(si + CW(gSpriteStride));
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
    }
    SCB(gSpriteRows, rows);                     /* L125B */
    bp = (u16)(bp << 1);
    for (;;) {                                  /* L1262 */
        row_cga(ds, &si, es, &di, bp, 1);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        row_cga(ds, &si, es, &di, bp, 1);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        si = (u16)(si + CW(gSpriteStride));
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
    }
}

/* 17A6:179E _BlitSpriteRowEga: one row of the EGA sprite. The source words (a pixel byte, then
   the plane in the low nibble of the flags, bit 7 the next column, bits 7 and 6 the row's end) are
   sorted into one 40-byte record per plane in the code segment (CS:1375, pointers at CS:15F5),
   each record is packed three bytes to two through the table at CS:1615, and the output is, per
   column, a (pixel, plane) pair for each plane used, bit 7 on the column's last byte and bit 6 on
   the row's. The column count indexes the records unchecked (`mov cs:[bx+di],al`), as the original
   does. */
static void row_ega(u16 ds, u16 *si, u16 es, u16 *di)
{
    u16 bp, cx, dx, bx, ax, d = *di, s, sd, keep_si;
    u8 al, ah, dl, t;
    u16 w;

    bp = gSpritePlaneUsed;
    cx = 0x10;
    do {                                        /* L17A6 */
        SCW(bp, 0);
        bp = (u16)(bp + 2u);
    } while (--cx != 0);
    SCW(gSpriteDst, d);
    dx = 0;
    bx = 0;
    cx = 0;
    for (;;) {                                  /* L17BA */
        ax = x86_lodsw(ds, si);
        al = (u8)ax;
        ah = (u8)(ax >> 8);
        if (al != 0) {
            dl = (u8)(ah & 0x0Fu);
            dx = (u16)((dx & 0xFF00u) | dl);
            bp = dx;
            if (CB(bp + gSpritePlaneUsed) == 0) {
                SCB(bp + gSpritePlaneUsed, CB(bp + gSpritePlaneUsed) + 1u);
                bp = (u16)(bp << 1);
                d = CW(bp + gSpritePlanePtrs);
                bp = cx;
                SCB(bp + gSpritePlaneIndex, dl);
                cx = CW(gSpriteWidth);
                do {                            /* L17E6 */
                    SCB(d, dx >> 8);
                    d++;
                } while (--cx != 0);
                d = (u16)(d - CW(gSpriteWidth));
                cx = bp;
                cx++;
            } else {                            /* L17F6 */
                bp = (u16)(bp << 1);
                d = CW(bp + gSpritePlanePtrs);
            }
            SCB(bx + d, al);                    /* L17FD */
        }
        if (!(ah & 0x80u)) {                    /* L1800 */
            continue;
        }
        bx++;
        if (!(ah & 0x40u)) {
            continue;
        }
        break;
    }
    keep_si = *si;
    dx = 0;
    do {                                        /* L180C */
        bp = dx;
        if (CB(bp + gSpritePlaneUsed) != 0) {
            bp = (u16)(bp << 1);                /* L181E: pack the record in place, DS = ES = CS */
            s = CW(bp + gSpritePlanePtrs);
            sd = s;
            cx = CW(gSpriteGroups);
            bx = gTable1615;
            do {                                /* L1837 */
                ax = x86_lodsw(CSEG, &s);
                al = CB(bx + (u8)ax);
                ah = (u8)(ax >> 8);
                t = al;                         /* xchg ah,al */
                al = ah;
                ah = t;
                al = (u8)(al << 1);
                w = (u16)((((u16)ah << 8) | al) << 2);
                al = (u8)(w >> 8);              /* xchg ah,al */
                ah = (u8)w;
                x86_stosb(CSEG, &sd, al);
                ah = (u8)((ah << 1) & 0xE0u);
                al = x86_lodsb(CSEG, &s);
                al = (u8)(al >> 1);
                al = CB(bx + al);
                al |= ah;
                x86_stosb(CSEG, &sd, al);
            } while (--cx != 0);
            if (CW(gSpriteRemainder) == 1) {
                al = x86_lodsb(CSEG, &s);
                al = (u8)(al >> 1);
                al = CB(bx + al);
                al = (u8)(al << 3);
                x86_stosb(CSEG, &sd, al);
                x86_stosb(CSEG, &sd, 0);
            } else if (CW(gSpriteRemainder) == 2) {     /* L1871 */
                al = x86_lodsb(CSEG, &s);
                al = CB(bx + al);
                ah = al;
                al = x86_lodsb(CSEG, &s);
                al = (u8)(al << 1);
                w = (u16)((((u16)ah << 8) | al) << 2);
                x86_stosw(CSEG, &sd, (u16)(((w & 0xFFu) << 8) | (w >> 8)));
            }
        }
        dx++;                                   /* L1816 */
    } while (dx != 0x10);
    d = CW(gSpriteDst);                         /* L1890 */
    cx = CB(gSpriteHeight);
    bx = 0;
    do {                                        /* L189E */
        dx = 0;
        for (;;) {                              /* L18A0 */
            bp = (u16)(dx & 0x0Fu);
            al = CB(bp + gSpritePlaneIndex);
            if (al == 0) {
                break;
            }
            ax = (u16)((u16)al << 1);
            bp = ax;
            s = CW(bp + gSpritePlanePtrs);
            ah = (u8)ax;
            al = CB(bx + s);
            if (al != 0) {
                x86_stosb(es, &d, al);
                al = (u8)(ah >> 1);
                x86_stosb(es, &d, al);
                dx = (u16)(dx + 0x100u);        /* inc dh */
            }
            dx++;                               /* L18CA */
            if ((dx & 0xFFu) == 0x10u) {
                break;
            }
        }
        if ((dx >> 8) == 0) {                   /* L18D0 */
            x86_stosw(es, &d, 0);
        }
        ST8(es, d - 1u, LD8(es, d - 1u) | 0x80u);   /* L18D7 */
        bx++;
    } while (--cx != 0);
    ST8(es, d - 1u, LD8(es, d - 1u) | 0x40u);
    *si = keep_si;
    *di = d;
}

/* 17A6:1715: a sprite for the EGA mode */
/*@ 17A6:1715 */ void BlitSpriteEga(farptr src, farptr dst)
{
    u16 si = TC_OFF(src), ds = TC_SEG(src), di = TC_OFF(dst), es = TC_SEG(dst), ax;
    u8 cl;

    MOVSB(ds, &si, es, &di);
    ax = x86_lodsb(ds, &si);
    SCB(gSpriteRows, ax);
    ax = (u16)(ax << 1);
    ax = (u16)(ax / 3u);
    cl = (u8)ax;
    x86_stosb(es, &di, (u8)ax);
    ax = x86_lodsb(ds, &si);
    SCW(gSpriteWidth, ax);
    ax = (u16)(ax << 1);
    if ((ax % 3u) & 0xFFu) {
        ax = (u16)(ax / 3u + 1u);
    } else {
        ax = (u16)(ax / 3u);
    }
    x86_stosb(es, &di, (u8)ax);                 /* L1755 */
    SCB(gSpriteHeight, ax);
    x86_stosb(es, &di, cl);
    si++;
    ax = CW(gSpriteWidth);
    SCW(gSpriteGroups, ax / 3u);
    SCW(gSpriteRemainder, ax % 3u);
    for (;;) {                                  /* L176F */
        row_ega(ds, &si, es, &di);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        row_ega(ds, &si, es, &di);
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
        do {                                    /* L1783: a row dropped */
            ax = x86_lodsw(ds, &si);
        } while (!(ax & 0x4000u));
        SCB(gSpriteRows, CB(gSpriteRows) - 1u);
        if (CB(gSpriteRows) == 0) {
            break;
        }
    }
    ST8(es, di - 1u, LD8(es, di - 1u) | 0x20u);     /* L1790 */
}

/* 17A6:18EA: the CGA mask of the 0CB20h-byte image at `p`, in place: each pair of bytes
   translated through one of two tables of the driver (ds:80D0:80CE + 190h, + 290h, alternating
   every two pairs), followed by its mask byte */
/*@ 17A6:18EA */ void BuildMask(farptr p)
{
    u16 si = TC_OFF(p), es = TC_SEG(p), cx = 0x6590u, dx = 0, bx, ds;
    u8 al, ah, dl;

    bx = (u16)(LD16(tc_ds, gRowOffsetTableOff) + 0x190u);
    SCW(gMaskTableOff, bx);
    bx = (u16)(bx + 0x100u);                    /* inc bh */
    SCW(gMaskTableOff2, bx);
    ds = LD16(tc_ds, gRowOffsetTableSeg);
    bx = CW(gMaskTableOff);
    for (;;) {                                  /* L1917, L191C */
        dl = 2;
        for (;;) {                              /* L191E */
            al = xlat(ds, bx, LD8(es, si));
            al = (u8)(al << 4);
            ah = al;
            al = xlat(ds, bx, LD8(es, si + 1u));
            al |= ah;
            ah = LD8(es, si);                   /* xchg al,es:[si] */
            ST8(es, si, al);
            al = ah;
            si++;
            ah = 0;
            if (al & 0xF0u) {
                ah |= 0xC0u;
            }
            if (al & 0x0Fu) {                   /* L1940 */
                ah |= 0x30u;
            }
            al = LD8(es, si);                   /* L1947 */
            if (al & 0xF0u) {
                ah |= 0x0Cu;
            }
            if (al & 0x0Fu) {                   /* L1951 */
                ah |= 0x03u;
            }
            ah = (u8)~ah;                       /* L1958 */
            ST8(es, si, ah);
            si++;
            dl--;
            if (dl == 0) {
                break;
            }
            if (--cx == 0) {                    /* loop L191E */
                return;
            }
        }
        if (--cx == 0) {                        /* L1966 */
            return;
        }
        dx ^= 0x0100u;                          /* xor dh,1 */
        if ((dx >> 8) == 0) {
            bx = CW(gMaskTableOff);             /* L1917 */
        } else {
            bx = CW(gMaskTableOff2);
        }
    }
}

/* 17A6:197E: an icon from `src` to `dst` through the table `xlat_table` by the mode (the graphics
   choice): 0 count * 2 bytes translated and as many copied, 2 a translated stream to a byte with
   20h, 3 the 16-byte palette from the table and count * 2 words copied, any other count * 4 bytes
   translated. The count is the header word's two bytes multiplied. */
/*@ 17A6:197E */ void TranslateImage(farptr src, farptr dst, farptr xlat_table, s16 mode)
{
    u16 si = TC_OFF(src), ds = TC_SEG(src), di = TC_OFF(dst), es = TC_SEG(dst);
    u16 bx = TC_OFF(xlat_table), dx = TC_SEG(xlat_table), ax = (u16)mode, cx, keep;
    u8 al;

    if (ax == 3) {                              /* L19FB */
        MOVSW(ds, &si, es, &di);
        ax = x86_lodsw(ds, &si);
        x86_stosw(es, &di, ax);
        keep = si;                              /* xchg si,bx */
        si = bx;
        cx = 8;
        x86_rep_movsw(dx, &si, es, &di, &cx);
        si = (u16)(keep + 0x10u);
        cx = (u16)((u16)(ax & 0xFFu) * (ax >> 8));  /* mul dx, DX = AH */
        cx = (u16)(cx << 1);
        x86_rep_movsw(ds, &si, es, &di, &cx);
        return;
    }
    if (ax == 0) {
        MOVSW(ds, &si, es, &di);
        ax = x86_lodsw(ds, &si);
        x86_stosw(es, &di, ax);
        cx = (u16)((u16)(ax & 0xFFu) * (ax >> 8));
        cx = (u16)(cx << 1);
        keep = cx;
        do {                                    /* L19BA */
            al = xlat(dx, bx, x86_lodsb(ds, &si));
            x86_stosb(es, &di, al);
        } while (--cx != 0);
        cx = keep;
        x86_rep_movsb(ds, &si, es, &di, &cx);
        return;
    }
    if (ax == 2) {                              /* L19EB */
        MOVSW(ds, &si, es, &di);
        MOVSW(ds, &si, es, &di);
        do {                                    /* L19ED */
            MOVSB(ds, &si, es, &di);
            al = xlat(dx, bx, x86_lodsb(ds, &si));
            x86_stosb(es, &di, al);
        } while (!(al & 0x20u));
        return;
    }
    MOVSW(ds, &si, es, &di);                    /* L19C8 */
    ax = x86_lodsw(ds, &si);
    x86_stosw(es, &di, ax);
    cx = (u16)((u16)(ax & 0xFFu) * (ax >> 8));
    cx = (u16)(cx << 2);
    do {                                        /* L19E0 */
        al = xlat(dx, bx, x86_lodsb(ds, &si));
        x86_stosb(es, &di, al);
    } while (--cx != 0);
}
