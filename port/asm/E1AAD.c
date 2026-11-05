/* E1AAD.c: src/E1AAD.ASM in C (playbook 80, M3): the game's keyboard driver. The INT 9 handler
   (1AAD:07C0) and its install and removal, the 256-byte event ring at 1AAD:007C and its readers, the
   mouse-button folding called from the INT 33h handler (1AAD:0B0E), and the record and replay path of
   the input stream (1AAD:0C17, 1AAD:0D26).

   Written from the TASM source, which stays the reference: each routine follows its labels. All the
   unit's state stays in the arena where the original keeps it: its code-segment variables and
   tables (1AAD:000C..0222, 1AAD:066E..0692, 1AAD:09F2..09F4, 1AAD:0C16, 1AAD:0D1A..0D25) through
   CS_VAR, and the DGROUP variables (ds:80E6..80F7, 8136..8143, 8488) through the segment each routine
   reads them with: the caller's DS (tc_ds) where the source uses DS as it comes, DGROUP
   (`mov ax,212Eh`, relocated at load) where it loads it, and the unit's own segment where it sets
   DS = CS. The near routines with register conventions (_TranslateKey, _DequeueKeyEvent,
   _QueueMouseEvent, _SetKeyDownBit, _ProcessKeyEvent, _InjectRecordedKey) are static functions with
   their registers as parameters. The block at 1AAD:0693 (the wrapper the source describes beside
   _GetKeyState) has no entry: nothing calls or points at it, so it has no C. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x1AADu
#define CSEG ((u16)(tc_load_seg + SEG))                 /* the value `mov ax,cs` gives */
#define DGROUP ((u16)(tc_load_seg + 0x212Eu))           /* the value `mov ax,212Eh` gives */
#define RD(off) tc_ld_u16(CS_VAR(SEG, (off)))
#define WR(off, v) tc_st_u16(CS_VAR(SEG, (off)), (u16)(v))
#define RD8(off) tc_ld_u8(CS_VAR(SEG, (off)))
#define WR8(off, v) tc_st_u8(CS_VAR(SEG, (off)), (u8)(v))
#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (off)))
#define ST16(seg, off, v) tc_st_u16(TC_MKFP((seg), (off)), (u16)(v))
#define LD8(seg, off) tc_ld_u8(TC_MKFP((seg), (off)))
#define ST8(seg, off, v) tc_st_u8(TC_MKFP((seg), (off)), (u8)(v))

/* the unit's code-segment variables and tables (src/E1AAD.ASM names) */
#define K_KEY_BIT_MASKS 0x000Cu     /* kKeyBitMasks: the bit for a key's low 3 bits */
#define K_CHAIN_FAR_PTR 0x0048u     /* kChainFarPtr */
#define K_KEY_BUF 0x007Cu           /* kKeyBuf: the 256-byte event ring */
#define K_KEY_BUF_READ 0x017Cu
#define K_KEY_BUF_WRITE 0x017Eu
#define K_MODIFIER_STATE 0x0180u
#define K_SHIFT_FLAGS 0x0182u
#define K_SHIFT_FLAGS_COPY 0x0184u
#define K_SCAN_TO_KEY 0x0186u
#define K_KEY_BIT_TABLE1 0x01DFu
#define K_KEY_BIT_TABLE2 0x01EFu
#define K_KEY_DOWN_BITS 0x01FFu
#define K_KEY_FLAG_204 0x0204u
#define K_KEY_FLAG_206 0x0206u
#define K_KEY_FLAG_207 0x0207u
#define K_KEY_FLAG_20F 0x020Fu
#define K_KEY_FLAG_210 0x0210u
#define K_INT9_SAVED_OFF 0x0211u
#define K_INT9_SAVED_SEG 0x0213u
#define K_INT23_SAVED_OFF 0x0215u
#define K_INT23_SAVED_SEG 0x0217u
#define K_SPECIAL_KEYS 0x0219u
#define K_MODIFIER_OUT 0x0222u
#define K_UNSHIFTED_CODES 0x02DBu
#define K_SHIFTED_CODES 0x0319u
#define K_KEYPAD_CODES 0x0357u
#define K_CTRL_FN_CODES 0x037Au
#define K_KEY_BITS_66E 0x066Eu
#define K_KEY_MASKS_66F 0x066Fu
#define K_SCAN_LIST_677 0x0677u
#define K_MODE_LIST_685 0x0685u
#define K_KEY_MODE_9F2 0x09F2u
#define K_SCAN_CODE_9F3 0x09F3u
#define K_MOUSE_BUTTONS 0x0C16u
#define K_REC_LEN_D1A 0x0D1Au
#define K_REC_KEY_D1E 0x0D1Eu
#define K_REC_COUNT_D20 0x0D20u
#define K_REC_X_D22 0x0D22u
#define K_REC_Y_D24 0x0D24u

