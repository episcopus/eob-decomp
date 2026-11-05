/* E1948.c: src/E1948.ASM in C (playbook 80, M3): the PC speaker and Tandy (SN76489) sound driver,
   its INT 8 handler, the song interpreter it runs every tick, and the far routines the game calls
   (install, uninstall, start, stop, resume, silence).

   Written from the TASM source, which stays the reference: each routine follows its labels, with
   16-bit register variables named as the source names them. The driver's variables are in a segment
   the code addresses as 29B8h (relocated at load: tc_load_seg + 29B8h), an alias of DGROUP at 88A0h,
   so the offsets below are the driver segment's (gTandyOn at 1C7h is DGROUP:8A67). The saved BIOS
   INT 8 vector is the handler's own first four bytes, in the code segment at 1948:0006 and 0008
   (CS_VAR). Port I/O is in the source's order and count: PIT 40h, 42h, 43h, port 61h (the speaker
   gate) and C0h (the Tandy chip). */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x1948u
#define OLD_INT8_OFF CS_VAR(SEG, 0x0006u)
#define OLD_INT8_SEG CS_VAR(SEG, 0x0008u)
#define SND_SEG ((u16)(tc_load_seg + 0x29B8u))
#define V(off) TC_MKFP(SND_SEG, (u16)(off))
#define LDB(off) tc_ld_u8(V(off))
#define LDW(off) tc_ld_u16(V(off))
#define STB(off, v) tc_st_u8(V(off), (u8)(v))
#define STW(off, v) tc_st_u16(V(off), (u16)(v))

/* the driver segment's variables (src/E1948.ASM:34-128) */
#define gChainReload     0x0006u
#define gChainCount      0x0008u
#define gSongBase        0x000Au
#define gSongBaseSeg     0x000Cu
#define gVoicePos        0x000Eu
#define gSongPtr         0x0016u
#define gSongPtrSeg      0x0018u
#define gNoteTablePc     0x001Au
#define gNoteTableTandy  0x00DAu
#define gPort61Save      0x019Au
#define gVoiceCount      0x019Bu
#define gPlaying         0x019Du
#define gStartRequest    0x019Eu
#define gStopRequest     0x019Fu
#define gVoiceMute       0x01A0u
#define gVoiceReadNext   0x01A4u
#define gVoiceSilent     0x01A8u
#define gVoiceDuration   0x01ACu
#define gVoiceTandyVol   0x01B4u
#define gVoiceLengthMul  0x01B8u
#define gVoiceRepeat1    0x01BCu
#define gVoiceRepeat2    0x01C0u
#define gVoiceDelay1     0x01C4u
#define gVoiceDelay2     0x01C5u
#define gRunning         0x01C6u
#define gTandyOn         0x01C7u
#define gTickCount       0x01C8u
#define gTickSeconds     0x01CAu

/* [reg+name]: the displacement wraps at 64 KB within the segment, as the CPU's does */
#define AT(name, reg) ((u16)((name) + (reg)))

/* the forward declaration StopSound needs: no C unit calls it, so ported.h has no prototype */
void StopSound(void);

/* 1948:02C7: turn voice SI on. PC speaker: set the gate and data bits of port 61h. Tandy: write the
   voice's volume register with its stored volume XOR 0Fh (the whole byte: a volume above 15 leaves
   its high bits in the command byte). */
/*@ 1948:02C7 */ static void VoiceOn(u16 si)
{
    u8 al, ah;

    if (LDB(gTandyOn) != 1) {
        x86_outb(0x61, (u8)(x86_inb(0x61) | 3u));
        return;
    }
    al = (u8)si;                                /* tandyOn */
    ah = (u8)(LDB(AT(gVoiceTandyVol, si)) ^ 0x0Fu);
    al = (u8)(al << 1);
    al = (u8)(al + 1u);
    al = (u8)(al << 4);
    al = (u8)(al | ah | 0x80u);
    x86_outb(0xC0, al);
}

/* 1948:02F1: turn voice SI off. PC speaker: clear the gate and data bits of port 61h. Tandy: the
   voice's volume register at 0Fh, full attenuation. */
/*@ 1948:02F1 */ static void VoiceOff(u16 si)
{
    u8 al;

    if (LDB(gTandyOn) != 1) {
        x86_outb(0x61, (u8)(x86_inb(0x61) & 0xFCu));
        return;
    }
    al = (u8)si;                                /* tandyOff */
    al = (u8)(al << 1);
    al = (u8)(al + 1u);
    al = (u8)(al << 4);
    al = (u8)(al | 0x0Fu | 0x80u);
    x86_outb(0xC0, al);
}

