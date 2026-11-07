/* E1BBE.c: src/E1BBE.ASM in C (playbook 80, M3): the game's own mouse pointer, its hide and show
   halves (HidePointer 1BBE:0002, ShowPointer 1BBE:00ED) and the wrappers that take the busy flag and
   bracket the work with [ds:80AA] and [ds:80A2] (HidePointerZone, ShowPointerZone, LockHidePointer,
   LockShowPointer).

   Written from the TASM source, which stays the reference: each routine follows its labels. Every
   routine reads its variables through DS as it comes (tc_ds): DGROUP from the game's C, and from the
   INT 33h handler, which loads it (E1A72.c sets tc_ds around its calls). The pointer is drawn and
   saved through the far function pointers [ds:804E] (restore), [ds:80AE] (save) and [ds:80C2] (draw):
   a routine registered in C at the address they hold is called in C, any other runs as the original's
   code (tc_code_far). The byte at 1BBE:0001 (_PadBeforeHidePointer) is data, not code: it has no C.
   The `push cs; call near ptr` pairs of the wrappers are far calls of the FAR procs: here plain calls. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (off)))
#define ST16(seg, off, v) tc_st_u16(TC_MKFP((seg), (off)), (u16)(v))

#define SCREEN_COLUMNS 0x28                             /* 40 eight-pixel columns */
#define SCREEN_ROWS 0xC8                                /* 200 rows */
#define COLUMN_MASK 0xFFF8u

/* the DGROUP variables (src/E1BBE.ASM:30-182) */
#define gRestoreRect 0x804Eu
#define gRestoreDrawState 0x80A2u
#define gSaveDrawState 0x80AAu
#define gSaveRect 0x80AEu
#define gDrawShape 0x80C2u
#define gPointerBusy 0x812Eu
#define gCursorX 0x8138u
#define gCursorY 0x813Au
#define gHideDepth 0x8146u
#define gMouseDisabled 0x814Au
#define gPointerHeight 0x8154u
#define gPointerWidth8 0x8156u
#define gPointerHotX 0x8158u
#define gPointerHotY 0x815Au
#define gDrawnX 0x815Cu
#define gDrawnY 0x815Eu
#define gPointerState 0x8160u
#define gZoneLeft 0x8162u
#define gZoneTop 0x8164u
#define gZoneRight 0x8166u
#define gZoneBottom 0x8168u
#define gPointerBackOff 0x816Au
#define gPointerBackSeg 0x816Cu
#define gPointerShapeOff 0x816Eu
#define gPointerShapeSeg 0x8170u
#define gEgaRegShadowOff 0x8172u
#define gEgaRegShadowSeg 0x8174u
#define gRectLeft 0x8176u
#define gRectTop 0x8178u
#define gRectRight 0x817Au
#define gRectBottom 0x817Cu
#define gShapeX 0x817Eu
#define gShapeY 0x8180u
#define gSpriteHeight 0x8182u
#define gNegSpriteHeight 0x8184u
#define gSpriteWidth 0x8186u
#define gNegSpriteWidth 0x8188u

#define W(off) LD16(ds, (off))
#define SW(off) ((s16)LD16(ds, (off)))
#define SET(off, v) ST16(ds, (off), (v))

/* a far call through the pointer at ds:slot with the argument words as pushed (first pushed last) */
static void call_slot(u16 ds, u16 slot, const u8 *args, u16 n, const char *site)
{
    farptr fn = tc_ld_u32(TC_MKFP(ds, slot));
    tc_fn c = tc_code_find(fn);
    if (c == 0) {
        (void)tc_code_far(fn, args, n, site);
    } else if (slot == gRestoreDrawState) {
        c();
    } else if (slot == gSaveDrawState) {
        ((void (*)(farptr))c)(tc_vr32(args));
    } else if (slot == gDrawShape) {
        ((void (*)(s16, farptr, s16, s16))c)((s16)tc_vr16(args), tc_vr32(args + 2), (s16)tc_vr16(args + 6),
                                             (s16)tc_vr16(args + 8));
    } else {
        /* gRestoreRect, gSaveRect: (x, y, w, h, buf) */
        ((void (*)(s16, s16, s16, s16, farptr))c)((s16)tc_vr16(args), (s16)tc_vr16(args + 2),
                                                  (s16)tc_vr16(args + 4), (s16)tc_vr16(args + 6),
                                                  tc_vr32(args + 8));
    }
}

/* `push [gEgaRegShadowSeg]; push [gEgaRegShadowOff]; call dword ptr [gSaveDrawState]; add sp,4` */
static void save_draw_state(u16 ds, const char *site)
{
    u8 b[4];
    tc_va16(b, W(gEgaRegShadowOff));
    tc_va16(b + 2, W(gEgaRegShadowSeg));
    call_slot(ds, gSaveDrawState, b, 4, site);
}

static void restore_draw_state(u16 ds, const char *site)
{
    call_slot(ds, gRestoreDrawState, 0, 0, site);
}

