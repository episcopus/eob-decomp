/* E_TEXT.c: src/E_TEXT.ASM in C (playbook 80, M3): Westwood's AdLib sound driver, the _TEXT segment
   of EOB.EXE between the start-up and the runtime library (load-relative segment 0000).

   The driver keeps code and data in one segment and runs with DS = CS. Its data stays in the arena
   at its offsets in segment 0000, where the EXE image put it and where the game changes it: the
   pitch curves (020C), the driver block LoadSoundDriver reads (0416), the level curves (3944), the
   variables (3B36), the ten voice records (3B93, 43h bytes each), the voice tables (3E31) and the
   script command jump table (4F94). Nothing is copied into C.

   Written from the TASM source, which stays the reference, label by label. The routines inside the
   driver pass values in registers and lean on what a callee leaves in them (VoiceKeyOff's AL is
   compared again at 41E8, VoiceModulatorLevel's CL is the operator offset LoadVoicePatch goes on
   adding), so they run over a register record (`cpu`, AX BX CX DX SI DI) named as the source names
   the registers; a routine that saves a register (push/pop) saves it here too. Every port access is
   made in the source's order and count, the AdLib status polls of the two waits included. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x0000u
#define CSEG ((u16)(tc_load_seg + SEG))
#define P(off) TC_MKFP(CSEG, (u16)(off))
#define RB(off) tc_ld_u8(P(off))
#define WB(off, v) tc_st_u8(P(off), (u8)(v))
#define RW(off) tc_ld_u16(P(off))
#define WW(off, v) tc_st_u16(P(off), (u16)(v))

/* the data, by its offsets in segment 0000 (the source's names) */
#define gCurveTablePtrs 0x020Cu
#define gDriverTablePtrs 0x039Cu
#define gScriptFlag 0x03A0u
#define DriverBlock 0x0416u
#define gPatchTable 0x0542u
#define gLevelCurvePtrs 0x395Au
#define gBusyCountdown 0x3B36u
#define gNoteDuration 0x3B3Bu
#define gVoiceIndex 0x3B3Eu
#define gUpdateEnable 0x3B3Fu
#define gRhythmKeyBits 0x3B40u
#define gTempoRate 0x3B41u
#define gEffectMode 0x3B42u
#define gEffectPeriod 0x3B43u
#define gEffectCounter 0x3B44u
#define gEffectIndex 0x3B45u
#define gRandomSeed 0x3B47u
#define gToggleBit 0x3B49u
#define gRegBDDepth 0x3B4Au
#define gOperatorOffset 0x3B4Bu
#define gRhythmLevelA 0x3B4Cu
#define gRhythmLevelB 0x3B51u
#define gRhythmLevelC 0x3B56u
#define gTempoReload 0x3B5Bu
#define gTempoCounter 0x3B5Cu
#define gTempoAccum 0x3B5Du
#define gTempoBeat 0x3B5Eu
#define gTempoWait 0x3B5Fu
#define gEffectCurveA 0x3B60u
#define gEffectCurveB 0x3B62u
#define gSoundTable 0x3B64u
#define gSavedTimerOff 0x3B66u
#define gSavedTimerSeg 0x3B68u
#define gSampleSavedOff 0x3B6Au
#define gSampleSavedSeg 0x3B6Cu
#define gTimerPasses12 0x3B6Eu
#define gTimerPasses48 0x3B6Fu
#define gSampleNibbles 0x3B74u
#define gSampleLevel 0x3B75u
#define gSampleLength 0x3B76u
#define gSamplePtrOff 0x3B78u
#define gSamplePtrSeg 0x3B7Au
#define gSampleStart 0x3B7Cu
#define gSampleEnable 0x3B7Eu
#define gQueueWrite 0x3B7Fu
#define gQueueRead 0x3B81u
#define gQueueRing 0x3B83u
#define gVoiceRecord0 0x3B93u
#define VOICE_RECORD_SIZE 0x43u
#define gVoiceRecordPtrs 0x3E31u
#define gOperatorOffsets 0x3E45u
#define gFNumbers 0x3E4Eu
#define CommandTable 0x4F94u

/* the code addresses the voice records' handler words (+29h, +2Bh) and the jump table hold */
#define L_4E7D 0x4E7Du
#define L_4E7E 0x4E7Eu
#define L_4ED7 0x4ED7u
#define L_4F00 0x4F00u
#define AdlibTimerIsrOff 0x3E66u
#define SampleTimerIsrOff 0x3EE5u

/* the registers of the driver's internal routines */
typedef struct {
    u16 ax, bx, cx, dx, si, di;
} cpu;

static inline void set_lo(u16 *x, u8 v) { *x = (u16)((*x & 0xFF00u) | v); }
static inline void set_hi(u16 *x, u8 v) { *x = (u16)((*x & 0x00FFu) | ((u16)v << 8)); }
#define AL ((u8)c->ax)
#define AH ((u8)(c->ax >> 8))
#define BL ((u8)c->bx)
#define BH ((u8)(c->bx >> 8))
#define CL ((u8)c->cx)
#define CH ((u8)(c->cx >> 8))
#define DL ((u8)c->dx)
#define DH ((u8)(c->dx >> 8))
#define MOV_AL(v) set_lo(&c->ax, (u8)(v))
#define MOV_AH(v) set_hi(&c->ax, (u8)(v))
#define MOV_BL(v) set_lo(&c->bx, (u8)(v))
#define MOV_BH(v) set_hi(&c->bx, (u8)(v))
#define MOV_CL(v) set_lo(&c->cx, (u8)(v))
#define MOV_CH(v) set_hi(&c->cx, (u8)(v))
#define MOV_DL(v) set_lo(&c->dx, (u8)(v))
#define MOV_DH(v) set_hi(&c->dx, (u8)(v))
/* a byte or word of the voice record at DI */
#define F8(o) RB(c->di + (o))
#define SF8(o, v) WB(c->di + (o), (v))
#define F16(o) RW(c->di + (o))
#define SF16(o, v) WW(c->di + (o), (v))

static void lodsb(cpu *c) { MOV_AL(x86_lodsb(CSEG, &c->si)); }
static void lodsw(cpu *c) { c->ax = x86_lodsw(CSEG, &c->si); }
static void xchg_ah_al(cpu *c) { c->ax = (u16)((c->ax << 8) | (c->ax >> 8)); }

/* `shl r16, cl` and `shr r16, cl` as the oracle's CPU does them: the count masked to five bits, a count of
   16 or more clears the word (an 8086 would shift by the whole count, with the same result below 16) */
static u16 shl16(u16 v, u8 n)
{
    n &= 0x1Fu;
    return n >= 16 ? 0 : (u16)(v << n);
}

static u16 shr16(u16 v, u8 n)
{
    n &= 0x1Fu;
    return n >= 16 ? 0 : (u16)(v >> n);
}

/* 0000:4CD0 AdlibWaitAfterData: reads the status port 388h 17h times; keeps every register */
static void AdlibWaitAfterData(void)
{
    u16 n;
    for (n = 0x17u; n != 0; n--) {
        (void)x86_inb(0x388u);
    }
}

/* 0000:4CE0 AdlibWaitAfterIndex: reads the status port 388h 4 times; keeps every register */
static void AdlibWaitAfterIndex(void)
{
    u16 n;
    for (n = 4u; n != 0; n--) {
        (void)x86_inb(0x388u);
    }
}

/* 0000:4C6B AdlibWrite: the value AL to the AdLib register AH; keeps AX, leaves DX = 389h */
static void AdlibWrite(cpu *c)
{
    c->dx = 0x388u;
    x86_outb(c->dx, AH);
    AdlibWaitAfterIndex();
    c->dx = 0x389u;
    x86_outb(c->dx, AL);
    AdlibWaitAfterData();
}

/* AdlibWrite with AX = reg:value */
static void adlib(cpu *c, u8 reg, u8 value)
{
    c->ax = (u16)((reg << 8) | value);
    AdlibWrite(c);
}

/* 0000:4C7C ClampLevel: the signed AX to 0 to 3Fh, in AL */
static void ClampLevel(cpu *c)
{
    if ((s16)c->ax < 0) {
        MOV_AL(0);
    } else if ((s16)c->ax > 0x3F) {
        MOV_AL(0x3Fu);
    }
}

/* 0000:4F84 SoundRandom: gRandomSeed + 9248h rotated right three times, in AX and the seed */
static void SoundRandom(cpu *c)
{
    u16 ax = (u16)(RW(gRandomSeed) + 0x9248u);
    ax = (u16)((ax >> 3) | (ax << 13));
    WW(gRandomSeed, ax);
    c->ax = ax;
}

/* 0000:4D95 ClearVoice: zeroes the record at DI from +03h for 40h bytes, then speed FFh, priority 0, both
   handlers L_4E7D, second key-off count 1. Keeps every register (rep stosw of 20h words, then
   `adc cx,cx` with the carry of `shr cx,1`, clear: the stosb moves nothing). */