/* the DGROUP variables (offsets in whatever segment the routine reads them through) */
#define G_SOUND_ON 0x80E6u
#define G_TUNES_ON 0x80E8u
#define G_KEYBOARD_OPTIONS 0x80EAu
#define G_RECORD_MODE 0x80ECu
#define G_RECORD_HANDLE 0x80EDu
#define G_LAST_EVENT_KEY 0x80EFu
#define G_RECORD_DELAY 0x80F1u
#define G_LAST_EVENT_X 0x80F3u
#define G_LAST_EVENT_Y 0x80F5u
#define G_RECORD_ENDED 0x80F7u
#define G_MOUSE_BUTTON_SEEN 0x8136u
#define G_CURSOR_X 0x8138u
#define G_CURSOR_Y 0x813Au
#define G_MOUSE_EVENT_X 0x813Cu
#define G_MOUSE_EVENT_Y 0x813Eu
#define G_CLICK_X 0x8140u
#define G_CLICK_Y 0x8142u
#define G_TICKS_SINCE_EVENT 0x8488u

/* the ring slot at index i: cs:[i+kKeyBuf] */
#define SLOT(i) ((u16)(K_KEY_BUF + (i)))

/* `mov di,OFFSET kSpecialKeys; mov cx,9; repne scasb; or cx,cx; jne`: nonzero when CX is left
   nonzero. QUIRK: a match on the ninth entry (80h) leaves CX at 0, the same as no match, so 80h is
   never treated as special. */
static int is_special(u8 al)
{
    u16 di = K_SPECIAL_KEYS, cx = 9;
    (void)x86_rep_scasb(0, CSEG, &di, &cx, al, 0);
    return cx != 0;
}

/* repne scasw: AX with the words at ES:DI while not equal; returns ZF */
static int rep_scasw_ne(u16 es, u16 *di, u16 *cx, u16 ax)
{
    int zf = 0;
    while (*cx != 0) {
        (*cx)--;
        zf = LD16(es, *di) == ax;
        *di = (u16)(*di + 2u);
        if (zf) {
            break;
        }
    }
    return zf;
}

/* 1AAD:0223 _TranslateKey: the key code in AL with modifier bits in AH to a key value; returns the
   new AL (AH and every other register are kept). DS = CS throughout. */
static u8 translate_key(u16 ax)
{
    u8 al = (u8)ax, ah = (u8)(ax >> 8), bl, bh;
    u16 bx, di;

    if (al == 0x6Eu) {
        return 0x1Bu;                                       /* f0223_02D4: ESC */
    }
    if (al < 0x3Eu) {
        bx = (u16)(ax & 0x3Fu);
        al = (ah & 1u) ? RD8(K_SHIFTED_CODES + bx) : RD8(K_UNSHIFTED_CODES + bx);
        if (ah & 2u) {                                      /* f0223_024C: control */
            di = (u16)(bx & 7u);
            if (RD8(K_KEY_BIT_TABLE1 + (bx >> 3)) & RD8(K_KEY_BIT_MASKS + di)) {
                al &= 0x1Fu;
            }
        }
        return al;
    }
    if (al >= 0x4Bu) {
        if (al < 0x6Eu) {
            bl = (u8)(al - 0x4Bu);
            if (!(RD(K_MODIFIER_STATE) & 0x200u) && (RD(K_SHIFT_FLAGS_COPY) & 2u)) {
                /* f0223_0294. QUIRK 1AAD:0223: AL 4Bh..59h give an index of F1h..FFh (byte
                   subtractions), past the 20-byte Ctrl-function table */
                bl = (u8)(bl - 0x0Fu);
                return RD8(K_CTRL_FN_CODES + bl);
            }
            return RD8(K_KEYPAD_CODES + bl);                /* f0223_028D */
        }
        if (al >= 0x70u && al <= 0x79u) {                   /* f0223_029E */
            bl = (u8)(al - 0x70u);
            bh = 0xC5u;
            if (ah & 7u) {
                bh = 0x98u;
                if (!(ah & 4u)) {
                    bh = 0xA2u;
                    if (!(ah & 2u)) {
                        bh = 0xACu;
                    }
                }
            }
            return (u8)(bh - bl);                           /* f0223_02C2 */
        }
        return (u8)(al | 0x80u);                            /* f0223_02D0 */
    }
    if (al >= 0x41u) {                                      /* f0223_02C8 */
        return (u8)(al + 0x85u);
    }
    return (u8)(al | 0x80u);
}

/* 1AAD:0D26 _InjectRecordedKey: with the caller's DS; ax is AX on entry, the value is AX on return.
   In state 0 or 1 nothing; otherwise the recorded key (or 0 until the pacing count is reached, or
   when the recording ended) overwrites the ring slot at the read index. */
