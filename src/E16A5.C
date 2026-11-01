/* Unit at EOB.EXE 16A5:0009 (3457 bytes, 5 functions): the event dispatch of the node lists
   E1658.C builds. A list item is a `struct Node` (its far `next` at +00, a flags word at +0E, a
   state word at +2E and the box it draws at +1E..+24). DispatchKey (16A5:0009) and
   DispatchKeyWait (16A5:005A) read one input event each, tag it, and hand it to the dispatcher
   16A5:00BD; the dispatcher walks the list under the mouse, updates the state word of the node it
   hits, redraws it through 16A5:07C3 and fires the callback SetNodeCallback (16A5:0D78) installs
   at ds:FC7C. The two key event codes 0x41/0x42 and 0xC6/0xC7 are the mouse buttons: E14EF.C
   names the same pair KEY_CLICK and KEY_CLICK2. Placeholder names. */

/*@flags -ml -O -Z */

struct Node {
    struct Node far *next;      /* +00: next node, or a null pointer at the tail */
    unsigned type;              /* +04: a word whose bit 0x8000 the dispatcher reads */
    unsigned char pad06[8];     /* +06: unknown fields */
    unsigned flags;             /* +0E: the inactive bit and the layout bits E1658.C names */
    unsigned char pad10[0x1E];  /* +10: unknown fields */
    unsigned state;             /* +2E: the on bit, the previous-on bit, others */
    unsigned char pad30[4];     /* +30: unknown fields */
    unsigned char pad34[0x1C];  /* +34: unknown fields */
};

extern int f1AAD_0409(void);   /*@ 1AAD:0409: nonzero when a key is ready to read (E1AAD.ASM) */
extern int f1AAD_0588(void);   /*@ 1AAD:0588: read the ready key (E1AAD.ASM) */
extern int f1AAD_0471(void);   /*@ 1AAD:0471: nonzero when a key is waiting; same as E14EF.C */
extern int f1AAD_05D3(void);   /*@ 1AAD:05D3: read a key; same as E14EF.C */
extern int f1AAD_05F1(int key); /*@ 1AAD:05F1 */

void DispatchNodeEvent(struct Node far *head, int key);

extern void (far *gFC7C)(); /*@ ds:FC7C: the callback 16A5:0D78 installs and 16A5:00BD fires */

/* 16A5:0009: read one pending key event and dispatch it to the node list `head`. 1AAD:0409 says
   a key is ready and 1AAD:0588 reads it; the event gets the 0x8000 tag. A letter 'A' to 'Z' is
   folded to its lower-case code (bit 8 cleared, bit 5 set). The dispatcher is always called, with
   a zero key when nothing was ready, so it can still react to the mouse. No direct caller in the
   resident code or the overlays was found; 16A5:005A is the same dispatch for a waiting key. */
/*@ 16A5:0009 */ void DispatchKey(struct Node far *head)
{
    int key;
    unsigned char code;

    key = 0;
    if (f1AAD_0409()) {
        key = f1AAD_0588();
        key |= 0x8000;
        code = key & 0xff;
        if (code >= 'A' && code <= 'Z') {
            key &= ~0x100;
            key |= 0x20;
        }
    }
    DispatchNodeEvent(head, key);
}

/* 16A5:005A: wait for a key event and dispatch it to the node list `head`. 1AAD:0471 says a key
   is waiting and 1AAD:05D3 reads it; only an event with bit 0x100 is accepted, and three code
   ranges (0x11..0x1a, 0x1f..0x27, 0x2e..0x34) lose that bit before the dispatch. Called once per
   frame by overlay 0 (ovl0:00F7), overlay 1 (ovl1:0416, 097A, 1551, 1ACD, 243A) and 0ACD:0E6C,
   each passing the list head at ds:FB48. */
/*@ 16A5:005A */ void DispatchKeyWait(struct Node far *head)
{
    int key;
    unsigned char code;

    key = 0;
    if (f1AAD_0471()) {
        key = f1AAD_05D3();
        if (key & 0x100) {
            code = key & 0xff;
            if ((code >= 0x11 && code <= 0x1a) || (code >= 0x1f && code <= 0x27) ||
                (code >= 0x2e && code <= 0x34)) {
                key &= ~0x100;
            }
        }
    }
    DispatchNodeEvent(head, key);
}

/*@ 16A5:00BD stub */ void DispatchNodeEvent(struct Node far *head, int key) {}
/*@ 16A5:07C3 stub */ void DrawNode(struct Node far *node) {}

/* 16A5:0D78: store the 32-bit second argument as the far callback at ds:FC7C; the word first
   argument is ignored. main calls it once as (0, 0L) to clear the callback before the game starts
   (E07C8.C:151); the dispatcher 16A5:00BD fires gFC7C for the node under the pointer. */
/*@ 16A5:0D78 */ void SetNodeCallback(int arg, long value)
{
    gFC7C = (void (far *)())value;
}