/* 1948:0316: program voice SI's pitch, AX the table value. PC speaker: the divisor to PIT channel 2,
   low byte then high. Tandy: the tone latch (channel SI, the low nibble) then the data byte (bits 4
   to 11 of AX: DX = AX << 4, DL replaced by the low nibble, DH sent). */
/*@ 1948:0316 */ static void SetVoicePitch(u16 si, u16 ax)
{
    u16 dx;
    u8 al;

    if (LDB(gTandyOn) != 1) {
        x86_outb(0x42, (u8)ax);
        x86_outb(0x42, (u8)(ax >> 8));
        return;
    }
    dx = (u16)(ax << 4);                        /* tandyPitch */
    dx = (u16)((dx & 0xFF00u) | (ax & 0x0Fu));
    al = (u8)((u8)si << 5);
    al = (u8)(al | (u8)dx | 0x80u);
    x86_outb(0xC0, al);
    x86_outb(0xC0, (u8)(dx >> 8));
}

/* 1948:034A: three voices: gVoiceCount 3, each of the four voices flagged silent and turned off
   (voice 3's Tandy write is the noise channel), then bits 5 and 6 of port 61h set. */
/*@ 1948:034A */ static void SetThreeVoices(void)
{
    u16 si;

    STW(gVoiceCount, 3u);
    for (si = 0; si != 4; si++) {               /* threeLoop */
        STB(AT(gVoiceSilent, si), 1u);
        VoiceOff(si);
    }
    x86_outb(0x61, (u8)(x86_inb(0x61) | 0x60u));
}

/* 1948:0367: one voice: gVoiceCount 1 and port 61h from gPort61Save with the gate and data bits
   clear. QUIRK 1948:036D: port 61h is read and the value thrown away; the read is kept (the oracle
   sees every access). */
/*@ 1948:0367 */ static void SetOneVoice(void)
{
    STW(gVoiceCount, 1u);
    (void)x86_inb(0x61);
    x86_outb(0x61, (u8)(LDB(gPort61Save) & 0xFCu));
}

/* div word ptr es:[bx] with DX:AX = dxax: the quotient. QUIRK 1948:01E3 and 01EF: a zero divisor or a
   quotient above FFFFh raises INT 0. On the 80286 and later (and in the oracle's CPU) the INT 0
   frame returns to the div itself, so after the handler the division is tried again with the
   registers and memory the handler leaves; Turbo C's own INT 0 handler never returns (it ends the
   program). Only the registers the div reads are given to the handler; the rest of the frame (the
   pushed registers of _SoundTick) the C does not have. */
static u16 div_es_bx(u32 dxax, u16 es, u16 bx, u16 si, u16 bp)
{
    for (;;) {
        u16 d = tc_ld_u16(TC_MKFP(es, bx));
        tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        if (d != 0 && dxax / d <= 0xFFFFu) {
            return (u16)(dxax / d);
        }
        r.ax = (u16)dxax;
        r.dx = (u16)(dxax >> 16);
        r.bx = bx;
        r.si = si;
        r.bp = bp;
        r.ds = SND_SEG;
        r.es = es;
        x86_int(0, &r);
        dxax = ((u32)r.dx << 16) | r.ax;
        bx = r.bx;
        es = r.es;
    }
}

/* 1948:0050: one tick of the song (see the source's comment): a start request reloads the song and
   restarts voices 0 to 2; then each voice up to gVoiceCount reads its notes and commands from
   gSongBaseSeg:gSongBase + gVoicePos[voice] or counts its note down. Written over the source's
   registers and labels: SI the voice, DI = 2 * SI, BP the voice's position, ES:BX the song byte. */