static u16 inject_recorded_key(u16 ds, u16 ax)
{
    u8 mode = LD8(ds, G_RECORD_MODE);
    if (mode == 0 || mode == 1) {
        return ax;
    }
    if (LD16(ds, G_RECORD_ENDED) == 0 && LD16(ds, G_TICKS_SINCE_EVENT) >= LD16(ds, G_RECORD_DELAY)) {
        ax = LD16(ds, G_LAST_EVENT_KEY);
    } else {
        ax = 0;                                             /* f0D26_0D49 */
    }
    WR(SLOT(RD(K_KEY_BUF_READ)), ax);
    return ax;
}

/* 1AAD:048D _DequeueKeyEvent: the event at ring index di (AX on entry); returns the key (AX). DS is
   DGROUP for the whole routine. In state 2 the ring words read are replaced by ds:80EF/80F3/80F5 and
   the read index is not written back. */
static u16 dequeue_key_event(u16 di)
{
    const u16 ds = DGROUP;
    u16 ax, v;
    u8 al;

    ax = RD(SLOT(di));
    if (LD8(ds, G_RECORD_MODE) == 2) {
        ax = LD16(ds, G_LAST_EVENT_KEY);
    }
    ST16(ds, G_LAST_EVENT_KEY, ax);
    di = (u16)((di + 2u) & 0xFFu);
    al = (u8)ax;
    if (al >= 0x41u && al <= 0x44u) {
        /* a mouse event: its two position words follow; for 41h/42h they are also the click */
        v = RD(SLOT(di));
        if (LD8(ds, G_RECORD_MODE) == 2) {
            v = LD16(ds, G_LAST_EVENT_X);
        }
        ST16(ds, G_LAST_EVENT_X, v);
        di = (u16)((di + 2u) & 0xFFu);
        if (al <= 0x42u) {
            ST16(ds, G_CLICK_X, v);
        }
        v = RD(SLOT(di));
        if (LD8(ds, G_RECORD_MODE) == 2) {
            v = LD16(ds, G_LAST_EVENT_Y);
        }
        ST16(ds, G_LAST_EVENT_Y, v);
        di = (u16)((di + 2u) & 0xFFu);
        if (al <= 0x42u) {
            ST16(ds, G_CLICK_Y, v);
        }
    }
    if (LD8(ds, G_RECORD_MODE) != 2) {                      /* f048D_052B */
        WR(K_KEY_BUF_READ, di);
    }
    return ax;
}

/* 1AAD:0C17 _ProcessKeyEvent, with the caller's DS (AX is kept). State 1 writes the event to the
   recording, state 2 reads the next one from it. */
static void process_key_event(u16 ds)
{
    const farptr rec = TC_MKFP(CSEG, K_REC_KEY_D1E);
    u16 ax;

    if (LD8(ds, G_RECORD_MODE) == 0) {
        return;
    }
    ax = LD16(ds, G_LAST_EVENT_KEY);
    if (LD8(ds, G_RECORD_MODE) == 1) {
        WR(K_REC_KEY_D1E, ax);
        WR(K_REC_COUNT_D20, LD16(ds, G_TICKS_SINCE_EVENT));
        (void)FileWrite((s16)LD16(ds, G_RECORD_HANDLE), rec, tc_ld_u32(CS_VAR(SEG, K_REC_LEN_D1A)));
        ax = RD(K_REC_KEY_D1E);
        /* QUIRK 1AAD:0C5B: a word compare then a byte one, so any AX of 41h or more whose low byte
           is 41h..44h or below (0105h, say) is taken as a mouse event */
        if (ax >= 0x41u && (u8)ax <= 0x44u) {
            WR(K_REC_KEY_D1E, LD16(ds, G_LAST_EVENT_X));
            WR(K_REC_COUNT_D20, LD16(ds, G_LAST_EVENT_Y));
            (void)FileWrite((s16)LD16(ds, G_RECORD_HANDLE), rec, tc_ld_u32(CS_VAR(SEG, K_REC_LEN_D1A)));
        }
        ST16(ds, G_TICKS_SINCE_EVENT, 0);                   /* f0C17_0C88 */
        return;
    }
    if (LD8(ds, G_RECORD_MODE) != 2) {                      /* f0C17_0C91 */
        return;
    }
    if ((u16)FileRead((s16)LD16(ds, G_RECORD_HANDLE), rec, tc_ld_u32(CS_VAR(SEG, K_REC_LEN_D1A))) == 0) {
        ST16(ds, G_RECORD_ENDED, 1);                        /* f0C17_0D12 */
        return;
    }
    ST16(ds, G_RECORD_DELAY, RD(K_REC_COUNT_D20));
    ax = RD(K_REC_KEY_D1E);
    ST16(ds, G_LAST_EVENT_KEY, ax);
    if (ax >= 0x41u && (u8)ax <= 0x44u) {                   /* the same QUIRK as above */
        if ((u16)FileRead((s16)LD16(ds, G_RECORD_HANDLE), tc_fp_add(rec, 4),
                          tc_ld_u32(CS_VAR(SEG, K_REC_LEN_D1A))) == 0) {
            ST16(ds, G_RECORD_ENDED, 1);
            return;
        }
        ax = RD(K_REC_X_D22);
        ST16(ds, G_LAST_EVENT_X, ax);
        ST16(ds, G_CURSOR_X, ax);
        ax = RD(K_REC_Y_D24);
        ST16(ds, G_LAST_EVENT_Y, ax);
        ST16(ds, G_CURSOR_Y, ax);
        ST16(ds, G_MOUSE_BUTTON_SEEN, 1);
    }
    ST16(ds, G_TICKS_SINCE_EVENT, 0);                       /* f0C17_0D0A */
}