static void ClearVoice(cpu *c)
{
    u16 di = (u16)(c->di + 3u), cx = 0x20u;
    for (; cx != 0; cx--) {
        x86_stosw(CSEG, &di, 0);
    }
    SF8(0x25, 0xFFu);
    SF8(0x08, 0);
    SF16(0x29, L_4E7D);
    SF16(0x2B, L_4E7D);
    SF8(0x35, 1);
}

/* 0000:4D1F VoiceCarrierLevel: the carrier level byte in AL (keeps AH; CL and BX change when +33h is
   set) */
static void VoiceCarrierLevel(cpu *c)
{
    MOV_AL(F8(0x2F) & 0x3Fu);
    MOV_AL(AL + F8(0x20));
    MOV_AL(AL + F8(0x00));
    MOV_AL(AL + F8(0x30));
    if (F8(0x33) != 0) {
        MOV_CL(F8(0x33) + 1u);
        MOV_BH(0);
        MOV_BL(F8(0x34));
        c->bx = shl16(c->bx, CL);
        MOV_AL(AL - BH);
    }
    if (AL > 0x3Fu) {                           /* jbe: unsigned */
        MOV_AL((s8)AL < 0 ? 0 : 0x3Fu);         /* jl: signed */
    }
    MOV_BL(F8(0x2F) & 0xC0u);
    MOV_AL(AL | BL);
}

/* 0000:4D57 VoiceModulatorLevel: the same for the modulator, from +2Eh; the record's level terms are added
   only when the connection bit +31h is set, the shift is by +32h */
static void VoiceModulatorLevel(cpu *c)
{
    MOV_AL(F8(0x2E) & 0x3Fu);
    if (F8(0x31) != 0) {
        MOV_AL(AL + F8(0x20));
        MOV_AL(AL + F8(0x00));
        MOV_AL(AL + F8(0x30));
    }
    if (F8(0x32) != 0) {
        MOV_CL(F8(0x32) + 1u);
        MOV_BH(0);
        MOV_BL(F8(0x34));
        c->bx = shl16(c->bx, CL);
        MOV_AL(AL - BH);
    }
    if (AL > 0x3Fu) {
        MOV_AL((s8)AL < 0 ? 0 : 0x3Fu);
    }
    MOV_BL(F8(0x2E) & 0xC0u);
    MOV_AL(AL | BL);
}

/* 0000:4CF0 WriteVoiceReg: the carrier level register, and the modulator's when +31h is set */
static void WriteVoiceReg(cpu *c)
{
    VoiceCarrierLevel(c);
    c->bx = gOperatorOffsets;
    MOV_BL(BL + RB(gVoiceIndex));
    MOV_CL(RB(c->bx));
    MOV_AH(CL + 0x43u);
    AdlibWrite(c);
    if (F8(0x31) != 0) {
        VoiceModulatorLevel(c);
        c->bx = gOperatorOffsets;
        MOV_BL(BL + RB(gVoiceIndex));
        MOV_CL(RB(c->bx));
        MOV_AH(CL + 0x40u);
        AdlibWrite(c);
    }
}

/* 0000:4DC8 LoadVoicePatch: the 11 patch bytes at SI to the voice's operator registers (CL = the operator
   offset, DI = the record). QUIRK: VoiceModulatorLevel and VoiceCarrierLevel change CL when the
   record's shift (+32h, +33h) is set, and the registers after them are numbered from that CL; kept. */
static void LoadVoicePatch(cpu *c)
{
    MOV_AH(0x20u + CL);
    lodsb(c);
    AdlibWrite(c);
    MOV_AH(0x23u + CL);
    lodsb(c);
    AdlibWrite(c);
    MOV_AH(0xC0u + RB(gVoiceIndex));
    lodsb(c);
    AdlibWrite(c);
    MOV_AL(AL & 1u);
    SF8(0x31, AL);
    MOV_AH(0xE0u + CL);
    lodsb(c);
    AdlibWrite(c);
    MOV_AH(0xE3u + CL);
    lodsb(c);
    AdlibWrite(c);
    MOV_AH(0x40u + CL);
    lodsb(c);
    SF8(0x2E, AL);
    VoiceModulatorLevel(c);
    AdlibWrite(c);
    MOV_AH(0x43u + CL);
    lodsb(c);
    SF8(0x2F, AL);
    VoiceCarrierLevel(c);
    AdlibWrite(c);
    MOV_AH(0x60u + CL);
    lodsb(c);
    AdlibWrite(c);
    MOV_AH(0x63u + CL);
    lodsb(c);
    AdlibWrite(c);
    MOV_AH(0x80u + CL);
    lodsb(c);
    AdlibWrite(c);
    MOV_AH(0x83u + CL);
    lodsb(c);
    AdlibWrite(c);
}

/* 0000:4E34 VoiceSilence: CL = the voice. Attack/decay and sustain/release FFh, then B0h 00h. QUIRK 4E79:
   the next write sends 20h to the same B0h register (AdlibWrite keeps AX). Skips voices 6 and up in
   rhythm mode. */
static void VoiceSilence(cpu *c)
{
    if (RB(gRhythmKeyBits) != 0 && (s8)CL >= 6) {
        return;
    }
    c->bx = gOperatorOffsets;
    MOV_CH(CL);
    MOV_BL(BL + CL);
    MOV_CL(RB(c->bx));
    adlib(c, (u8)(0x60u + CL), 0xFFu);
    adlib(c, (u8)(0x63u + CL), 0xFFu);
    adlib(c, (u8)(0x80u + CL), 0xFFu);
    adlib(c, (u8)(0x83u + CL), 0xFFu);
    adlib(c, (u8)(0xB0u + CH), 0);
    MOV_AL(0x20u);
    AdlibWrite(c);
}

/* 0000:4E7E (L_4E7E): the oscillator, a per-tick handler (+29h) */
static void Oscillator(cpu *c)
{
    if (F8(0x1F) != 0) {
        SF8(0x1F, F8(0x1F) - 1u);
        return;
    }
    {
        u16 sum = (u16)(F8(0x1E) + F8(0x1D));
        MOV_CL(sum);
        SF8(0x1E, CL);
        if (sum < 0x100u) {
            return;
        }
    }
    c->bx = F16(0x17);
    SF8(0x1A, F8(0x1A) - 1u);
    if (F8(0x1A) == 0) {
        c->bx = (u16)((c->bx ^ 0xFFFFu) + 1u);
        SF16(0x17, c->bx);
        MOV_AL(F8(0x1B));
        SF8(0x1A, AL);
    }
    c->ax = (u16)((F16(0x27) & 0x3FFu) + c->bx);
    SF8(0x27, AL);
    MOV_AL((F8(0x28) & 0xFCu) | AH);
    SF8(0x28, AL);
    adlib(c, (u8)(0xA0u + RB(gVoiceIndex)), F8(0x27));
    adlib(c, (u8)(0xB0u + RB(gVoiceIndex)), F8(0x28));
}

/* 0000:4ED7 (L_4ED7): the pitch envelope, a per-voice handler (+2Bh): on a carry of its accumulator, the
   next byte of its table (xlatb) to its register */
static void PitchEnvelope(cpu *c)
{
    u16 sum = (u16)(F8(0x3A) + F8(0x39));
    MOV_AL(sum);
    SF8(0x3A, AL);
    if (sum < 0x100u) {
        return;
    }
    SF8(0x3C, F8(0x3C) - 1u);
    if ((s8)F8(0x3C) < 0) {
        MOV_AL(F8(0x3B));
        SF8(0x3C, AL);
    }
    MOV_AH(F8(0x3D) + RB(gOperatorOffset));
    MOV_AL(F8(0x3C));
    c->bx = F16(0x3E);
    MOV_AL(RB(c->bx + AL));
    AdlibWrite(c);
}

/* 0000:4F00 (L_4F00): the pitch slide, a per-tick handler (+29h). QUIRK 4F35 and 4F51: when the F-number
   halved (doubled) has its low ten bits zero, the source pops twice where it pushed once, taking its
   own return address into BX and returning to the word below it (UpdateVoices' saved DI, an address in
   the voice records): C cannot follow, so the port stops there. */