/* the rectangle's words (x, y, w, h, buffer) as 1BBE:00B9 and 1BBE:01B9 push them */
static void rect_args(u16 ds, u8 *b)
{
    tc_va16(b, W(gRectLeft));
    tc_va16(b + 2, W(gRectTop));
    tc_va16(b + 4, (u16)(W(gRectRight) - W(gRectLeft)));
    tc_va16(b + 6, (u16)(W(gRectBottom) - W(gRectTop)));
    tc_va16(b + 8, W(gPointerBackOff));
    tc_va16(b + 10, W(gPointerBackSeg));
}

/* the sprite size copies both halves make (1BBE:002F..0047, 1BBE:012E..0146) */
static void sprite_size(u16 ds)
{
    u16 ax = W(gPointerHeight);
    SET(gSpriteHeight, ax);
    SET(gNegSpriteHeight, (u16)-ax);
    ax = W(gPointerWidth8);
    SET(gSpriteWidth, ax);
    SET(gNegSpriteWidth, (u16)-ax);
}

/* 1BBE:0002 HidePointer: restore the background under the pointer (unless off screen), then
   gHideDepth++ (saturating at FFFFh). Nothing when disabled, or when shown with no buffer. */
/*@ 1BBE:0002 */ void HidePointer(void)
{
    const u16 ds = tc_ds;
    s16 ax;

    if (W(gMouseDisabled) != 0) {
        return;                                         /* HideDone */
    }
    if (W(gHideDepth) != 0) {
        goto count;                                     /* L0075 */
    }
    if (W(gPointerBackSeg) == 0) {
        return;
    }
    SET(gRectLeft, W(gDrawnX) - W(gPointerHotX));
    SET(gRectTop, W(gDrawnY) - W(gPointerHotY));
    sprite_size(ds);
    ax = (s16)(SW(gRectLeft) >> 3);                     /* three `sar ax,1` */
    if (ax <= SW(gNegSpriteWidth) || ax >= SCREEN_COLUMNS) {
        goto count;
    }
    SET(gRectLeft, ax);
    SET(gRectRight, ax + W(gSpriteWidth));
    if (SW(gRectLeft) < 0) {
        SET(gRectLeft, 0);
    } else if (SW(gRectRight) > SCREEN_COLUMNS) {       /* L0077 */
        SET(gRectRight, SCREEN_COLUMNS);
    }
    ax = SW(gRectTop);                                  /* L0084 */
    if (ax <= SW(gNegSpriteHeight) || ax >= SCREEN_ROWS) {
        goto count;
    }
    SET(gRectTop, ax);
    SET(gRectBottom, ax + W(gSpriteHeight));
    if (SW(gRectTop) < 0) {
        SET(gRectTop, 0);
    } else if (SW(gRectBottom) > SCREEN_ROWS) {         /* L00AB */
        SET(gRectBottom, SCREEN_ROWS);
    }
    {                                                   /* L00B9 */
        u8 b[12];
        rect_args(ds, b);
        call_slot(ds, gRestoreRect, b, 12, "1BBE:00D8");
    }
count:                                                  /* L00E0 */
    SET(gHideDepth, W(gHideDepth) + 1u);
    if (W(gHideDepth) == 0) {
        SET(gHideDepth, 0xFFFFu);
    }
}

/* 1BBE:00ED ShowPointer: gHideDepth-- when hidden; at zero with a buffer, the drawn position takes the
   mouse position, and when on screen the background is saved and the shape drawn. */
/*@ 1BBE:00ED */ void ShowPointer(void)
{
    const u16 ds = tc_ds;
    s16 ax;

    if (W(gMouseDisabled) != 0 || W(gHideDepth) == 0) {
        return;                                         /* L015B */
    }
    SET(gHideDepth, W(gHideDepth) - 1u);
    if (W(gHideDepth) != 0 || W(gPointerBackSeg) == 0) {
        return;
    }
    /* QUIRK 1BBE:0111: the drawn position moves even when the rectangle then proves off screen */
    SET(gDrawnX, W(gCursorX));
    SET(gRectLeft, W(gCursorX) - W(gPointerHotX));
    SET(gShapeX, W(gRectLeft));
    SET(gDrawnY, W(gCursorY));
    SET(gRectTop, W(gCursorY) - W(gPointerHotY));
    SET(gShapeY, W(gRectTop));
    sprite_size(ds);
    ax = (s16)(SW(gRectLeft) >> 3);
    if (ax <= SW(gNegSpriteWidth) || ax >= SCREEN_COLUMNS) {
        return;
    }
    SET(gRectLeft, ax);                                 /* L015E */
    SET(gRectRight, ax + W(gSpriteWidth));
    if (SW(gRectLeft) < 0) {
        SET(gRectLeft, 0);
    } else if (SW(gRectRight) > SCREEN_COLUMNS) {       /* L0177 */
        SET(gRectRight, SCREEN_COLUMNS);
    }
    ax = SW(gRectTop);                                  /* L0184 */
    if (ax <= SW(gNegSpriteHeight) || ax >= SCREEN_ROWS) {
        return;
    }
    SET(gRectTop, ax);
    SET(gRectBottom, ax + W(gSpriteHeight));
    if (SW(gRectTop) < 0) {
        SET(gRectTop, 0);
    } else if (SW(gRectBottom) > SCREEN_ROWS) {         /* L01AB */
        SET(gRectBottom, SCREEN_ROWS);
    }
    {                                                   /* L01B9 */
        u8 b[12];
        rect_args(ds, b);
        call_slot(ds, gSaveRect, b, 12, "1BBE:01D8");
        tc_va16(b, 0);                                  /* mode 0 */
        tc_va16(b + 2, W(gPointerShapeOff));
        tc_va16(b + 4, W(gPointerShapeSeg));
        tc_va16(b + 6, W(gShapeX));
        tc_va16(b + 8, W(gShapeY));
        call_slot(ds, gDrawShape, b, 10, "1BBE:01F4");
    }
}