/* 1AAD:0BF1 _SetKeyDownBit: the held bit of key code ax set to the mask *bl (BH = 1 on entry, as
   every caller sets BX = 0101h, then BL = 0 for a release); BL is left shifted to the key's bit, as
   the callers read it. Returns CL, the bit's old state. */
static u8 set_key_down_bit(u16 ax, u8 *bl)
{
    u16 di = (u16)(ax >> 3);
    u8 cl = (u8)(ax & 7u), bh, old;
    *bl = (u8)(*bl << cl);
    bh = (u8)~(u8)(1u << cl);
    old = (u8)(RD8(K_KEY_DOWN_BITS + di) & *bl);
    WR8(K_KEY_DOWN_BITS + di, RD8(K_KEY_DOWN_BITS + di) & bh);
    WR8(K_KEY_DOWN_BITS + di, RD8(K_KEY_DOWN_BITS + di) | *bl);
    return old;
}

/* 1AAD:0B9C _QueueMouseEvent: three words (the code ax and the position es:813C/813E) into the
   ring; returns AX, 0 when the ring has no room. */
static u16 queue_mouse_event(u16 es, u16 ax)
{
    u16 di = RD(K_KEY_BUF_WRITE), si = di;
    di = (u16)((di + 6u) & 0xFFu);
    if (si > di || si < RD(K_KEY_BUF_READ)) {
        if (di >= RD(K_KEY_BUF_READ)) {                     /* f0B9C_0BB5 */
            return 0;                                       /* f0B9C_0BEC */
        }
    }
    WR(SLOT(si), ax);                                       /* f0B9C_0BBC */
    si = (u16)((si + 2u) & 0xFFu);
    WR(SLOT(si), LD16(es, G_MOUSE_EVENT_X));
    si = (u16)((si + 2u) & 0xFFu);
    WR(SLOT(si), LD16(es, G_MOUSE_EVENT_Y));
    WR(K_KEY_BUF_WRITE, di);
    return ax;
}

/*@ 1AAD:038E */ s16 ReadKey(void)
{
    u16 ax, si;

    (void)inject_recorded_key(tc_ds, 0);
    for (;;) {                                              /* f038E_0399 */
        ax = RD(K_KEY_BUF_READ);
        if (LD8(tc_ds, G_RECORD_MODE) != 2 && ax == RD(K_KEY_BUF_WRITE)) {
            ax = 0;                                         /* f038E_03ED: the ring is empty */
            break;
        }
        si = ax;
        ax = RD(SLOT(si));
        if (LD8(tc_ds, G_RECORD_MODE) == 2 && ax == 0) {
            break;
        }
        if (!is_special((u8)ax) && !(ax & 0x0800u) && (u8)ax < 0x7Au) {
            break;                                          /* a key: it stays in the ring */
        }
        if ((u8)ax >= 0x41u && (u8)ax <= 0x44u) {           /* f038E_03D3: skip a mouse event's words */
            si = (u16)(si + 4u);
        }
        si = (u16)((si + 2u) & 0xFFu);
        WR(K_KEY_BUF_READ, si);
    }
    WR(K_SHIFT_FLAGS_COPY, RD(K_SHIFT_FLAGS));              /* f038E_03EF */
    if (ax != 0) {
        ax = translate_key(ax);                             /* AL translated, then xor ah,ah */
    }
    return (s16)ax;
}

/*@ 1AAD:0409 */ s16 ReadKeySpecial(void)
{
    u16 ax, si;

    (void)inject_recorded_key(tc_ds, 0);
    for (;;) {                                              /* f0409_0414 */
        ax = RD(K_KEY_BUF_READ);
        if (LD8(tc_ds, G_RECORD_MODE) != 2 && ax == RD(K_KEY_BUF_WRITE)) {
            ax = 0;                                         /* f0409_0458 */
            break;
        }
        si = ax;
        ax = RD(SLOT(si));
        if (LD8(tc_ds, G_RECORD_MODE) == 2 && ax == 0) {
            break;
        }
        if (!is_special((u8)ax) && (u8)ax < 0x7Au) {
            break;
        }
        si = (u16)((si + 2u) & 0xFFu);                      /* f0409_0449 */
        WR(K_KEY_BUF_READ, si);
    }
    WR(K_SHIFT_FLAGS_COPY, RD(K_SHIFT_FLAGS));              /* f0409_045A */
    if (ax != 0) {
        ax = (u16)((ax & 0xFF00u) | translate_key(ax));     /* AH stays the event's */
    }
    return (s16)ax;
}