static void PitchSlide(cpu *c)
{
    u16 sum = (u16)(F8(0x14) + F8(0x13));
    MOV_CL(sum);
    SF8(0x14, CL);
    if (sum < 0x100u) {
        return;
    }
    MOV_BL(F8(0x27));
    MOV_BH(F8(0x28) & 3u);
    MOV_DL(F8(0x28));
    MOV_DH(DL);
    MOV_DL(DL & 0x1Cu);
    MOV_DH(DH & 0x20u);
    c->cx = F16(0x15);
    if ((s16)c->cx >= 0) {
        c->bx = (u16)(c->bx + c->cx);
        if ((s16)c->bx >= 0x2DE) {
            c->bx = (u16)(c->bx >> 1);
            if ((c->bx & 0x3FFu) == 0) {
                tc_fail(75, "E_TEXT 0000:4F35: the pitch slide pops its return address (BX %04X): the original "
                        "returns into the voice records", c->bx);
            }
            MOV_DL(DL + 4u);
            MOV_DL(DL & 0x1Cu);
        }
    } else {
        c->bx = (u16)(c->bx + c->cx);
        if ((s16)c->bx <= 0x184) {
            c->bx = (u16)(c->bx << 1);
            if ((c->bx & 0x3FFu) == 0) {
                tc_fail(75, "E_TEXT 0000:4F51: the pitch slide pops its return address (BX %04X): the original "
                        "returns into the voice records", c->bx);
            }
            MOV_DL(DL - 4u);
            MOV_DL(DL & 0x1Cu);
        }
    }
    c->bx &= 0x3FFu;                            /* L_4F5D */
    c->cx = c->dx;
    MOV_AH(0xA0u + RB(gVoiceIndex));
    MOV_AL(BL);
    AdlibWrite(c);
    SF8(0x27, AL);
    MOV_AH(0xB0u + RB(gVoiceIndex));
    MOV_BH(BH | CL);
    MOV_BH(BH | CH);
    MOV_AL(BH);
    AdlibWrite(c);
    SF8(0x28, AL);
}

/* a call through a voice record's handler word (+29h or +2Bh): the four handlers the driver sets */
static void call_handler(cpu *c, u16 target, u16 field)
{
    switch (target) {
    case L_4E7D:
        return;                                 /* a bare ret */
    case L_4E7E:
        Oscillator(c);
        return;
    case L_4ED7:
        PitchEnvelope(c);
        return;
    case L_4F00:
        PitchSlide(c);
        return;
    default:
        tc_fail(75, "E_TEXT 0000:425F: voice record %04X +%02Xh calls 0000:%04X, which is none of the driver's "
                "handlers (4E7D, 4E7E, 4ED7, 4F00)", c->di, field, target);
    }
}

/* 0000:4A3A SetVoicePitch: AL = the note; registers A0h and B0h of the voice; keeps AX */
static void SetVoicePitch(cpu *c)
{
    u16 saved = c->ax;
    SF8(0x41, AL);
    MOV_CH(AL & 0xF0u);
    MOV_BL(F8(0x07));
    MOV_CH(CH + BL);
    MOV_AL(AL & 0x0Fu);
    MOV_BL(F8(0x12));
    MOV_AL(AL + BL);
    if ((s8)AL >= 12) {
        MOV_AL(AL - 12u);
        MOV_CH(CH + 0x10u);
    } else if ((s8)AL < 0) {
        MOV_AL(AL + 12u);
        MOV_CH(CH - 0x10u);
    }
    MOV_BL(AL);                                 /* L_4A63 */
    MOV_BH(0);
    MOV_BL(BL << 1);
    c->ax = RW(c->bx + gFNumbers);
    c->dx = F8(0x24);
    c->ax = (u16)(c->ax + c->dx);
    MOV_CH(CH >> 2);
    MOV_CH(CH & 0x1Cu);
    MOV_CH(CH | AH);
    MOV_CL(AL);
    if (F8(0x42) != 0) {
        u16 si;
        if ((s8)F8(0x42) > 0) {
            MOV_BH(0);
            MOV_BL(F8(0x41));
            MOV_BL(BL & 0x0Fu);
            MOV_BL(BL + 2u);
            c->bx = (u16)(c->bx << 1);
            si = RW(c->bx + gLevelCurvePtrs);
            MOV_BH(0);
            MOV_BL(F8(0x42));
            MOV_AL(RB(c->bx + si));
            MOV_AH(0);
            c->cx = (u16)(c->cx + c->ax);
        } else {
            MOV_BH(0);                          /* L_4AA8 */
            MOV_BL(F8(0x41));
            MOV_BL(BL & 0x0Fu);
            c->bx = (u16)(c->bx << 1);
            si = RW(c->bx + gLevelCurvePtrs);
            MOV_BH(0);
            MOV_BL(F8(0x42));
            MOV_BL((BL ^ 0xFFu) + 1u);
            MOV_AL(RB(c->bx + si));
            MOV_AH(0);
            c->cx = (u16)(c->cx - c->ax);
        }
    }
    MOV_AL(F8(0x28) & 0x20u);                   /* L_4AC8 */
    MOV_CH(CH | AL);
    SF8(0x28, CH);
    SF8(0x27, CL);
    adlib(c, (u8)(0xA0u + RB(gVoiceIndex)), CL);
    adlib(c, (u8)(0xB0u + RB(gVoiceIndex)), CH);
    c->ax = saved;
}

/* 0000:4AEF VoiceKeyOn: key-on in B0h; the oscillator delta (+17h) from the F-number and its shift
   (+19h), and its wait (+1Fh) from +1Ch; keeps AX */
static void VoiceKeyOn(cpu *c)
{
    u16 saved = c->ax;
    MOV_AH(0xB0u + RB(gVoiceIndex));
    MOV_AL(F8(0x28) | 0x20u);
    SF8(0x28, AL);
    AdlibWrite(c);
    MOV_CL(F8(0x19));
    MOV_AL(9u - CL);
    MOV_CL(AL);
    c->ax = (u16)(F16(0x27) & 0x3FFu);
    c->ax = shr16(c->ax, CL);
    MOV_AH(0);
    SF16(0x17, c->ax);
    MOV_CL(F8(0x1C));
    SF8(0x1F, CL);
    c->ax = saved;
}

/* 0000:4B1F VoiceKeyOff: clears the key-on bit of B0h; skips voices 6 and up in rhythm mode and voice 9 */
static void VoiceKeyOff(cpu *c)
{
    if (RB(gRhythmKeyBits) != 0 && (s8)RB(gVoiceIndex) >= 6) {
        return;
    }
    if (RB(gVoiceIndex) == 9) {
        return;
    }
    MOV_AH(0xB0u + RB(gVoiceIndex));
    MOV_AL(F8(0x28) & 0xDFu);
    SF8(0x28, AL);
    AdlibWrite(c);
}

/* 0000:4B47 StartVoiceNote: AH = the duration; the note delay (+05h), with the random delay (+36h) or the
   key-off count (+21h) */
static void StartVoiceNote(cpu *c)
{
    WB(gNoteDuration, AH);
    MOV_BL(AH);
    if (F8(0x36) != 0) {
        SoundRandom(c);
        MOV_AL(AL & F8(0x36));
        MOV_BL(BL + AL);
        SF8(0x05, BL);
        return;
    }
    if (F8(0x2D) != 0) {
        MOV_BL(AH);
        MOV_AH(AH >> 3);
        MOV_CL(F8(0x2D));
        MOV_CH(0);
        MOV_AL(0);
        do {                                    /* L_4B74: loop */
            MOV_AL(AL + AH);
            c->cx--;
        } while (c->cx != 0);
        SF8(0x21, AL);
    }
    SF8(0x05, BL);                              /* L_4B7B */
}

/* 0000:4C8D TickTempo: gTempoAccum += gTempoRate; a carry counts gTempoCounter down; at zero it reloads
   and steps gTempoBeat */
static void TickTempo(cpu *c)
{
    u16 sum = (u16)(RB(gTempoAccum) + RB(gTempoRate));
    MOV_AL(sum);
    WB(gTempoAccum, AL);
    if (sum < 0x100u) {
        return;
    }
    WB(gTempoCounter, RB(gTempoCounter) - 1u);
    if (RB(gTempoCounter) != 0) {
        return;
    }
    MOV_AL(RB(gTempoReload));
    WB(gTempoCounter, AL);
    WB(gTempoBeat, RB(gTempoBeat) + 1u);
}

/* 0000:413B ResetVoices: the reset registers, every voice's levels to 3Fh, every record cleared */
static void ResetVoices(cpu *c)
{
    WB(gUpdateEnable, 0);
    WW(gRandomSeed, 0x1234u);
    c->ax = 0x0120u;
    AdlibWrite(c);
    c->ax = 0x0800u;
    AdlibWrite(c);
    c->ax = 0xBD00u;
    AdlibWrite(c);
    WB(gEffectMode, 0);
    c->cx = 0x0Au;
    MOV_BH(0);
    do {                                        /* L_4162 */
        MOV_BL(CL - 1u);
        if (BL != 9) {
            MOV_AH(RB(c->bx + gOperatorOffsets) + 0x40u);
            MOV_AL(0x3Fu);
            AdlibWrite(c);
            MOV_AH(RB(c->bx + gOperatorOffsets) + 0x43u);
            MOV_AL(0x3Fu);
            AdlibWrite(c);
        }
        MOV_BL(BL << 1);                        /* L_4183 */
        c->di = RW(c->bx + gVoiceRecordPtrs);
        ClearVoice(c);
        c->cx--;
    } while (c->cx != 0);
}

