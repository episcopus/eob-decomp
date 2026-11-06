/* E1A72.c: src/E1A72.ASM in C (playbook 80, M3): the game's INT 33h glue. InitMouse (1A72:00DA)
   detects the driver and installs the event handler at 1A72:017A, which the driver calls on each
   mouse event; RemoveMouseHandler, IsMouseInRect, the interrupt wrappers and the far pointer at
   ds:8172/8174 that the 17A6 unit saves and restores.

   Written from the TASM source, which stays the reference: each routine follows its labels. The
   mouse state is in DGROUP where the original keeps it, read through the segment each routine reads
   it with: the caller's DS (tc_ds) where the source uses DS as it comes, DGROUP (`mov ax,212Eh`,
   relocated at load) where it loads it.

   Left out: the INT 3Fh handler _Int3FHandler (1A72:0006), a hook around the overlay manager's trap
   that re-finds EOB.EXE. The port runs the overlays as plain code (playbook 80, "The overlays as plain
   code"): there is no INT 3Fh to hook, and main's setvect(0x3F, ...) has nothing to point at.

   The event handler (1A72:017A) takes its arguments in registers (BX the buttons, CX x, DX y), which C
   cannot state. The driver's trampoline (F000:3000, tools/oracle/input.py TRAMPOLINE, written to the
   ROM by port/runtime/mouse.c) loads them from its data block at F000:3080 before its far call, and
   the machine's delivery of a mouse event (port/runtime/irq.c, deliver) writes that block; so the
   block is the handler's register record, and the C reads its registers there. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define SEG 0x1A72u
#define CSEG ((u16)(tc_load_seg + SEG))                 /* the value `push cs` gives */
#define DGROUP ((u16)(tc_load_seg + 0x212Eu))           /* the value `mov ax,212Eh` gives */
#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (off)))
#define ST16(seg, off, v) tc_st_u16(TC_MKFP((seg), (off)), (u16)(v))
#define LD8(seg, off) tc_ld_u8(TC_MKFP((seg), (off)))

/* the handler's entry, inside InitMouse (MouseEventHandler in the source) */
#define MOUSE_EVENT_HANDLER 0x017Au

/* the trampoline's data: AX, BX, CX, DX, SI, DI as it loads them (input.py Mouse.deliver) */
#define EVENT_BX TC_MKFP(MACHINE_ROM_SEG, 0x3082u)
#define EVENT_CX TC_MKFP(MACHINE_ROM_SEG, 0x3084u)
#define EVENT_DX TC_MKFP(MACHINE_ROM_SEG, 0x3086u)

/* the DGROUP variables (src/E1A72.ASM:32-160) */
#define gHitTestX 0x8026u
#define gHitTestY 0x8028u
#define gRestoreDrawState 0x80A2u
#define gSaveDrawState 0x80AAu
#define gRecordMode 0x80ECu                             /* a byte */
#define gPointerBusy 0x812Eu
#define gLastMouseButtons 0x8130u
#define gMiddleButtonBits 0x8132u
#define gUnreadButtonChanges 0x8134u
#define gMouseButtonSeen 0x8136u
#define gCursorX 0x8138u
#define gCursorY 0x813Au
#define gMouseEventX 0x813Cu
#define gMouseEventY 0x813Eu
#define gMouseXDoubled 0x8144u
#define gHideDepth 0x8146u
#define gMouseHandlerInstalled 0x8148u
#define gMouseDisabled 0x814Au
#define gCursorMaxX 0x814Eu
#define gCursorMaxY 0x8152u
#define gDrawnX 0x815Cu
#define gDrawnY 0x815Eu
#define gPointerState 0x8160u
#define gZoneLeft 0x8162u
#define gZoneTop 0x8164u
#define gZoneRight 0x8166u
#define gZoneBottom 0x8168u
#define gEgaRegShadowOff 0x8172u
#define gEgaRegShadowSeg 0x8174u

/* `push [gEgaRegShadowSeg]; push [gEgaRegShadowOff]; call dword ptr [gSaveDrawState]; add sp,4` */
static void save_draw_state(u16 ds, const char *site)
{
    farptr fn = tc_ld_u32(TC_MKFP(ds, gSaveDrawState));
    farptr arg = TC_MKFP(LD16(ds, gEgaRegShadowSeg), LD16(ds, gEgaRegShadowOff));
    tc_fn c = tc_code_find(fn);
    if (c != 0) {
        ((void (*)(farptr))c)(arg);
    } else {
        u8 b[4];
        tc_va32(b, (u32)arg);
        (void)tc_code_far(fn, b, 4, site);
    }
}

/* `call dword ptr [gRestoreDrawState]` */
static void restore_draw_state(u16 ds, const char *site)
{
    farptr fn = tc_ld_u32(TC_MKFP(ds, gRestoreDrawState));
    tc_fn c = tc_code_find(fn);
    if (c != 0) {
        c();
    } else {
        (void)tc_code_far(fn, 0, 0, site);
    }
}

