/* E1BF3.c: src/E1BF3.ASM in C (playbook 80, M3): the cursor's placement. PlacePointer (1BF3:000E,
   near, CX and DX in registers: a static function with them as parameters) clamps the position to the
   limits at ds:814C..8152, moves the driver's cursor when a handler is installed, and redraws the
   game's pointer when it moved; MovePointerTo (1BF3:0087) and MovePointerBy (1BF3:00A8) call it under
   the busy flag.

   Written from the TASM source, which stays the reference: each routine follows its labels. DS is the
   caller's throughout (tc_ds). The byte at 1BF3:000D (_PadBeforePlacePointer) is data: it has no C. */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (off)))
#define ST16(seg, off, v) tc_st_u16(TC_MKFP((seg), (off)), (u16)(v))

/* the DGROUP variables (the source addresses them by number; names from src/E1A72.ASM) */
#define gRestoreDrawState 0x80A2u
#define gSaveDrawState 0x80AAu
#define gPointerBusy 0x812Eu
#define gCursorX 0x8138u
#define gCursorY 0x813Au
#define gMouseXDoubled 0x8144u
#define gMouseHandlerInstalled 0x8148u
#define gCursorMinX 0x814Cu
#define gCursorMaxX 0x814Eu
#define gCursorMinY 0x8150u
#define gCursorMaxY 0x8152u
#define gDrawnX 0x815Cu
#define gDrawnY 0x815Eu
#define gEgaRegShadowOff 0x8172u
#define gEgaRegShadowSeg 0x8174u

/* `push [8174h]; push [8172h]; call dword ptr [80AAh]; add sp,4` */
static void save_draw_state(u16 ds)
{
    farptr fn = tc_ld_u32(TC_MKFP(ds, gSaveDrawState));
    farptr arg = TC_MKFP(LD16(ds, gEgaRegShadowSeg), LD16(ds, gEgaRegShadowOff));
    tc_fn c = tc_code_find(fn);
    if (c != 0) {
        ((void (*)(farptr))c)(arg);
    } else {
        u8 b[4];
        tc_va32(b, (u32)arg);
        (void)tc_code_far(fn, b, 4, "1BF3:006E");
    }
}

static void restore_draw_state(u16 ds)
{
    farptr fn = tc_ld_u32(TC_MKFP(ds, gRestoreDrawState));
    tc_fn c = tc_code_find(fn);
    if (c != 0) {
        c();
    } else {
        (void)tc_code_far(fn, 0, 0, "1BF3:0082");
    }
}

/* 1BF3:000E: place the cursor at (cx, dx), clamped (signed), and redraw the pointer if it moved */
/*@ 1BF3:000E */ static void PlacePointer(u16 ds, u16 cx, u16 dx)
{
    tc_regs r = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if ((s16)cx < (s16)LD16(ds, gCursorMinX)) {
        cx = LD16(ds, gCursorMinX);                     /* clampedx */
    } else if ((s16)cx > (s16)LD16(ds, gCursorMaxX)) {
        cx = LD16(ds, gCursorMaxX);
    }
    ST16(ds, gCursorX, cx);                             /* storex */
    if ((s16)dx < (s16)LD16(ds, gCursorMinY)) {
        dx = LD16(ds, gCursorMinY);                     /* clampedy */
    } else if ((s16)dx > (s16)LD16(ds, gCursorMaxY)) {
        dx = LD16(ds, gCursorMaxY);
    }
    ST16(ds, gCursorY, dx);                             /* storey */
    if (LD16(ds, gMouseHandlerInstalled) != 0) {
        if (LD16(ds, gMouseXDoubled) == 1) {
            cx = (u16)(cx << 1);
        }
        r.ax = 4;                                       /* setpos */
        r.cx = cx;
        r.dx = dx;
        x86_int(0x33, &r);
    }
    if (LD16(ds, gDrawnX) == LD16(ds, gCursorX) && LD16(ds, gDrawnY) == LD16(ds, gCursorY)) {
        return;                                         /* checkmove: done */
    }
    save_draw_state(ds);                                /* redraw */
    HidePointer();
    ShowPointer();
    restore_draw_state(ds);
}

/* the busy flag at ds:812E: wait while set (only an interrupt could clear it), then take it */
static void acquire(u16 ds)
{
    while (LD16(ds, gPointerBusy) != 0) {
        machine_wait();
    }
    ST16(ds, gPointerBusy, LD16(ds, gPointerBusy) + 1u);
}

/* 1BF3:0087: set the cursor to (x, y) */
/*@ 1BF3:0087 */ void MovePointerTo(s16 x, s16 y)
{
    const u16 ds = tc_ds;

    acquire(ds);                                        /* busyset */
    PlacePointer(ds, (u16)x, (u16)y);
    ST16(ds, gPointerBusy, LD16(ds, gPointerBusy) - 1u);
}

/* 1BF3:00A8: move the cursor by (dx, dy) */
/*@ 1BF3:00A8 */ void MovePointerBy(s16 dx, s16 dy)
{
    const u16 ds = tc_ds;

    acquire(ds);                                        /* busymove */
    PlacePointer(ds, (u16)(dx + LD16(ds, gCursorX)), (u16)(dy + LD16(ds, gCursorY)));
    ST16(ds, gPointerBusy, LD16(ds, gPointerBusy) - 1u);
}