/* a sound's header, as ProcessSoundQueue (4043 to 404F) and the script command 82h (428B to 4295) read
   it: BL = the sound number (BH zero); SI = its data in the sound table gSoundTable, BX = its first word
   (BL the voice, BH the priority) */
static void sound_header(cpu *c)
{
    c->bx = (u16)(c->bx << 1);
    c->bx = (u16)(c->bx + RW(gSoundTable));
    c->si = RW(c->bx);
    c->bx = RW(c->si);
}

/* 0000:402C ProcessSoundQueue: starts each sound queued since the last tick */
static void ProcessSoundQueue(cpu *c)
{
    u16 si = c->si, di = c->di;
    for (;;) {
        c->bx = RW(gQueueRead);                 /* L_4033 */
        if (c->bx == RW(gQueueWrite)) {
            break;
        }
        c->bx = RW(c->bx + gQueueRing);
        MOV_BH(0);
        sound_header(c);
        if (BL == 0) {
            WB(gEffectMode, 0);
        }
        MOV_AL(BH);                             /* L_405A */
        MOV_CL(BL);
        MOV_BH(0);
        c->bx = (u16)(c->bx << 1);
        c->di = RW(c->bx + gVoiceRecordPtrs);
        if ((s8)AL >= (s8)F8(0x08)) {
            ClearVoice(c);
            SF8(0x08, AL);
            c->si = (u16)(c->si + 2u);
            SF16(0x03, c->si);
            SF8(0x25, 0xFFu);
            SF8(0x26, 0xFFu);
            SF8(0x05, 1);
            WB(gUpdateEnable, 1);
            if (BL != 0x12u) {                  /* BL is 2 * the voice: voice 9 is not silenced */
                VoiceSilence(c);
            }
        }
        WW(gQueueRead, RW(gQueueRead) + 1u);    /* L_408F */
        WW(gQueueRead, RW(gQueueRead) & 0x0Fu);
    }
    c->si = si;
    c->di = di;
}

/* the rhythm level commands C4h, C5h and C6h (484B, 48EF, 4993): BX = the operand (the operator mask) and
   the level byte; for each of the five operator bits, in the source's order, the level register gets the
   clamped sum of the level and the operator's terms */
static void rhythm_levels(cpu *c, int kind)
{
    static const u8 bit[5] = {0x01u, 0x02u, 0x04u, 0x08u, 0x10u};
    static const u8 term[5] = {2, 4, 3, 1, 0};
    static const u8 reg[5] = {0x51u, 0x55u, 0x52u, 0x54u, 0x53u};
    int k;
    for (k = 0; k < 5; k++) {
        u16 a = (u16)(gRhythmLevelA + term[k]), b = (u16)(gRhythmLevelB + term[k]), cc = (u16)(gRhythmLevelC + term[k]);
        c->ax = c->bx;
        if (!(AH & bit[k])) {
            continue;
        }
        switch (kind) {
        case 0xC4:                              /* L_484B: the level is the C term */
            WB(cc, AL);
            MOV_AH(0);
            MOV_AL(AL + RB(a));
            MOV_AL(AL + RB(b));
            MOV_AL(AL + RB(cc));
            ClampLevel(c);
            break;
        case 0xC5:                              /* L_48EF: the clamped sum is the B term */
            MOV_AH(0);
            MOV_AL(AL + RB(a));
            MOV_AL(AL + RB(b));
            MOV_AL(AL + RB(cc));
            ClampLevel(c);
            WB(b, AL);
            break;
        default:                                /* L_4993: the level is the B term */
            WB(b, AL);
            MOV_AH(0);
            MOV_AL(AL + RB(a));
            MOV_AL(AL + RB(cc));
            ClampLevel(c);
            break;
        }
        MOV_AH(reg[k]);
        AdlibWrite(c);
    }
}

/* 0000:418F UpdateVoices: each voice from 9 down to 0: its speed, its note delay, its script (the
   commands through the jump table at 4F94, read from the arena), its two handlers */