/* 1A72:00DA: initialise the driver and install the handler; returns AX 1 with a driver, 0 without
   (the callers' prototype is void). DS is the caller's (DGROUP). */
/*@ 1A72:00DA */ void InitMouse(void)
{
    const u16 ds = tc_ds;
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    u16 cx, dx;

    r.ax = 0x3533u;                                     /* the INT 33h vector: ES:BX */
    x86_int(0x21, &r);
    ST16(ds, gCursorX, 0xA0u);
    ST16(ds, gCursorY, 0x64u);
    ST16(ds, gHideDepth, 1);
    ST16(ds, gPointerState, 0);
    ST16(ds, gCursorMaxX, 0x13Fu);
    ST16(ds, gCursorMaxY, 0xC7u);
    if ((r.es | r.bx) == 0 || LD8(r.es, r.bx) == 0xCFu) {
        goto none;                                      /* L0118: a null vector or an IRET */
    }
    r.ax = 0;                                           /* L0123: reset */
    x86_int(0x33, &r);
    if (r.ax != 0xFFFFu) {
        goto none;
    }
    ST16(ds, gMouseXDoubled, 0);
    r.ax = 3;                                           /* the position */
    x86_int(0x33, &r);
    if (r.cx != 0xA0u) {
        ST16(ds, gMouseXDoubled, 1);                    /* 640 wide [H] */
    }
    cx = 0xA0u;                                         /* L0144 */
    dx = 0x64u;
    ST16(ds, gMouseHandlerInstalled, 1);
    ST16(ds, gMouseDisabled, 1);
    if (LD16(ds, gMouseXDoubled) == 1) {
        cx = (u16)(cx << 1);
    }
    r.ax = 4;                                           /* L015F: set the position */
    r.cx = cx;
    r.dx = dx;
    x86_int(0x33, &r);
    r.ax = 0x0Cu;                                       /* the handler, for moves and both buttons */
    r.cx = 0x1Fu;
    r.es = CSEG;
    r.dx = MOUSE_EVENT_HANDLER;
    x86_int(0x33, &r);
    return;                                             /* AX = 1 */
none:
    ST16(ds, gMouseHandlerInstalled, 0);                /* AX = 0 */
}

/* 1A72:017A: the INT 33h event handler (MouseEventHandler), far called by the driver's trampoline
   with CX = x, DX = y, BX = buttons (read from its record, above). It keeps every register but BX
   (the trampoline restores BX); DS is DGROUP throughout, so the routines it calls see DGROUP as the
   caller's DS (tc_ds). */
/*@ 1A72:017A */ void MouseEventHandler(void)
{
    const u16 ds = DGROUP;
    const u16 caller_ds = tc_ds;
    u16 bx = tc_ld_u16(EVENT_BX), cx = tc_ld_u16(EVENT_CX), dx = tc_ld_u16(EVENT_DX), ax;

    tc_ds = ds;
    if (LD16(ds, gMouseDisabled) != 0) {
        goto done;                                      /* L01E9 */
    }
    if (LD16(ds, gMouseXDoubled) == 1) {
        cx = (u16)(cx >> 1);
    }
    if (cx > 0x13Fu) {                                  /* L0195 */
        cx = 0x13Fu;
    }
    ST16(ds, gMouseEventX, cx);                         /* L019E */
    ST16(ds, gMouseEventY, dx);
    ax = LD16(ds, gMiddleButtonBits);
    if (ax != 0 && (bx & 4u)) {
        /* QUIRK 1A72:01A6: the middle bit replaces BX by (BX and 3) or the mask */
        bx = (u16)((bx & 3u) | ax);
    }
    if (LD16(ds, gLastMouseButtons) != bx) {            /* L01B8 */
        (void)HandleMouseButtons(bx);
        ax = (u16)((LD16(ds, gLastMouseButtons) ^ bx) | LD16(ds, gUnreadButtonChanges));
        ST16(ds, gUnreadButtonChanges, ax);
        ST16(ds, gLastMouseButtons, bx);
    }
    if (bx != 0) {                                      /* L01D7 */
        ST16(ds, gMouseButtonSeen, 1);
    }
    if (LD16(ds, gPointerBusy) != 0) {                  /* L01E2 */
        goto done;                                      /* L01E9: no position, no drawing */
    }
    ST16(ds, gPointerBusy, 1);                          /* L020C */
    if (LD8(ds, gRecordMode) != 2) {
        ST16(ds, gCursorX, cx);
        ST16(ds, gCursorY, dx);
    }
    cx = LD16(ds, gCursorX);                            /* L0221 */
    if (LD16(ds, gDrawnX) == cx) {
        /* QUIRK 1A72:0229: only this path reloads DX; when x changed the zone test reads the event's y */
        dx = LD16(ds, gCursorY);
        if (LD16(ds, gDrawnY) == dx) {
            goto release;
        }
    }
    if (LD16(ds, gHideDepth) != 0) {                    /* L0235 */
        goto release;
    }
    if (LD16(ds, gPointerState) & 0x8000u) {
        /* QUIRK 1A72:0248: x left of the zone jumps to the y test, so the left edge is never enforced */
        if ((s16)cx >= (s16)LD16(ds, gZoneLeft) && (s16)cx > (s16)LD16(ds, gZoneRight)) {
            goto redraw;
        }
        if ((s16)dx >= (s16)LD16(ds, gZoneTop) && (s16)dx <= (s16)LD16(ds, gZoneBottom)) {  /* L0250 */
            save_draw_state(ds, "1A72:01F4");           /* L01EC: in the zone, hide only */
            HidePointer();
            restore_draw_state(ds, "1A72:01FF");
            ST16(ds, gPointerState, LD16(ds, gPointerState) | 0x4000u);
            goto release;
        }
    }
redraw:                                                 /* L025C */
    save_draw_state(ds, "1A72:0264");
    HidePointer();
    ShowPointer();
    restore_draw_state(ds, "1A72:0274");
release:                                                /* L0279 */
    ST16(ds, gPointerBusy, 0);
done:                                                   /* L027F */
    tc_ds = caller_ds;
}