/*@ 1AAD:0471 */ s16 IsKeyPending(void)
{
    u16 ax = (u16)(RD(K_KEY_BUF_READ) ^ RD(K_KEY_BUF_WRITE));
    return (s16)inject_recorded_key(tc_ds, ax);
}

/*@ 1AAD:0480 */ void FlushKeyBuffer(void)
{
    WR(K_KEY_BUF_WRITE, RD(K_KEY_BUF_READ));
}

/* The three blocking readers wait for the ring to hold an event: the INT 9 handler (or the INT 33h
   handler, through 1AAD:0B0E) fills it while they spin (playbook 80, "The clock with hand-written
   assembly": the machine delivers the interrupts recorded while a routine waits: machine_wait on each
   turn the ring is empty). In state 2 they
   do not wait and take the event with AX as it is: the segment value `mov ax,cs` left (053A, 0588),
   or the previous event's key on a later turn of the loop. In state 2 _DequeueKeyEvent replaces
   what it reads at that index, so the index only matters when the state the caller's DS shows and
   DGROUP's differ. */

/*@ 1AAD:053A */ s16 WaitGameKey(void)
{
    u16 ax = CSEG;

    for (;;) {                                              /* f053A_0543 */
        if (LD8(tc_ds, G_RECORD_MODE) != 2) {
            ax = RD(K_KEY_BUF_READ);
            if (ax == RD(K_KEY_BUF_WRITE)) {
                machine_wait();                             /* the ring is empty: an interrupt fills it */
                continue;
            }
        }
        ax = dequeue_key_event(ax);                         /* f053A_0559 */
        WR(K_SHIFT_FLAGS_COPY, RD(K_SHIFT_FLAGS));
        if (is_special((u8)ax) || (ax & 0x0800u) || (u8)ax >= 0x7Au) {
            continue;
        }
        break;
    }
    ax = (u16)((ax & 0xFF00u) | translate_key(ax));
    process_key_event(tc_ds);
    return (s16)ax;
}

/*@ 1AAD:0588 */ s16 WaitGameKey2(void)
{
    u16 ax = CSEG;

    for (;;) {                                              /* f0588_0593 */
        /* QUIRK 1AAD:0593: DS = CS here, so the state byte read is 1AAD:80EC, not ds:80EC */
        if (RD8(G_RECORD_MODE) != 2) {
            ax = RD(K_KEY_BUF_READ);
            if (ax == RD(K_KEY_BUF_WRITE)) {
                machine_wait();                             /* the ring is empty: an interrupt fills it */
                continue;
            }
        }
        ax = dequeue_key_event(ax);                         /* f0588_05A9 */
        WR(K_SHIFT_FLAGS_COPY, RD(K_SHIFT_FLAGS));
        if (is_special((u8)ax) || (u8)ax >= 0x7Au) {
            continue;
        }
        break;
    }
    ax = (u16)((ax & 0xFF00u) | translate_key(ax));
    process_key_event(tc_ds);                               /* after `pop ds`: the caller's DS */
    return (s16)ax;
}

/*@ 1AAD:05D3 */ s16 WaitKeyEvent(void)
{
    /* AX on entry is the caller's, which C does not have: 0 stands for it (see above) */
    u16 ax = 0;

    for (;;) {                                              /* f05D3_05D3 */
        if (LD8(tc_ds, G_RECORD_MODE) != 2) {
            ax = RD(K_KEY_BUF_READ);
            if (ax == RD(K_KEY_BUF_WRITE)) {
                machine_wait();                             /* the ring is empty: an interrupt fills it */
                continue;
            }
        }
        break;
    }
    ax = dequeue_key_event(ax);                             /* f05D3_05E9 */
    process_key_event(tc_ds);
    return (s16)ax;
}

/*@ 1AAD:05F1 */ s16 GetKeyState(s16 key)
{
    u16 ax = (u16)key, di, bx;
    u8 al;

    WR8(K_KEY_BITS_66E, 0);
    WR(K_MODIFIER_STATE, LD16(DGROUP, G_KEYBOARD_OPTIONS));
    if (!(RD(K_MODIFIER_STATE) & 2u)) {
        for (di = 0x0Eu;;) {                                /* f05F1_061C */
            di = (u16)(di - 1u);
            if ((s16)di < 0) {
                break;
            }
            if ((u8)ax == RD8(K_SCAN_LIST_677 + di)) {
                al = RD8(K_MODE_LIST_685 + di);
                di = (u16)(al >> 3);
                bx = (u16)(al & 7u);
                WR8(K_KEY_BITS_66E, RD8(K_KEY_MASKS_66F + bx) & RD8(K_KEY_DOWN_BITS + di));
                break;
            }
        }
    }
    al = (u8)ax;                                            /* f05F1_064B */
    di = (u16)(al >> 3);
    bx = (u16)(al & 7u);
    al = (u8)(RD8(K_KEY_MASKS_66F + bx) & RD8(K_KEY_DOWN_BITS + di));
    al |= RD8(K_KEY_BITS_66E);
    return (s16)al;                                         /* AH = 0 */
}