static void UpdateVoices(cpu *c)
{
    u16 target, saved, saved_di;

    if (RB(gUpdateEnable) == 0) {
        return;
    }
    WB(gVoiceIndex, 9);
voice:                                          /* L_419C */
    MOV_BL(RB(gVoiceIndex));
    MOV_BH(0);
    MOV_BL(BL << 1);
    c->di = RW(c->bx + gVoiceRecordPtrs);
    MOV_AL(F8(0x05));
    if (AL == 0) {
        goto next;
    }
    c->bx = gOperatorOffsets;                   /* L_41B2 */
    MOV_BL(BL + RB(gVoiceIndex));
    MOV_AL(RB(c->bx));
    WB(gOperatorOffset, AL);
    if (F8(0x40) != 0) {
        MOV_AL(RB(gTempoRate));
        SF8(0x25, AL);
    }
    {
        u16 sum = (u16)(F8(0x26) + F8(0x25));   /* L_41CA */
        MOV_AL(sum);
        SF8(0x26, AL);
        if (sum < 0x100u) {
            goto handlers;
        }
    }
    SF8(0x05, F8(0x05) - 1u);                   /* L_41D8 */
    if (F8(0x05) != 0) {
        MOV_AL(F8(0x05));
        if (AL == F8(0x21)) {
            VoiceKeyOff(c);
        }
        /* L_41E8: AL is what VoiceKeyOff left in it when it wrote */
        if (AL == F8(0x35) && RB(gVoiceIndex) != 9) {
            VoiceKeyOff(c);
        }
        goto handlers;
    }
    c->si = F16(0x03);                          /* L_41F9 */
script:                                         /* L_41FC */
    lodsw(c);
    if ((s8)AL >= 0) {
        /* a note: AL the note, AH the duration */
        SetVoicePitch(c);
        VoiceKeyOn(c);
        StartVoiceNote(c);
        if (F16(0x32) != 0 || F16(0x33) != 0) {
            lodsb(c);                           /* L_421A */
            SF8(0x34, AL);
            VoiceCarrierLevel(c);
            c->dx = 0x388u;
            MOV_AH(RB(gOperatorOffset) + 0x43u);
            AdlibWrite(c);
            VoiceModulatorLevel(c);
            c->dx = 0x388u;
            MOV_AH(RB(gOperatorOffset) + 0x40u);
            AdlibWrite(c);
        }
        MOV_AL(RB(gNoteDuration));              /* L_423E */
        if (AL == 0) {
            goto script;
        }
        SF16(0x03, c->si);
        goto handlers;
    }
    MOV_AL(AL & 0x7Fu);                         /* L_424A */
    if ((s8)AL > 0x70) {
        goto end_script;
    }
    MOV_BL(AL);                                 /* L_4253 */
    MOV_BH(0);
    MOV_BL(BL << 1);
    c->bx = RW(c->bx + CommandTable);
    target = c->bx;
    switch (target) {
    case 0x4271:                                /* 80h: the loop count */
        SF8(0x06, AH);
        goto script;
    case 0x4276:                                /* 81h: the loop back while the count lasts */
        SF8(0x06, F8(0x06) - 1u);
        if (F8(0x06) != 0) {
            goto jump;
        }
        c->si++;
        goto script;
    case 0x427F:                                /* 82h: starts the sound AH */
        {
            u16 si = c->si, di = c->di, bx = c->bx;
            MOV_BL(AH);
            if (BL < 0xFFu) {
                MOV_BH(0);                      /* L_428B */
                sound_header(c);
                MOV_AL(BH);
                MOV_CL(BL);
                MOV_BH(0);
                c->bx = (u16)(c->bx << 1);
                c->di = RW(c->bx + gVoiceRecordPtrs);
                if ((s8)AL >= (s8)F8(0x08)) {
                    ClearVoice(c);
                    SF8(0x08, AL);
                    c->si = (u16)(c->si + 2u);
                    SF16(0x03, c->si);
                    SF8(0x25, 0xFFu);
                    SF8(0x26, 0xFFu);
                    SF8(0x05, 1);
                    WB(gUpdateEnable, 1);
                    VoiceSilence(c);
                }
            }
            c->bx = bx;                         /* L_42C8 */
            c->di = di;
            c->si = si;
        }
        goto script;
    case 0x42CE:                                /* 83h: the second key-off count */
        SF8(0x35, AH);
        goto script;
    case 0x42D4:                                /* 84h: jump */
        goto jump;
    case 0x42E0:                                /* 85h: call */
        c->si--;
        MOV_BL(F8(0x09));
        MOV_BH(0);
        MOV_BL(BL << 1);
        c->ax = (u16)(c->si + 2u);
        WW(c->bx + c->di + 0x0Au, c->ax);
        SF8(0x09, F8(0x09) + 1u);
        lodsw(c);
        c->si = c->ax;
        goto script;
    case 0x42F8:                                /* 86h: return */
        c->si--;
        SF8(0x09, F8(0x09) - 1u);
        MOV_BL(F8(0x09));
        MOV_BH(0);
        MOV_BL(BL << 1);
        c->ax = RW(c->bx + c->di + 0x0Au);
        c->si = c->ax;
        goto script;
    case 0x430B:                                /* 87h: transpose */
        SF8(0x07, AH);
        goto script;
    case 0x4311:                                /* the end of the script */
        goto end_script;
    case 0x4327:                                /* 89h: a rest */
        StartVoiceNote(c);
        SF16(0x03, c->si);
        if (RB(gVoiceIndex) != 9) {
            VoiceKeyOff(c);
        }
        goto duration;
    case 0x4344:                                /* 8Ah: a register write */
        lodsb(c);
        AdlibWrite(c);
        goto script;
    case 0x434B:                                /* 8Bh: a note with no key-on */
        xchg_ah_al(c);
        SetVoicePitch(c);
        lodsb(c);
        xchg_ah_al(c);
        StartVoiceNote(c);
        SF16(0x03, c->si);
        goto duration;
    case 0x4366:                                /* 8Ch: detune */
        SF8(0x12, AH);
        goto script;
    case 0x436C:                                /* 8Dh: the pitch envelope */
        SF8(0x3A, AH);
        SF8(0x39, AH);
        lodsb(c);
        SF8(0x3B, AL);
        SF8(0x3C, AL);
        lodsb(c);
        SF8(0x3D, AL);
        lodsw(c);
        SF16(0x3E, c->ax);
        SF16(0x2B, L_4ED7);
        goto script;
    case 0x4389:                                /* 8Eh: stops voice AH */
        MOV_AL(RB(gVoiceIndex));
        saved = c->ax;
        saved_di = c->di;
        MOV_BL(AH);
        MOV_BH(0);
        MOV_BL(BL << 1);
        c->di = RW(c->bx + gVoiceRecordPtrs);
        SF8(0x05, 0);
        SF8(0x08, 0);
        SF16(0x03, 0);
        c->di = saved_di;
        c->ax = saved;
        WB(gVoiceIndex, AL);
        goto script;
    case 0x43AD:                                /* 90h: the patch AH */
        saved = c->si;
        MOV_CL(RB(gOperatorOffset));
        c->bx = AH;
        c->bx = (u16)(c->bx << 1);
        c->si = RW(c->bx + gPatchTable);
        LoadVoicePatch(c);
        c->si = saved;
        goto script;
    case 0x43C3:                                /* 91h: the pitch slide */
        SF8(0x13, AH);
        lodsb(c);
        xchg_ah_al(c);
        lodsb(c);
        SF16(0x15, c->ax);
        SF16(0x29, L_4F00);
        SF8(0x14, 0xFFu);
        goto script;
    case 0x43D9:                                /* 92h: no pitch slide */
        c->si--;
        SF16(0x29, L_4E7D);
        SF16(0x15, 0);
        goto script;
    case 0x43E7:                                /* 93h: fine tune */
        SF8(0x24, AH);
        goto script;
    case 0x43ED:                                /* 95h: the oscillator */
        SF8(0x1D, AH);
        lodsb(c);
        SF8(0x19, AL);
        lodsb(c);
        SF8(0x1A, AL);
        SF8(0x1A, F8(0x1A) + 1u);
        MOV_AL(AL << 1);
        SF8(0x1B, AL);
        lodsb(c);
        SF8(0x1C, AL);
        SF16(0x29, L_4E7E);
        goto script;
    case 0x440C:                                /* 9Ah: priority */
        SF8(0x08, AH);
        goto script;
    case 0x4412:                                /* 9Ch: tempo */
        MOV_AH(AH >> 1);
        WB(gTempoReload, AH);
        WB(gTempoCounter, AH);
        WB(gTempoAccum, 0xFFu);
        MOV_AH(0);
        WB(gTempoBeat, AH);
        WB(gTempoWait, AH);
        goto script;
    case 0x442E:                                /* 9Dh: waits for a beat */
        MOV_AL(RB(gTempoWait));
        if (AL == 0) {
            MOV_AH(AH & RB(gTempoBeat));
            if (AH == 0) {
                WB(gTempoWait, RB(gTempoWait) + 1u);
            }
        } else {
            MOV_AH(AH & RB(gTempoBeat));    /* L_4441 */
            if (AH != 0) {
                WB(gTempoWait, 0);
                goto script;
            }
        }
        c->si = (u16)(c->si - 2u);              /* L_444F: the command again next tick */
        SF8(0x05, 1);
        SF16(0x03, c->si);
        goto next;
    case 0x445E:                                /* 9Eh: the level offset */
        SF8(0x20, AH);
        WriteVoiceReg(c);
        goto script;
    case 0x4467:                                /* A0h: a note's duration */
        StartVoiceNote(c);
        SF16(0x03, c->si);
        goto duration;
    case 0x447A:                                /* A1h: a key-on for a duration */
        StartVoiceNote(c);
        VoiceKeyOn(c);
        SF16(0x03, c->si);
        goto duration;
    case 0x4490:                                /* A4h: the key-off multiplier */
        MOV_AH(AH & 7u);
        SF8(0x2D, AH);
        goto script;
    case 0x4499:                                /* A6h: the tempo rate */
        WB(gTempoRate, AH);
        goto script;
    case 0x44A0:                                /* A7h: no pitch envelope */
        c->si--;
        SF16(0x2B, L_4E7D);
        goto script;
    case 0x44A9:                                /* A9h: speed */
        SF8(0x25, AH);
        goto script;
    case 0x44AF:                                /* ABh: the second level offset */
        SF8(0x30, AH);
        goto script;
    case 0x44B5:                                /* ACh: the level adjustment of voice AH */
        MOV_AL(RB(gVoiceIndex));
        saved = c->ax;
        saved_di = c->di;
        lodsb(c);
        WB(gVoiceIndex, AH);
        MOV_BL(AH);
        MOV_BH(0);
        MOV_BL(BL << 1);
        c->di = RW(c->bx + gVoiceRecordPtrs);
        SF8(0x00, AL);
        WriteVoiceReg(c);
        c->di = saved_di;
        c->ax = saved;
        WB(gVoiceIndex, AL);
        goto script;
    case 0x44D6:                                /* ADh: adds to the level adjustment of voice AH */
        MOV_AL(RB(gVoiceIndex));
        saved = c->ax;
        saved_di = c->di;
        WB(gVoiceIndex, AH);
        c->bx = AH;
        lodsb(c);
        MOV_BL(BL << 1);
        c->di = RW(c->bx + gVoiceRecordPtrs);
        MOV_AL(AL + F8(0x00));
        SF8(0x00, AL);
        WriteVoiceReg(c);
        c->di = saved_di;
        c->ax = saved;
        WB(gVoiceIndex, AL);
        goto script;
    case 0x44F9:                                /* AEh: amplitude depth (BDh bit 7) */
        MOV_AH(AH & 1u);
        MOV_CL(7);
        MOV_AH(AH << 7);
        MOV_BL(RB(gRegBDDepth));
        MOV_BL(BL & 0x7Fu);
        MOV_BL(BL | AH);
        WB(gRegBDDepth, BL);
        adlib(c, 0xBDu, BL);
        goto script;
    case 0x4517:                                /* AFh: vibrato depth (BDh bit 6) */
        MOV_AH(AH & 1u);
        MOV_CL(6);
        MOV_AH(AH << 6);
        MOV_BL(RB(gRegBDDepth));
        MOV_BL(BL & 0xBFu);
        MOV_BL(BL | AH);
        WB(gRegBDDepth, BL);
        adlib(c, 0xBDu, BL);
        goto script;
    case 0x4535:                                /* B0h: adds to the level offset */
        MOV_AH(AH + F8(0x20));
        SF8(0x20, AH);
        WriteVoiceReg(c);
        goto script;
    case 0x4541:                                /* B3h: silences voice AH */
        MOV_AL(RB(gVoiceIndex));
        saved = c->ax;
        saved_di = c->di;
        if (AH == 0) {
            WB(gEffectMode, 0);
        }
        WB(gVoiceIndex, AH);                    /* L_4550 */
        MOV_BL(AH);
        MOV_BH(0);
        MOV_BL(BL << 1);
        c->di = RW(c->bx + gVoiceRecordPtrs);
        SF8(0x05, 0);
        SF8(0x08, 0);
        SF16(0x03, 0);
        SF8(0x00, 0);
        if (RB(gVoiceIndex) != 9) {
            MOV_BL(AH);
            MOV_BH(0);
            MOV_CL(RB(c->bx + gOperatorOffsets));
            adlib(c, (u8)(0xC0u + RB(gVoiceIndex)), 0);
            adlib(c, (u8)(0x43u + CL), 0x3Fu);
            adlib(c, (u8)(0x83u + CL), 0xFFu);
            adlib(c, (u8)(0xB0u + RB(gVoiceIndex)), 0);
        }
        c->di = saved_di;                       /* L_45A5 */
        c->ax = saved;
        WB(gVoiceIndex, AL);
        goto script;
    case 0x45AD:                                /* B5h: a random F-number offset */
        lodsb(c);
        c->bx = c->ax;
        SoundRandom(c);
        c->ax &= c->bx;
        MOV_CL(F8(0x28));
        MOV_BH(CL);
        MOV_CL(CL & 0x20u);
        MOV_BL(F8(0x27));
        MOV_BH(BH & 0x1Fu);
        c->bx = (u16)(c->bx + c->ax);
        MOV_BH(BH | CL);
        adlib(c, (u8)(0xA0u + RB(gVoiceIndex)), BL);
        adlib(c, (u8)(0xB0u + RB(gVoiceIndex)), BH);
        goto script;
    case 0x45E0:                                /* B6h: no per-tick handler */
        c->si--;
        SF16(0x29, L_4E7D);
        goto script;
    case 0x45E9:                                /* B9h: the note effect amount, the pitch again */
        SF8(0x42, AH);
        MOV_AL(F8(0x41));
        saved = c->ax;
        MOV_CH(AL & 0xF0u);
        MOV_BL(F8(0x07));
        MOV_CH(CH + BL);
        MOV_AL(AL & 0x0Fu);
        MOV_BL(F8(0x12));
        MOV_AL(AL + BL);
        if ((s8)AL >= 12) {
            MOV_AL(AL - 12u);                   /* L_4610 */
            MOV_CH(CH + 0x10u);
        } else if ((s8)AL < 0) {
            MOV_AL(AL + 12u);
            MOV_CH(CH - 0x10u);
        }
        MOV_BL(AL);                             /* L_4615 */
        MOV_BH(0);
        MOV_BL(BL << 1);
        c->ax = RW(c->bx + gFNumbers);
        c->dx = F8(0x24);
        c->ax = (u16)(c->ax + c->dx);
        MOV_CH(CH >> 2);
        MOV_CH(CH & 0x1Cu);
        MOV_CH(CH | AH);
        MOV_CL(AL);
        {
            u16 si;
            /* QUIRK 4630: only `jl`, so an amount of 0 takes the positive path (adds byte 0 of a curve) */
            if ((s8)F8(0x42) >= 0) {
                MOV_BH(0);
                MOV_BL(F8(0x41));
                MOV_BL(BL & 0x0Fu);
                MOV_BL(BL + 2u);
                c->bx = (u16)(c->bx << 1);
                si = RW(c->bx + gLevelCurvePtrs);
                MOV_BH(0);
                MOV_BL(F8(0x42));
                MOV_AL(RB(c->bx + si));
                MOV_AH(0);
                c->cx = (u16)(c->cx + c->ax);
            } else {
                MOV_BH(0);                      /* L_4658 */
                MOV_BL(F8(0x41));
                MOV_BL(BL & 0x0Fu);
                c->bx = (u16)(c->bx << 1);
                si = RW(c->bx + gLevelCurvePtrs);
                MOV_BH(0);
                MOV_BL(F8(0x42));
                MOV_BL((BL ^ 0xFFu) + 1u);
                MOV_AL(RB(c->bx + si));
                MOV_AH(0);
                c->cx = (u16)(c->cx - c->ax);
            }
        }
        MOV_AL(F8(0x28) & 0x20u);               /* L_467A */
        MOV_CH(CH | AL);
        SF8(0x28, CH);
        SF8(0x27, CL);
        adlib(c, (u8)(0xA0u + RB(gVoiceIndex)), CL);
        adlib(c, (u8)(0xB0u + RB(gVoiceIndex)), CH);
        c->ax = saved;
        goto script;
    case 0x46A3:                                /* BAh: speed from the tempo rate */
        c->si--;
        MOV_AL(RB(gTempoRate));
        SF8(0x25, AL);
        goto script;
    case 0x46AD:                                /* BBh: nothing */
        c->si--;
        goto script;
    case 0x46B1:                                /* BCh: the random-delay mask */
        SF8(0x36, AH);
        goto script;
    case 0x46B7:                                /* BDh: adds to the speed, saturating */
        if ((s8)AH >= 0) {
            u16 sum = (u16)(AH + F8(0x25));
            MOV_AH(sum);
            if (sum >= 0x100u) {
                MOV_AH(0xFFu);
            }
        } else {
            MOV_AH(AH + F8(0x25));              /* L_46C4 */
            if (!(AH < F8(0x25))) {
                MOV_AH(1);
            }
        }
        SF8(0x25, AH);                          /* L_46CE */
        goto script;
    case 0x46D4:                                /* BFh: the pitch effect of the INT 08h handler */
        WB(gEffectMode, AH);
        lodsb(c);
        saved = c->si;
        c->bx = AL;
        c->bx = (u16)(c->bx << 1);
        c->bx = RW(c->bx + gCurveTablePtrs);
        WW(gEffectCurveA, c->bx);
        MOV_AL(AL + 1u);
        c->bx = AL;
        c->bx = (u16)(c->bx << 1);
        c->bx = RW(c->bx + gCurveTablePtrs);
        WW(gEffectCurveB, c->bx);
        if (AH == 2) {
            c->bx = 0;
            WW(gEffectIndex, c->bx);
            c->si = RW(gEffectCurveA);
            MOV_AL(RB(c->bx + c->si));
            WB(gEffectPeriod, AL);
            c->si = RW(gEffectCurveB);
            MOV_AL(RB(c->bx + c->si));
            MOV_AH(0xA0u);
            AdlibWrite(c);
        }
        c->si = saved;                          /* L_471C */
        goto script;
    case 0x4720:                                /* C0h: no pitch effect */
        c->si--;
        WB(gEffectMode, 0);
        goto script;
    case 0x4729:                                /* C1h: the rhythm voices' patches */
        {
            u16 p1, p2, p3;
            MOV_AL(RB(gVoiceIndex));
            p1 = c->ax;
            MOV_AL(RB(gOperatorOffset));
            p2 = c->ax;
            p3 = c->si;
            /* QUIRK: LoadVoicePatch writes the record at DI, the voice running the script, not voices 6 to 8 */
            WB(gVoiceIndex, 6);
            c->bx = gOperatorOffsets;
            MOV_BL(BL + 6u);
            MOV_AL(RB(c->bx));
            WB(gOperatorOffset, AL);
            c->dx = 0x388u;
            MOV_CL(RB(gOperatorOffset));
            c->bx = AH;
            c->bx = (u16)(c->bx << 1);
            c->si = RW(c->bx + gPatchTable);
            MOV_AL(RB(c->si + 6u));
            WB(gRhythmLevelA, AL);
            LoadVoicePatch(c);
            c->si = p3;
            lodsb(c);
            p3 = c->si;
            WB(gVoiceIndex, 7);
            c->bx = gOperatorOffsets;
            MOV_BL(BL + 7u);
            MOV_AH(RB(c->bx));
            WB(gOperatorOffset, AH);
            c->dx = 0x388u;
            MOV_CL(RB(gOperatorOffset));
            c->bx = AL;
            c->bx = (u16)(c->bx << 1);
            c->si = RW(c->bx + gPatchTable);
            MOV_AL(RB(c->si + 5u));
            WB(gRhythmLevelA + 2u, AL);
            MOV_AL(RB(c->si + 6u));
            WB(gRhythmLevelA + 1u, AL);
            LoadVoicePatch(c);
            c->si = p3;
            lodsb(c);
            p3 = c->si;
            WB(gVoiceIndex, 8);
            c->bx = gOperatorOffsets;
            MOV_BL(BL + 8u);
            MOV_AH(RB(c->bx));
            WB(gOperatorOffset, AH);
            c->dx = 0x388u;
            MOV_CL(RB(gOperatorOffset));
            c->bx = AL;
            c->bx = (u16)(c->bx << 1);
            c->si = RW(c->bx + gPatchTable);
            MOV_AL(RB(c->si + 5u));
            WB(gRhythmLevelA + 3u, AL);
            MOV_AL(RB(c->si + 6u));
            WB(gRhythmLevelA + 4u, AL);
            LoadVoicePatch(c);
            c->si = p3;
            lodsb(c);
            MOV_AL(AL & 0x2Fu);
            WB(gVoiceRecord0 + 6u * VOICE_RECORD_SIZE + 0x28u, AL);
            MOV_AH(0xB6u);
            AdlibWrite(c);
            lodsb(c);
            MOV_AH(0xA6u);
            AdlibWrite(c);
            lodsb(c);
            MOV_AL(AL & 0x2Fu);
            WB(gVoiceRecord0 + 7u * VOICE_RECORD_SIZE + 0x28u, AL);
            MOV_AH(0xB7u);
            AdlibWrite(c);
            lodsb(c);
            MOV_AH(0xA7u);
            AdlibWrite(c);
            lodsb(c);
            MOV_AL(AL & 0x2Fu);
            WB(gVoiceRecord0 + 8u * VOICE_RECORD_SIZE + 0x28u, AL);
            MOV_AH(0xB8u);
            AdlibWrite(c);
            lodsb(c);
            MOV_AH(0xA8u);
            AdlibWrite(c);
            WB(gRhythmKeyBits, 0x20u);
            c->ax = p2;
            WB(gOperatorOffset, AL);
            c->ax = p1;
            WB(gVoiceIndex, AL);
        }
        goto script;
    case 0x4808:                                /* C2h: rhythm keys, BDh off then on */
        c->dx = 0x388u;
        MOV_AL(0xBDu);
        x86_outb(c->dx, AL);
        AdlibWaitAfterIndex();
        c->dx++;
        xchg_ah_al(c);
        saved = c->ax;
        MOV_AL(AL & 0x1Fu);
        MOV_AL(AL ^ 0xFFu);
        MOV_AL(AL & RB(gRhythmKeyBits));
        MOV_AL(AL | 0x20u);
        x86_outb(c->dx, AL);
        AdlibWaitAfterData();
        c->ax = saved;
        MOV_AL(AL | RB(gRhythmKeyBits));
        WB(gRhythmKeyBits, AL);
        MOV_AL(AL | RB(gRegBDDepth));
        MOV_AL(AL | 0x20u);
        x86_outb(c->dx, AL);
        AdlibWaitAfterData();
        goto script;
    case 0x4838:                                /* C3h: rhythm keys off */
        c->si--;
        MOV_AL(RB(gRegBDDepth));
        WB(gRhythmKeyBits, 0);
        MOV_AL(AL & 0xC0u);
        MOV_AH(0xBDu);
        AdlibWrite(c);
        goto script;
    case 0x484B:                                /* C4h, C5h, C6h: rhythm levels */
    case 0x48EF:
    case 0x4993:
        lodsb(c);
        c->bx = c->ax;
        rhythm_levels(c, target == 0x484B ? 0xC4 : target == 0x48EF ? 0xC5 : 0xC6);
        goto script;
    case 0x4A23:                                /* C7h: the script flag */
        WB(gScriptFlag, AH);
        goto script;
    case 0x4A2A:                                /* C8h: follow the tempo */
        SF8(0x40, AH);
        goto script;
    case 0x4A30:                                /* C9h: the level shifts */
        SF8(0x32, AH);
        lodsb(c);
        SF8(0x33, AL);
        goto script;
    default:
        /* QUIRK 424C: script bytes CAh to F0h index past the table's 4Ah entries, into the library's text */
        tc_fail(75, "E_TEXT 0000:425D: script byte %02Xh (voice %u, script at %04X) jumps through the table to "
                "0000:%04X, which is none of the command handlers", (unsigned)(AL | 0x80u),
                (unsigned)RB(gVoiceIndex), (unsigned)(c->si - 2u), target);
    }

jump:                                           /* L_42D4 (84h, and 81h while its count lasts) */
    MOV_CL(AH);
    lodsb(c);
    xchg_ah_al(c);
    MOV_AL(CL);
    c->si = c->ax;
    goto script;

duration:                                       /* the note's duration decides (4337, 435C, 4470, 4486) */
    MOV_AL(RB(gNoteDuration));
    if (AL == 0) {
        goto script;
    }
    goto handlers;

end_script:                                     /* L_4311 */
    SF8(0x08, 0);
    SF16(0x03, 0);
    if (RB(gVoiceIndex) != 9) {
        VoiceKeyOff(c);
    }
    goto next;

handlers:                                       /* L_425F */
    saved_di = c->di;
    call_handler(c, F16(0x29), 0x29);
    c->di = saved_di;
    call_handler(c, F16(0x2B), 0x2B);
next:                                           /* L_4267 */
    WB(gVoiceIndex, RB(gVoiceIndex) - 1u);
    if ((s8)RB(gVoiceIndex) >= 0) {
        goto voice;
    }
}