/* 1A72:0285: remove the handler (INT 33h function 0Ch, mask 0) when one was installed */
/*@ 1A72:0285 */ void RemoveMouseHandler(void)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if (LD16(tc_ds, gMouseHandlerInstalled) == 0) {
        return;                                         /* L02A4 */
    }
    r.ax = 0x0Cu;
    r.cx = 0;
    r.es = CSEG;
    r.dx = MOUSE_EVENT_HANDLER;
    x86_int(0x33, &r);
}

/* 1A72:02A6: 1 when the position the pointer was last drawn at lies in [left, right] x [top, bottom]
   (signed, inclusive); waits for the busy flag and holds it around the test */
/*@ 1A72:02A6 */ s16 IsMouseInRect(s16 left, s16 top, s16 right, s16 bottom)
{
    const u16 ds = tc_ds;
    s16 ax;

    while (LD16(ds, gPointerBusy) != 0) {               /* L02B9 */
        machine_wait();                                 /* only an interrupt could free it */
    }
    ST16(ds, gPointerBusy, LD16(ds, gPointerBusy) + 1u);
    ST16(ds, gHitTestX, LD16(ds, gDrawnX));
    ST16(ds, gHitTestY, LD16(ds, gDrawnY));
    if ((s16)LD16(ds, gHitTestX) < left || (s16)LD16(ds, gHitTestX) > right ||
        (s16)LD16(ds, gHitTestY) < top || (s16)LD16(ds, gHitTestY) > bottom) {
        ax = 0;                                         /* L02ED */
    } else {
        ax = 1;
    }
    ST16(ds, gPointerBusy, LD16(ds, gPointerBusy) - 1u);  /* L02F0 */
    return ax;
}

/* 1A72:02FA and 02FC: `sti` and `cli`. The machine has no interrupt flag: it delivers an interrupt at
   the C-clock position and access the recording gives (playbook 80, "The interrupts and the C
   clock"), where the original, with interrupts closed, did not take one. So both do nothing. */
/*@ 1A72:02FA */ void EnableInterrupts(void)
{
}

/*@ 1A72:02FC */ void DisableInterrupts(void)
{
}

/* 1A72:02FE: point ds:8172/8174 at p ([bp+6] the offset, [bp+8] the segment); when the old segment
   was nonzero, copy 16 bytes from the old target to the new one (under cli ... sti). DS is DGROUP. */
/*@ 1A72:02FE */ void SetFarPointer(farptr p)
{
    const u16 ds = DGROUP, off = TC_OFF(p), seg = TC_SEG(p);
    u16 si = LD16(ds, gEgaRegShadowOff), cx = LD16(ds, gEgaRegShadowSeg), di;

    ST16(ds, gEgaRegShadowOff, off);
    ST16(ds, gEgaRegShadowSeg, seg);
    if (cx == 0) {
        return;                                         /* jcxz L0330 */
    }
    di = off;
    {
        const u16 src = cx;
        cx = 0x10u;
        x86_rep_movsb(src, &si, seg, &di, &cx);
    }
}

/* 1A72:033A: the far pointer at ds:8172/8174 in DX:AX. DS is DGROUP. */
/*@ 1A72:033A */ farptr GetFarPointer(void)
{
    const u16 ds = DGROUP;
    return TC_MKFP(LD16(ds, gEgaRegShadowSeg), LD16(ds, gEgaRegShadowOff));
}
