/* E1BB1.c: src/E1BB1.ASM in C (playbook 80, M3): SetMousePointerShape (1BB1:0002), the pointer's
   shape, hot spot and save-under buffer.

   Written from the TASM source, which stays the reference: it follows its labels. Its variables are in
   DGROUP, read through DS as it comes (tc_ds). The byte at 1BB1:0001 is the unit's start marker, data:
   it has no C. The size query [ds:80C6] and the bracket [ds:80AA]/[ds:80A2] are far function pointers:
   a routine registered in C at the address they hold is called in C, any other runs as the original's
   code (tc_code_far). */
#include "tcport.h"
#include "ported.h"
#include "x86.h"

#define LD16(seg, off) tc_ld_u16(TC_MKFP((seg), (off)))
#define ST16(seg, off, v) tc_st_u16(TC_MKFP((seg), (off)), (u16)(v))

#define SHAPE_WIDTH_BYTE 2
#define SHAPE_HEIGHT_BYTE 3
#define HEAP_BLOCK 16

/* the DGROUP variables (src/E1BB1.ASM:68-89) */
#define gRestoreDrawState 0x80A2u
#define gSaveDrawState 0x80AAu
#define gSaveRectSize 0x80C6u
#define gPointerBusy 0x812Eu
#define gPointerHeight 0x8154u
#define gPointerWidth8 0x8156u
#define gPointerHotX 0x8158u
#define gPointerHotY 0x815Au
#define gPointerBackOff 0x816Au
#define gPointerBackSeg 0x816Cu
#define gPointerShapeOff 0x816Eu
#define gPointerShapeSeg 0x8170u
#define gEgaRegShadowOff 0x8172u
#define gEgaRegShadowSeg 0x8174u

/* `push [gEgaRegShadowSeg]; push [gEgaRegShadowOff]; call dword ptr [gSaveDrawState]; add sp,4` */
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
        (void)tc_code_far(fn, b, 4, "1BB1:00B1");
    }
}

static void restore_draw_state(u16 ds)
{
    farptr fn = tc_ld_u32(TC_MKFP(ds, gRestoreDrawState));
    tc_fn c = tc_code_find(fn);
    if (c != 0) {
        c();
    } else {
        (void)tc_code_far(fn, 0, 0, "1BB1:00BD");
    }
}

/* `push ax (height); push dx (width); call dword ptr [gSaveRectSize]`: AX, the buffer size */
static u16 save_rect_size(u16 ds, u16 width, u16 height)
{
    farptr fn = tc_ld_u32(TC_MKFP(ds, gSaveRectSize));
    tc_fn c = tc_code_find(fn);
    if (c != 0) {
        return (u16)((s16 (*)(s16, s16))c)((s16)width, (s16)height);
    } else {
        u8 b[4];
        tc_va16(b, width);
        tc_va16(b + 2, height);
        return (u16)tc_code_far(fn, b, 4, "1BB1:006F");
    }
}

/* 1BB1:0002: the pointer's hot spot and shape record; a new save-under buffer when there is none or the
   size changed. Returns AX 1, or 0 when the heap is too small or MemAlloc fails (the callers'
   prototype is void). */
/*@ 1BB1:0002 */ void SetMousePointerShape(s16 hotX, s16 hotY, farptr icon)
{
    const u16 ds = tc_ds;
    const u16 es = TC_SEG(icon);
    u16 si = TC_OFF(icon), ax, dx;
    farptr p;
    s32 largest;

    while (LD16(ds, gPointerBusy) != 0) {               /* acquire */
        machine_wait();                                 /* only an interrupt could free it */
    }
    ST16(ds, gPointerBusy, LD16(ds, gPointerBusy) + 1u);
    HidePointer();
    ST16(ds, gPointerHotX, hotX);
    ST16(ds, gPointerHotY, hotY);
    ST16(ds, gPointerShapeOff, si);
    ST16(ds, gPointerShapeSeg, es);
    ax = tc_ld_u8(TC_MKFP(es, (u16)(si + SHAPE_HEIGHT_BYTE)));
    dx = (u16)(tc_ld_u8(TC_MKFP(es, (u16)(si + SHAPE_WIDTH_BYTE))) + 1u);
    if (LD16(ds, gPointerBackSeg) != 0) {
        if (ax == LD16(ds, gPointerHeight) && dx == LD16(ds, gPointerWidth8)) {
            goto ready;
        }
        /* resize. QUIRK 1BB1:0058: a later failure leaves this freed buffer in ds:816A/816C */
        MemFree(TC_MKFP(LD16(ds, gPointerBackSeg), LD16(ds, gPointerBackOff)));
    }
    ST16(ds, gPointerHeight, ax);                       /* setsize */
    ST16(ds, gPointerWidth8, dx);
    si = (u16)(save_rect_size(ds, dx, ax) + HEAP_BLOCK);
    largest = LargestFreeBlock();
    if ((u16)((u32)largest >> 16) == 0 && (u16)largest < si) {
        goto failed;
    }
    p = MemAlloc((u32)(u16)(si - HEAP_BLOCK), 0);       /* alloc */
    if (TC_SEG(p) == 0) {
        goto failed;
    }
    ST16(ds, gPointerBackOff, TC_OFF(p));
    ST16(ds, gPointerBackSeg, TC_SEG(p));
ready:
    save_draw_state(ds);
    ShowPointer();
    restore_draw_state(ds);
    goto release;                                       /* AX = 1 */
failed:
    /* QUIRK 1BB1:00C6: the show is skipped, so the hide above stays in the count; AX = 0 */
release:
    ST16(ds, gPointerBusy, LD16(ds, gPointerBusy) - 1u);
}