/* 0000:3FF4 SoundDriverTick: the queue, the voices, the tempo (DS = CS); keeps CX, DX, DI, SI */
static void SoundDriverTick(cpu *c)
{
    u16 cx = c->cx, dx = c->dx, di = c->di, si = c->si;
    ProcessSoundQueue(c);
    UpdateVoices(c);
    TickTempo(c);
    c->si = si;
    c->di = di;
    c->dx = dx;
    c->cx = cx;
}

/*@ 0000:03A1 */ u32 GetScriptFlag(void)
{
    /* DX = CS, AX = gScriptFlag */
    return ((u32)CSEG << 16) | RB(gScriptFlag);
}

/*@ 0000:03AA */ farptr GetDriverBlock(void)
{
    return TC_MKFP(CSEG, DriverBlock);
}

/* 0000:03B0: the byte at the block's sub-table idx (the word at 0416h + 2 * idx) plus reg */
/*@ 0000:03B0 */ void SetSoundByte(s16 idx, s16 reg, s16 value)
{
    u16 bx = RW((u16)((u16)idx << 1) + DriverBlock);
    WB(bx + (u16)reg, value);
}

/*@ 0000:03CA */ s16 GetSoundByte(s16 idx, s16 reg)
{
    u16 bx = RW((u16)((u16)idx << 1) + DriverBlock);
    return (s16)RB(bx + (u16)reg);
}