/*@ 1948:0050 */ static void SoundTick(void)
{
    u16 si, di, bp, bx, es, ax;
    u8 al;

    if (LDB(gStartRequest) != 0) {
        STB(gRunning, 1u);
        STB(gPlaying, 1u);
        STB(gStartRequest, 0u);
        STB(gStopRequest, 0u);
        STW(gSongBase, LDW(gSongPtr));
        STW(gSongBaseSeg, LDW(gSongPtrSeg));
        for (di = 0; di != 3; di++) {           /* startVoice, AL counting 3 down */
            STB(AT(gVoiceReadNext, di), 1u);
            STB(AT(gVoiceMute, di), 0u);
            STB(AT(gVoiceSilent, di), 0u);
            STB(AT(gVoiceLengthMul, di), 1u);
            STW(AT(gVoicePos, di * 2u), 0u);
            STW(AT(gVoiceDuration, di * 2u), 0u);
        }
        STB(gVoiceDelay1, 0x23u);
        STB(gVoiceDelay2, 0x46u);
    }
    if (LDB(gPlaying) == 0) {                   /* checkPlaying */
        return;
    }
    si = 0;                                     /* beginVoices */
voiceLoop:
    di = (u16)(si << 1);
    bp = LDW(AT(gVoicePos, di));
    if (si == 1) {
        if (LDB(gVoiceDelay1) != 0) {
            STB(gVoiceDelay1, LDB(gVoiceDelay1) - 1u);
            goto nextVoice;
        }
    } else if (si == 2) {                       /* notVoice1 */
        if (LDB(gVoiceDelay2) != 0) {
            STB(gVoiceDelay2, LDB(gVoiceDelay2) - 1u);
            goto nextVoice;
        }
    }
    if (LDB(AT(gVoiceReadNext, si)) == 0) {     /* delayDone */
        goto checkRestart;
    }
    es = LDW(gSongBaseSeg);                     /* readFirst */
    bx = (u16)(LDW(gSongBase) + bp);
readLoop:
    al = tc_ld_u8(TC_MKFP(es, bx));
    if (al & 0x80u) {
        goto command;
    }
    if (al > 0x5Fu) {                           /* 60h to 7Fh: muted for good, the byte not consumed */
        goto muteVoice;
    }
    /* playNote: the note's table word (DI saved around it) */
    if (LDB(gTandyOn) == 1) {
        ax = LDW(AT(gNoteTableTandy, (u16)al << 1));
    } else {
        ax = LDW(AT(gNoteTablePc, (u16)al << 1));
    }
afterTandy:
    if (ax != 0) {
        SetVoicePitch(si, ax);
        VoiceOn(si);
    } else {
        VoiceOff(si);                           /* silence */
    }
    bx = (u16)(bx + 1u);                        /* noteDone */
    al = tc_ld_u8(TC_MKFP(es, bx));
    al = (u8)(al + 1u);                         /* QUIRK 1948:0142: a duration byte of FFh gives 0 here */
    ax = (u16)(al * LDB(AT(gVoiceLengthMul, si)));
    ax = (u16)(ax - 1u);
    STW(AT(gVoiceDuration, di), ax);
    bx = (u16)(bx + 1u);
    bp = (u16)(bp + 2u);
    STB(AT(gVoiceReadNext, si), 0u);
    goto nextVoice;

command:
    if (al == 0xCEu || al == 0xD2u) {
        /* cmdCE / cmdD2: count the loop down; at 0 step over the command, else jump back by the
           following word from the command's position plus 1 */
        u16 rep = al == 0xCEu ? gVoiceRepeat1 : gVoiceRepeat2;
        u8 n = (u8)(LDB(AT(rep, si)) - 1u);
        STB(AT(rep, si), n);
        if (n == 0) {
            bx = (u16)(bx + 3u);
            bp = (u16)(bp + 3u);
            goto readLoop;
        }
        bx = (u16)(bx + 1u);                    /* cmdCEJump / cmdD2Jump */
        ax = tc_ld_u16(TC_MKFP(es, bx));
        bp = (u16)(bp + 1u);
        bx = (u16)(bx - ax);
        {
            u16 r = (u16)(bp - ax);
            int of = ((bp ^ ax) & (bp ^ r) & 0x8000u) != 0;
            bp = r;
            if (of) {                           /* QUIRK 1948:0174 / 01B0: an overflow mutes this tick */
                goto muteVoice;
            }
        }
        goto readLoop;
    }
    if (al == 0xCDu || al == 0xD1u) {
        /* cmdCD / cmdD1: the loop count, 0 read as 1 */
        u16 rep = al == 0xCDu ? gVoiceRepeat1 : gVoiceRepeat2;
        bx = (u16)(bx + 1u);
        al = tc_ld_u8(TC_MKFP(es, bx));
        if (al == 0) {
            al = 1;
        }
        STB(AT(rep, si), al);
        bx = (u16)(bx + 1u);
        bp = (u16)(bp + 2u);
        goto readLoop;
    }
    if (al == 0xE6u) {
        /* cmdE6: a note at the frequency in the following word */
        bx = (u16)(bx + 1u);
        if (LDB(gTandyOn) == 1) {
            ax = div_es_bx(0x1B4F4ul, es, bx, si, bp);      /* cmdE6Tandy */
            if (ax >= 0x400u) {
                ax = 0x3FFu;
            }
        } else {
            ax = div_es_bx(0x1234DCul, es, bx, si, bp);
        }
        bx = (u16)(bx + 1u);                    /* cmdE6After */
        bp = (u16)(bp + 2u);
        goto afterTandy;
    }
    if (al == 0xE2u) {
        /* cmdE2: the length multiplier, 0 read as 1 */
        bx = (u16)(bx + 1u);
        al = tc_ld_u8(TC_MKFP(es, bx));
        if (al == 0) {
            al = 1;
        }
        STB(AT(gVoiceLengthMul, si), al);
        bx = (u16)(bx + 1u);
        bp = (u16)(bp + 2u);
        goto readLoop;
    }
    if (al == 0xE8u) {
        /* cmdE8: subtract from the multiplier. QUIRK 1948:0228: the clamp to 1 needs a signed
           overflow with a zero result, which cannot happen, so it is never taken */
        u8 m, r;
        int of;
        bx = (u16)(bx + 1u);
        al = tc_ld_u8(TC_MKFP(es, bx));
        m = LDB(AT(gVoiceLengthMul, si));
        r = (u8)(m - al);
        of = ((m ^ al) & (m ^ r) & 0x80u) != 0;
        STB(AT(gVoiceLengthMul, si), r);
        if (of && r == 0) {
            STB(AT(gVoiceLengthMul, si), 1u);
        }
        bx = (u16)(bx + 1u);                    /* cmdE8Ok */
        bp = (u16)(bp + 2u);
        goto readLoop;
    }
    if (al == 0xE9u) {
        /* cmdE9: add to the multiplier. QUIRK 1948:0242: FFh only on a signed overflow */
        u8 m, r;
        bx = (u16)(bx + 1u);
        al = tc_ld_u8(TC_MKFP(es, bx));
        m = LDB(AT(gVoiceLengthMul, si));
        r = (u8)(m + al);
        STB(AT(gVoiceLengthMul, si), r);
        if ((~(m ^ al) & (m ^ r) & 0x80u) != 0) {
            STB(AT(gVoiceLengthMul, si), 0xFFu);
        }
        bx = (u16)(bx + 1u);                    /* cmdE9Ok */
        bp = (u16)(bp + 2u);
        goto readLoop;
    }
    STB(AT(gVoiceReadNext, si), 0u);            /* endCommand: any other byte from 80h */
    goto muteVoice;

checkRestart:
    if (LDB(gStopRequest) == 1) {
        goto muteVoice;
    }
    if (LDB(AT(gVoiceSilent, si)) != 1) {
        u16 d = (u16)(LDW(AT(gVoiceDuration, di)) - 1u);
        STW(AT(gVoiceDuration, di), d);
        if ((d & 0x8000u) != 0 && d != 0xFFFFu) {  /* below -1: read the next note */
            STB(AT(gVoiceReadNext, si), 1u);
        }
    }
    if (LDB(AT(gVoiceMute, si)) == 0) {         /* restartDone */
        goto nextVoice;
    }
muteVoice:
    STB(AT(gVoiceMute, si), 0u);
    STB(AT(gVoiceSilent, si), 1u);
    VoiceOff(si);
nextVoice:
    STW(AT(gVoicePos, di), bp);
    si = (u16)(si + 1u);
    if ((s16)LDW(gVoiceCount) > (s16)si) {
        goto voiceLoop;
    }
    /* loopDone: count the silent voices from SI - 1 down to 0 */
    ax = 0;
    si = (u16)(si - 1u);
    do {
        if (LDB(AT(gVoiceSilent, si)) == 1) {
            ax = (u16)(ax + 1u);
        }
        si = (u16)(si - 1u);
    } while ((s16)si >= 0);
    if (LDW(gVoiceCount) == ax) {
        STB(gStopRequest, 0u);
        STB(gPlaying, 0u);
        STB(gRunning, 0u);
    }
}