/* the busy flag: wait while set (only an interrupt could clear it), then take it */
static void acquire(u16 ds)
{
    while (W(gPointerBusy) != 0) {
        machine_wait();
    }
    SET(gPointerBusy, W(gPointerBusy) + 1u);
}

static void release(u16 ds)
{
    SET(gPointerBusy, W(gPointerBusy) - 1u);
}

/* 1BBE:01FE HidePointerZone: hide the pointer over the zone (left, top, right, bottom in pixels) and
   remember it, under the busy flag. */
/*@ 1BBE:01FE */ void HidePointerZone(s16 left, s16 top, s16 right, s16 bottom)
{
    const u16 ds = tc_ds;
    u16 ax, bx;

    acquire(ds);                                        /* L0213 */
    if (W(gPointerState) & 0x8000u) {
        /* a zone hide in force: count it in the low byte, saturating at FFh, and hide again */
        u16 st = W(gPointerState);
        u8 al = (u8)(st + 1u);
        if (al == 0) {
            al = (u8)(al - 1u);
        }
        SET(gPointerState, (st & 0xFF00u) | al);        /* L022F */
        goto hide;
    }
    SET(gZoneLeft, left);                               /* L0234 */
    SET(gZoneTop, top);
    SET(gZoneRight, right);
    SET(gZoneBottom, bottom);
    ax = (u16)((W(gDrawnX) - W(gPointerHotX)) & COLUMN_MASK);
    SET(gRectLeft, ax);
    bx = (u16)(W(gPointerWidth8) << 3);
    SET(gZoneLeft, W(gZoneLeft) - bx);
    SET(gRectRight, ax + bx);
    ax = (u16)(W(gDrawnY) - W(gPointerHotY));
    SET(gRectTop, ax);
    bx = W(gPointerHeight);
    SET(gZoneTop, W(gZoneTop) - bx);
    SET(gRectBottom, ax + bx);
    SET(gPointerState, 0x8000u);
    if (SW(gRectRight) < left || SW(gRectLeft) > right || SW(gRectBottom) <= top || SW(gRectTop) > bottom) {
        /* QUIRK 1BBE:02B6: a disjoint rectangle hides nothing but leaves 8000h set */
        release(ds);
        return;
    }
    SET(gPointerState, W(gPointerState) | 0x4000u);
hide:                                                   /* L029F */
    save_draw_state(ds, "1BBE:02A7");
    HidePointer();
    restore_draw_state(ds, "1BBE:02B2");
    release(ds);                                        /* L02B6 */
}

/* 1BBE:02C2 ShowPointerZone: undo one HidePointerZone, under the busy flag */
/*@ 1BBE:02C2 */ void ShowPointerZone(void)
{
    const u16 ds = tc_ds;
    u16 ax;

    acquire(ds);                                        /* L02C6 */
    ax = W(gPointerState);
    if ((u8)ax != 0) {
        SET(gPointerState, ax - 1u);
    } else {
        SET(gPointerState, 0);                          /* L02DE */
        if (!(ax & 0x4000u)) {
            release(ds);                                /* L0300: no redraw owed */
            return;
        }
    }
    save_draw_state(ds, "1BBE:02F1");                   /* L02E9 */
    ShowPointer();
    restore_draw_state(ds, "1BBE:02FC");
    release(ds);
}

/* 1BBE:0307 LockHidePointer: HidePointer under the busy flag and the [ds:80AA]/[ds:80A2] bracket */
/*@ 1BBE:0307 */ void LockHidePointer(void)
{
    const u16 ds = tc_ds;

    acquire(ds);                                        /* L030A */
    save_draw_state(ds, "1BBE:031D");
    HidePointer();
    restore_draw_state(ds, "1BBE:0328");
    release(ds);
}

/* 1BBE:0332 LockShowPointer: ShowPointer under the busy flag and the same bracket */
/*@ 1BBE:0332 */ void LockShowPointer(void)
{
    const u16 ds = tc_ds;

    acquire(ds);                                        /* L0335 */
    save_draw_state(ds, "1BBE:0348");
    ShowPointer();
    restore_draw_state(ds, "1BBE:0353");
    release(ds);
}