/*@ 1AAD:0718 */ void InstallKeyboard(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u8 al, ah;

    al = LD8(0x0040u, 0x17u);                               /* the BIOS shift state */
    if (al & 0x40u) {
        WR(K_SHIFT_FLAGS, RD(K_SHIFT_FLAGS) | 1u);          /* Caps Lock */
    }
    if (al & 0x20u) {
        WR(K_SHIFT_FLAGS, RD(K_SHIFT_FLAGS) | 2u);          /* Num Lock */
    }
    if (al & 2u) {
        WR8(K_KEY_FLAG_204, RD8(K_KEY_FLAG_204) | 0x10u);   /* left Shift */
    }
    ah = (u8)((al & 1u) << 1);                              /* f0718_0744 */
    al = (u8)((u8)(LD8(0x0040u, 0x18u) << 2) & 0x0Cu);
    ah |= al;
    al = LD8(0x0040u, 0x96u);
    if (al & 8u) {
        ah |= 0x40u;
    }
    WR8(K_KEY_FLAG_206, ah);                                /* f0718_0761 */
    if (al & 4u) {
        WR8(K_KEY_FLAG_207, RD8(K_KEY_FLAG_207) | 1u);
    }
    if (al & 2u) {
        WR8(K_KEY_FLAG_20F, 1);
    }
    if (al & 1u) {
        WR8(K_KEY_FLAG_210, 2);
    }
    r.ax = 0x3509u;                                         /* f0718_0784: INT 9 to cs:07C0 */
    x86_int(0x21, &r);
    WR(K_INT9_SAVED_OFF, r.bx);
    WR(K_INT9_SAVED_SEG, r.es);
    r.ds = CSEG;
    r.dx = 0x07C0u;
    r.ax = 0x2509u;
    x86_int(0x21, &r);
    r.ax = 0x3523u;                                         /* INT 23h to the IRET at cs:09F5 */
    x86_int(0x21, &r);
    WR(K_INT23_SAVED_OFF, r.bx);
    WR(K_INT23_SAVED_SEG, r.es);
    r.ds = CSEG;
    r.dx = 0x09F5u;
    r.ax = 0x2523u;
    x86_int(0x21, &r);
}

/* 1AAD:07C0: the INT 9 handler, inside _InstallKeyboard's region. One scancode from port 60h per
   interrupt. A key it takes is acknowledged here (f0718_09CA: the keyboard's 61h strobe, 40:96
   bit 1 cleared, the EOI); otherwise it ends in the saved INT 9 vector (f0718_09BD). */