/* 1948:000A: the INT 8 handler (the routine's symbol is at 1948:0006, where its first four bytes are
   the saved vector; the vector points at 000A, its code). It counts gTickCount to 123h and then
   gTickSeconds (signed compares, as the source's jle), runs a tick of the song, and then either
   acknowledges the PIC or, when the low byte of gChainCount is 0, reloads the count and jumps to the
   saved BIOS handler. QUIRK 1948:003F: only AL of the count is tested; the decrement stores the
   whole word (AH kept). Every register is kept. */
/*@ 1948:000A interrupt */ void SoundTimerIsr(void)
{
    u16 ax;
    u8 al;

    ax = (u16)(LDW(gTickCount) + 1u);
    if ((s16)ax > 0x123) {
        al = (u8)(LDB(gTickSeconds) + 1u);
        if ((s8)al > 0x3B) {
            al = 0;
        }
        STB(gTickSeconds, al);                  /* storeSeconds */
        ax = 0;
    }
    STW(gTickCount, ax);                        /* storeTick */
    SoundTick();
    ax = LDW(gChainCount);
    if ((u8)ax == 0) {
        STW(gChainCount, LDW(gChainReload));
        x86_chain(TC_MKFP(tc_ld_u16(OLD_INT8_SEG), tc_ld_u16(OLD_INT8_OFF)));
        return;
    }
    ax = (u16)((ax & 0xFF00u) | (u8)((u8)ax - 1u));      /* ackPIC: dec al */
    STW(gChainCount, ax);
    x86_outb(0x20, 0x20);
}