/*@ 0000:03E3 */ farptr GetSoundData(s16 idx)
{
    u16 bx = (u16)((u16)((u16)idx << 1) + DriverBlock);
    return TC_MKFP(CSEG, RW(bx));
}

/* 0000:03FA: adds 0416h to each of the 300 words of the block's offset table */
/*@ 0000:03FA */ void RebaseDriverBlock(void)
{
    u16 di = 0, cx;
    for (cx = 0x12Cu; cx != 0; cx--) {
        WW(di + DriverBlock, RW(di + DriverBlock) + DriverBlock);
        di = (u16)(di + 2u);
    }
}

/* 0000:3E66: the AdLib driver's INT 08h handler (PIT at divisor 0555h): the pitch effect's key toggle on
   voice 0, the busy countdown, the driver tick every 0Ch passes; every 30h passes it chains to the
   BIOS handler StartAdlibDriver saved (which sends the EOI), on the others it sends the EOI itself.
   Keeps every register. */
/*@ 0000:3E66 interrupt */ void AdlibTimerIsr(void)
{
    cpu regs = {0, 0, 0, 0, 0, 0};
    cpu *c = &regs;
    if (RB(gEffectMode) != 0) {
        WB(gEffectCounter, RB(gEffectCounter) - 1u);
        if (RB(gEffectCounter) == 0) {
            u16 rec = gVoiceRecord0;
            u8 al = RB(rec + 0x28u), ah;
            al &= 0x1Fu;
            WB(gToggleBit, RB(gToggleBit) ^ 0xFFu);
            ah = (u8)(RB(gToggleBit) & 0x20u);
            al |= ah;
            WB(rec + 0x28u, al);
            x86_outb(0x388u, 0xB0u);
            AdlibWaitAfterIndex();
            x86_outb(0x389u, al);
            AdlibWaitAfterData();
            WB(gEffectCounter, RB(gEffectPeriod));
        }
    }
    if (RB(gBusyCountdown) != 0) {              /* L_3EAF */
        WB(gBusyCountdown, RB(gBusyCountdown) - 1u);
    }
    WB(gTimerPasses12, RB(gTimerPasses12) - 1u);        /* L_3EBA */
    if (RB(gTimerPasses12) == 0) {
        WB(gTimerPasses12, 0x0Cu);
        SoundDriverTick(c);
    }
    WB(gTimerPasses48, RB(gTimerPasses48) - 1u);        /* L_3EC8 */
    if (RB(gTimerPasses48) == 0) {
        WB(gTimerPasses48, 0x30u);
        x86_chain(TC_MKFP(RW(gSavedTimerSeg), RW(gSavedTimerOff)));
        return;
    }
    x86_outb(0x20u, 0x20u);                     /* L_3EDC */
}

/* 0000:3EE5: the sample player's INT 08h handler (PIT at divisor 004Bh): one 4-bit delta of the sample
   per pass, the low nibble of a byte first, to the level written to the AdLib data port; at the end,
   or when gSampleEnable is zero, the PIT back to 0555h and the INT 08h vector back to what StartSample
   saved. QUIRK 3EE5: always sends the EOI itself, never chains. Keeps every register. */