/*@ 1AAD:07C0 interrupt */ void KeyboardIsr(void)
{
    const u16 cs = CSEG;
    u16 ax, bx, dx, di, si, cx;
    u8 al, ah, bl, bh, cl, ch;

    WR(K_SHIFT_FLAGS, RD(K_SHIFT_FLAGS) & 0xFFFCu);
    if (LD8(0x0040u, 0x17u) & 0x40u) {
        WR(K_SHIFT_FLAGS, RD(K_SHIFT_FLAGS) | 1u);
    }
    if (LD8(0x0040u, 0x17u) & 0x20u) {                      /* f0718_07E1 */
        WR(K_SHIFT_FLAGS, RD(K_SHIFT_FLAGS) | 2u);
    }
    WR(K_MODIFIER_STATE, LD16(DGROUP, G_KEYBOARD_OPTIONS)); /* f0718_07EF */
    dx = RD(K_MODIFIER_STATE);
    /* DS = ES = CS from here */
    ah = 0;
    bl = 1;
    bh = 1;
    /* QUIRK 1AAD:08FA: CH is the interrupted code's CX high byte on the path of a plain scancode
       (the E0 path leaves it 0); C has no interrupted registers, so CH is 0 on both. It only
       matters with Caps Lock or Num Lock on (kShiftFlags bits 0, 1) for the keys of the bit tables. */
    ch = 0;
    cl = 0;
    al = x86_inb(0x60);
    if (al == 0xE1u) {
        WR8(K_KEY_FLAG_210, 3);
    }
    if (RD8(K_KEY_FLAG_210) != 0) {                         /* f0718_0818: an E1 sequence's byte */
        WR8(K_KEY_FLAG_210, RD8(K_KEY_FLAG_210) - 1u);
        if (dx & 0x80u) {
            goto chain;
        }
        goto ack;
    }
    if (al == 0xE0u) {                                      /* f0718_082E */
        WR8(K_KEY_FLAG_20F, 1);
        goto chain;
    }
    if (al & 0x80u) {                                       /* f0718_083B: a break code */
        bl = 0;
        al &= 0x7Fu;
    }
    if (RD8(K_KEY_FLAG_20F) != 0) {                         /* f0718_0843: after an E0 */
        WR8(K_KEY_FLAG_20F, 0);
        WR8(K_KEY_MODE_9F2, 1);
        di = 0x004Cu;
        cx = 0x11u;
        (void)x86_rep_scasb(0, cs, &di, &cx, al, 0);
        al = LD8(cs, (u16)(di + 0x0Fu));
        if (cx == 0) {
            WR8(K_KEY_MODE_9F2, 0);
            goto ack;
        }
        ch = (u8)(cx >> 8);
    } else {
        di = al;                                            /* f0718_086F: ax, AH = 0 */
        al = RD8(K_SCAN_TO_KEY + di);
    }
    WR(K_SCAN_CODE_9F3, di);                                /* f0718_0876 */
    ax = al;
    di = (u16)(ax >> 3);
    cl = (u8)(al & 7u);
    bl = (u8)(bl << cl);
    bh = (u8)~(u8)(bh << cl);
    cl = (u8)(RD8(K_KEY_DOWN_BITS + di) & bl);
    WR8(K_KEY_DOWN_BITS + di, RD8(K_KEY_DOWN_BITS + di) & bh);
    WR8(K_KEY_DOWN_BITS + di, RD8(K_KEY_DOWN_BITS + di) | bl);
    if ((RD8(K_KEY_FLAG_206) & 4u) || (RD8(K_KEY_FLAG_207) & 1u)) {
        ah |= 2u;                                           /* f0718_08AF */
    }
    if (RD8(K_KEY_FLAG_206) & 0x50u) {                      /* f0718_08B2 */
        ah |= 4u;
    }
    if (RD8(K_KEY_MODE_9F2) != 0) {                         /* f0718_08BD */
        WR8(K_KEY_MODE_9F2, 0);
        if (!(dx & 2u)) {
            si = RD(K_SCAN_CODE_9F3);
            al = LD8(cs, (u16)(si + 0x1Fu));
            if ((s16)si >= 0x21) {
                goto out;
            }
        }
    }
    if ((RD8(K_KEY_FLAG_204) & 0x10u) || (RD8(K_KEY_FLAG_206) & 2u)) {
        ah |= 1u;                                           /* f0718_08EE */
    }
    if ((RD(K_SHIFT_FLAGS) & 1u) && (RD8(K_KEY_BIT_TABLE1 + di) & ch)) {
        ah ^= 1u;                                           /* f0718_08F1 */
    }
    if ((RD(K_SHIFT_FLAGS) & 2u) && (RD8(K_KEY_BIT_TABLE2 + di) & ch)) {
        ah ^= 1u;                                           /* f0718_0904 */
    }
out:                                                        /* f0718_0917 */
    WR8(K_MODIFIER_OUT, ah);
    ax = (u16)((ah << 8) | al);
    if (!(dx & 1u) && cl != 0) {
        goto ack;                                           /* a repeat of a key already down */
    }
    if (ax == 0x0668u || ax == 0x064Cu || bl == 0) {        /* f0718_0929 */
        goto chain;
    }
    di = RD(K_KEY_BUF_WRITE);                               /* f0718_093A: into the ring */
    si = di;
    di = (u16)((di + 2u) & 0xFFu);
    if (di != RD(K_KEY_BUF_READ)) {
        WR(SLOT(si), ax);
        WR(K_KEY_BUF_WRITE, di);
    }
    if (is_special(al)) {                                   /* f0718_0959 */
        goto chain;
    }
    if (ax == 0x0420u) {                                    /* S: sound */
        ST16(DGROUP, G_SOUND_ON, LD16(DGROUP, G_SOUND_ON) ^ 1u);
    } else if (ax == 0x0434u) {                             /* M: music */
        ST16(DGROUP, G_TUNES_ON, LD16(DGROUP, G_TUNES_ON) ^ 1u);
    }
    if (al == 0x6Eu && RD(K_CHAIN_FAR_PTR + 2u) != 0) {     /* f0718_098F */
        /* `call dword ptr cs:[kChainFarPtr]`: nothing in src/ writes it, so this never runs */
        farptr p = tc_ld_u32(CS_VAR(SEG, K_CHAIN_FAR_PTR));
        tc_fn fn = tc_code_find(p);
        if (fn != 0) {
            fn();
        } else {
            (void)tc_code_far(p, 0, 0, "1AAD:099B");
        }
    }
    di = 0x0014u;                                           /* f0718_09A0 */
    cx = 0x1Cu >> 1;
    (void)rep_scasw_ne(cs, &di, &cx, ax);
    if (cx == 0) {
        if (!(dx & 4u)) {                                   /* f0718_09B7 */
            goto ack;
        }
        goto chain;
    }
    bx = LD16(cs, (u16)(di + 0x18u));
    if (bx & dx) {
        goto chain;
    }
ack:                                                        /* f0718_09CA */
    al = x86_inb(0x61);
    x86_outb(0x61, (u8)(al | 0x80u));
    x86_outb(0x61, al);
    ST8(0x0040u, 0x96u, LD8(0x0040u, 0x96u) & 0xFDu);
    x86_outb(0x20, 0x20);
    return;
chain:                                                      /* f0718_09BD */
    x86_chain(TC_MKFP(RD(K_INT9_SAVED_SEG), RD(K_INT9_SAVED_OFF)));
}