/* 1948:0377: install: save the INT 8 vector in cs:0006/0008, the chain reload 0Fh and count 0,
   INT 8 to cs:000A, PIT channel 0 mode 3 divisor 1000h, channel 2 mode 3 (no count), port 61h read,
   saved in gPort61Save and written back, then three voices on Tandy or one on the speaker. */
/*@ 1948:0377 */ void SoundDriverInstall(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u8 al;

    r.ax = 0x3508u;                             /* get the INT 8 vector: ES:BX */
    r.ds = SND_SEG;
    x86_int(0x21, &r);
    tc_st_u16(OLD_INT8_OFF, r.bx);
    tc_st_u16(OLD_INT8_SEG, r.es);
    STW(gChainReload, 0x0Fu);
    STW(gChainCount, 0u);
    r.ax = 0x2508u;                             /* set it to cs:000A */
    r.ds = (u16)(tc_load_seg + SEG);
    r.dx = 0x000Au;
    x86_int(0x21, &r);
    x86_outb(0x43, 0x36);
    x86_outb(0x40, 0x00);
    x86_outb(0x40, 0x10);
    x86_outb(0x43, 0xB6);
    al = x86_inb(0x61);
    STB(gPort61Save, al);
    x86_outb(0x61, al);
    if (LDB(gTandyOn) == 1) {
        SetThreeVoices();                       /* installTandy */
    } else {
        SetOneVoice();
    }
}

/* 1948:03DA: uninstall: INT 8 back to the saved vector, PIT channel 0 mode 3 divisor 0 (65536); on
   Tandy the three voices silenced, then one voice. */
/*@ 1948:03DA */ void SoundDriverUninstall(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    r.dx = tc_ld_u16(OLD_INT8_OFF);
    r.bx = tc_ld_u16(OLD_INT8_SEG);
    r.ds = r.bx;
    r.ax = 0x2508u;
    x86_int(0x21, &r);
    x86_outb(0x43, 0x36);
    x86_outb(0x40, 0x00);
    x86_outb(0x40, 0x00);
    if (LDB(gTandyOn) == 1) {
        SetThreeVoices();
    }
    SetOneVoice();                              /* uninstallSpeaker */
}

/* 1948:0418: start a sound: the far pointer to its song in gSongPtr/gSongPtrSeg, and the start
   request, playing and running flags, so the next tick loads it. */
/*@ 1948:0418 */ void StartSound(farptr routine)
{
    STW(gSongPtr, TC_OFF(routine));
    STW(gSongPtrSeg, TC_SEG(routine));
    STB(gStartRequest, 1u);
    STB(gPlaying, 1u);
    STB(gRunning, 1u);
}

/* 1948:0443: ask for a stop: gStopRequest, read by the next tick's checkRestart */
/*@ 1948:0443 */ void StopSound(void)
{
    STB(gStopRequest, 1u);
}

/* 1948:0458: three-voice output (SetThreeVoices) */
/*@ 1948:0458 */ void SoundDriverResume(void)
{
    SetThreeVoices();
}

/* 1948:046B: three voices silenced, then one-voice output */
/*@ 1948:046B */ void SoundDriverSilenceToOneVoice(void)
{
    SetThreeVoices();
    SetOneVoice();
}