/*@ 0000:3EE5 interrupt */ void SampleTimerIsr(void)
{
    if (RB(gSampleEnable) != 0) {
        u16 ds = RW(gSamplePtrSeg), si = RW(gSamplePtrOff);
        u8 al = x86_lodsb(ds, &si), ah = RB(gSampleLevel);
        if (RB(gSampleNibbles) & 1u) {
            al = (u8)(al >> 4);
            si++;
        }
        si--;                                   /* L_3F0E */
        al &= 0x0Fu;
        if (!(al & 8u)) {
            ah = (u8)(ah + al);
        } else {
            al &= 7u;                           /* L_3F19 */
            ah = (u8)(ah - al);
        }
        WB(gSampleLevel, ah);                   /* L_3F1D */
        x86_outb(0x389u, ah);
        WB(gSampleNibbles, RB(gSampleNibbles) - 1u);
        WW(gSamplePtrOff, si);
        if ((u16)(si - RW(gSampleStart)) < RW(gSampleLength)) {
            x86_outb(0x20u, 0x20u);             /* L_3F6E */
            return;
        }
    }
    x86_outb(0x43u, 0x36u);                     /* L_3F40 */
    x86_outb(0x40u, 0x55u);
    x86_outb(0x40u, 0x05u);
    WW(gSamplePtrOff, 0);
    tc_st_u16(TC_MKFP(0, 0x20u), RW(gSampleSavedOff));
    tc_st_u16(TC_MKFP(0, 0x22u), RW(gSampleSavedSeg));
    WB(gUpdateEnable, 0);
    x86_outb(0x20u, 0x20u);                     /* L_3F6E */
}

/* 0000:3F77: the AdLib driver on: the sound table, INT 08h to 3E66 (the BIOS vector saved), the voices
   reset, the PIT at divisor 0555h, the speaker gate off, the queue emptied */
/*@ 0000:3F77 */ void StartAdlibDriver(void)
{
    cpu regs = {0, 0, 0, 0, 0, 0};
    cpu *c = &regs;
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u8 al;
    WW(gSoundTable, RW(gDriverTablePtrs));
    r.ax = 0x3508u;
    x86_int(0x21, &r);
    WW(gSavedTimerSeg, r.es);
    WW(gSavedTimerOff, r.bx);
    r.dx = AdlibTimerIsrOff;
    r.ds = CSEG;
    r.ax = 0x2508u;
    x86_int(0x21, &r);
    ResetVoices(c);
    x86_outb(0x43u, 0x36u);
    x86_outb(0x40u, 0x55u);
    x86_outb(0x40u, 0x05u);
    al = x86_inb(0x61u);
    x86_outb(0x61u, (u8)(al & 0xFEu));
    WW(gQueueRead, 0);
    WW(gQueueWrite, 0);
}

/* 0000:3FCA: the AdLib driver off: INT 08h back to the saved vector, the voices reset, the PIT back to
   divisor 0000h (the BIOS rate) */
/*@ 0000:3FCA */ void StopAdlibDriver(void)
{
    cpu regs = {0, 0, 0, 0, 0, 0};
    cpu *c = &regs;
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.dx = RW(gSavedTimerOff);
    r.ax = RW(gSavedTimerSeg);
    r.ds = r.ax;
    r.ax = 0x2508u;
    x86_int(0x21, &r);
    ResetVoices(c);
    x86_outb(0x43u, 0x36u);
    x86_outb(0x40u, 0);
    x86_outb(0x40u, 0);
}

/* 0000:400C: the sound number to the ring at the write index, which steps (masked with 0Fh). QUIRK 401B:
   the store is a word, so at index 15 it also writes the first byte of gVoiceRecord0 (3B93). */
/*@ 0000:400C */ void QueueSound(s16 sound)
{
    u16 bx = RW(gQueueWrite);
    WW(bx + gQueueRing, sound);
    bx = (u16)((bx + 1u) & 0x0Fu);
    WW(gQueueWrite, bx);
}

/* 0000:409E: gEffectIndex = the argument; with bit 15 cleared it indexes the effect's curves: the period
   from gEffectCurveA, register A0h from gEffectCurveB. The ninth entry of the entry table (3944). */
/*@ 0000:409E */ void SetNote(u16 note)
{
    cpu regs = {0, 0, 0, 0, 0, 0};
    cpu *c = &regs;
    u16 bx;
    c->ax = note;
    WW(gEffectIndex, c->ax);
    c->ax &= 0x7FFFu;
    bx = c->ax;
    MOV_AL(RB(bx + RW(gEffectCurveA)));
    WB(gEffectPeriod, AL);
    MOV_AL(RB(bx + RW(gEffectCurveB)));
    MOV_AH(0xA0u);
    AdlibWrite(c);
}

/*@ 0000:40CE */ void SetSamplePlaying(u16 playing)
{
    WB(gSampleEnable, playing);
}

/* 0000:40E5: gSoundTable = gDriverTablePtrs[argument], returned. QUIRK: no bound; an argument of 2 or more
   reads past the two entries (2 reads gScriptFlag and the code at 03A1) */
/*@ 0000:40E5 */ u16 SelectDriverTable(u16 n)
{
    u16 bx = RW((u16)(n << 1) + gDriverTablePtrs);
    WW(gSoundTable, bx);
    return RW(gSoundTable);
}

/* 0000:4102: 0 the driver's segment, 1 and 2 gSoundTable, 3 gPatchTable, 4 gCurveTablePtrs. QUIRK: any
   other argument returns the AX the caller left; C has no such value, and the port returns 0 (no C
   calls it; its callers, if any, are in the driver block [H]). */
/*@ 0000:4102 */ u16 GetDriverTable(u16 n)
{
    u16 ax = 0;
    if (n == 0) {
        return CSEG;
    }
    if (n == 1) {
        return RW(gSoundTable);
    }
    if (n == 2) {
        ax = RW(gSoundTable);
    }
    if (n == 3) {
        ax = gPatchTable;
    }
    if (n == 4) {
        ax = gCurveTablePtrs;
    }
    return ax;
}

/* 0000:4B7F: the AdLib sample player on: voice 0 set up for samples, a start-up delay polled on PIT
   channel 0 (until it has counted 952h down), then INT 08h to 3EE5 (the vector saved) with the PIT at
   divisor 004Bh. The sample is at `sample`, `length` bytes. The delay waits on the PIT's count, which
   the port's clock moves only with the C units' instructions (playbook 80): no C calls this routine
   (the entry table at 3944 names it). */
/*@ 0000:4B7F */ void StartSample(farptr sample, u16 length)
{
    cpu regs = {0, 0, 0, 0, 0, 0};
    cpu *c = &regs;
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 bx, cx;
    x86_outb(0x43u, 0x36u);
    x86_outb(0x40u, 0);
    x86_outb(0x40u, 0);
    adlib(c, 0x20u, 0x21u);
    adlib(c, 0x60u, 0xF0u);
    adlib(c, 0x80u, 0xF0u);
    adlib(c, 0xC0u, 0x01u);
    adlib(c, 0xE0u, 0x00u);
    adlib(c, 0x43u, 0x3Fu);
    adlib(c, 0xB0u, 0x00u);
    adlib(c, 0xA0u, 0x00u);
    adlib(c, 0xA0u, 0x8Fu);
    adlib(c, 0xB0u, 0x2Eu);
    x86_outb(0x43u, 0);
    bx = x86_inb(0x40u);
    bx = (u16)(bx | (x86_inb(0x40u) << 8));
    do {                                        /* L_4BD8 */
        x86_outb(0x43u, 0);
        cx = x86_inb(0x40u);
        cx = (u16)(cx | (x86_inb(0x40u) << 8));
        cx = (u16)(bx - cx);
    } while (cx < 0x952u);
    adlib(c, 0xB0u, 0x20u);
    adlib(c, 0xA0u, 0x00u);
    WW(gSamplePtrOff, TC_OFF(sample));
    WW(gSampleStart, TC_OFF(sample));
    WW(gSamplePtrSeg, TC_SEG(sample));
    WW(gSampleLength, length);
    WB(gSampleLevel, 8);
    WB(gSampleNibbles, 0);
    WB(gSampleEnable, 1);
    x86_outb(0x388u, 0x40u);
    AdlibWaitAfterIndex();
    x86_outb(0x389u, 0);
    AdlibWaitAfterData();
    r.ax = 0x3508u;
    x86_int(0x21, &r);
    WW(gSampleSavedSeg, r.es);
    WW(gSampleSavedOff, r.bx);
    r.dx = SampleTimerIsrOff;
    r.ds = CSEG;
    r.ax = 0x2508u;
    x86_int(0x21, &r);
    x86_outb(0x43u, 0x36u);
    AdlibWaitAfterIndex();
    x86_outb(0x40u, 0x4Bu);
    AdlibWaitAfterIndex();
    x86_outb(0x40u, 0);
}

/*@ 0000:4CAC */ u16 GetSampleOffset(void)
{
    return RW(gSamplePtrOff);
}

/* 0000:4CB1: the note delay (+05h) of the voice given */
/*@ 0000:4CB1 */ u16 GetVoiceDelay(u16 voice)
{
    u16 bx = (u16)((voice & 0xFFu) << 1);
    u16 di = RW(bx + gVoiceRecordPtrs);
    return RB(di + 5u);
}