/* 1AAD:09F5: the INT 23h (Ctrl-C) handler _InstallKeyboard installs, a bare IRET */
/*@ 1AAD:09F5 interrupt */ void CtrlBreakIret(void)
{
}

/*@ 1AAD:09F6 */ void RemoveKeyboard(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    r.ds = RD(K_INT23_SAVED_SEG);
    r.dx = RD(K_INT23_SAVED_OFF);
    r.ax = 0x2523u;
    x86_int(0x21, &r);
    r.ds = RD(K_INT9_SAVED_SEG);
    r.dx = RD(K_INT9_SAVED_OFF);
    r.ax = 0x2509u;
    x86_int(0x21, &r);
}

/* 1AAD:0A1F: queue one input event (the low 12 bits of code); returns it, or 0 (a zero low byte, or
   no room). ES = DGROUP, DS = CS. */
/*@ 1AAD:0A1F */ u16 QueueInputEvent(u16 code)
{
    const u16 es = DGROUP;
    u16 ax = (u16)(code & 0x0FFFu), di, si;
    u8 al = (u8)ax;

    if (al == 0) {
        return 0;
    }
    if (al >= 0x41u && al <= 0x44u) {
        /* QUIRK 1AAD:0A98: for 43h/44h the room test is for one word (DI = SI + 2) and the write
           index moves by one word, but three words are written */
        di = RD(K_KEY_BUF_WRITE);
        si = di;
        di = (u16)((di + (al <= 0x42u ? 6u : 2u)) & 0xFFu);
        if (si > di || si < RD(K_KEY_BUF_READ)) {
            if (di >= RD(K_KEY_BUF_READ)) {
                return 0;
            }
        }
        WR(SLOT(si), ax);
        si = (u16)((si + 2u) & 0xFFu);
        WR(SLOT(si), al <= 0x42u ? LD16(es, G_CURSOR_X) : ax);
        si = (u16)((si + 2u) & 0xFFu);
        WR(SLOT(si), al <= 0x42u ? LD16(es, G_CURSOR_Y) : ax);
        WR(K_KEY_BUF_WRITE, di);
        return ax;
    }
    di = RD(K_KEY_BUF_WRITE);                               /* f0A1F_0AE4 */
    si = di;
    di = (u16)((di + 2u) & 0xFFu);
    if (di == RD(K_KEY_BUF_READ)) {
        return 0;
    }
    WR(SLOT(si), ax);
    WR(K_KEY_BUF_WRITE, di);
    return ax;
}

/* 1AAD:0B0E: from the INT 33h handler (1A72:017A) with the new button state; ES is the caller's DS
   (DGROUP there). Returns AX: the last event queued, or 0. */
/*@ 1AAD:0B0E */ u16 HandleMouseButtons(u16 buttons)
{
    const u16 es = tc_ds;
    u8 dl = (u8)buttons, ch, ah, bl;
    u16 ax = 0;

    ch = (u8)(RD8(K_MOUSE_BUTTONS) ^ dl);
    if (ch & 1u) {
        bl = (dl & 1u) ? 1u : 0u;
        (void)set_key_down_bit(0x41u, &bl);
        ah = RD8(K_MODIFIER_OUT);
        if (RD(K_SHIFT_FLAGS) & 1u) {
            ah |= 1u;
        }
        if (bl == 0) {
            ah |= 8u;                                       /* a release */
        }
        ax = queue_mouse_event(es, (u16)((ah << 8) | 0x41u));
        if (ax == 0) {
            /* QUIRK 1AAD:0B5C: with the ring full, a change of button 2 at the same time is lost */
            WR8(K_MOUSE_BUTTONS, dl);
            return 0;
        }
    }
    if (ch & 2u) {                                          /* f0B0E_0B5E */
        bl = (dl & 2u) ? 1u : 0u;
        (void)set_key_down_bit(0x42u, &bl);
        ah = RD8(K_MODIFIER_OUT);
        if (RD(K_SHIFT_FLAGS) & 1u) {
            ah |= 1u;
        }
        if (bl == 0) {
            ah |= 8u;
        }
        ax = queue_mouse_event(es, (u16)((ah << 8) | 0x42u));
    }
    WR8(K_MOUSE_BUTTONS, dl);                               /* f0B0E_0B8E */
    return ax;
}
