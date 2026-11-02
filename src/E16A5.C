/* Unit at EOB.EXE 16A5:0009 (3457 bytes, 5 functions): the event dispatch of the node lists
   E1658.C builds. A list item is a `struct Node` (its far `next` at +00, a flags word at +0E, a
   state word at +2E and the box it draws at +1E..+24). DispatchKey (16A5:0009) and
   DispatchKeyWait (16A5:005A) read one input event each, tag it, and hand it to the dispatcher
   16A5:00BD; the dispatcher walks the list under the mouse, updates the state word of the node it
   hits, redraws it through 16A5:07C3 and fires the callback SetNodeCallback (16A5:0D78) installs
   at ds:FC7C. The two key event codes 0x41/0x42 and 0xC6/0xC7 are the mouse buttons: E14EF.C
   names the same pair KEY_CLICK and KEY_CLICK2. Placeholder names. The unit was compiled through
   the assembler (the flags line records -B): only that keeps DrawNode's identical restart tails
   (the g8115 and gFC80 save/restore around each kind) as separate copies, where TCC's own
   assembler merges them into one (the e14ef idiom). */

/*@flags -ml -O -Z -B */

struct Region {                 /* 16-byte records at ds:009F (src/E1A29.C) */
    int left;                   /* +0: left edge in 8-pixel columns */
    int top;                    /* +2: top edge in lines */
    int width;                  /* +4: width in 8-pixel columns */
    int height;                 /* +6: height in lines */
    int saved0;
    int saved1;
    int saved2;
    int saved3;
};

typedef void (far *NodeFn)(struct Node far *node);

struct Node {
    struct Node far *next;      /* +00: next node, or a null pointer at the tail */
    unsigned key;               /* +04: the sort key E1658.C compares */
    unsigned key06;             /* +06: the first key word the dispatcher matches */
    unsigned key08;             /* +08: the second key word the dispatcher matches */
    unsigned char kind;         /* +0A: the drawing kind 0..5 DrawNode switches on */
    unsigned char modeB;        /* +0B: the "on" drawing mode DrawNode switches on */
    unsigned char modeC;        /* +0C: the second drawing mode DrawNode switches on */
    unsigned char pad0D;
    unsigned flags;             /* +0E: the inactive bit and the layout bits E1658.C names */
    void far *src10;            /* +10: the default art pointer DrawNode draws */
    void far *src14;            /* +14: the "on" art/callback pointer DrawNode draws */
    void far *src18;            /* +18: the second art/callback pointer DrawNode draws */
    int region;                 /* +1C: index into the region table ds:009F */
    int x;                      /* +1E: box left, in the region's pixel space */
    int y;                      /* +20: box top, in lines */
    int width;                  /* +22: box width */
    int height;                 /* +24: box height */
    unsigned char color26;      /* +26: text foreground, the default pair */
    unsigned char color27;      /* +27: text background/extra, the default pair */
    unsigned char color28;      /* +28: foreground of the "on" pair */
    unsigned char color29;      /* +29: background/extra of the "on" pair */
    unsigned char color2A;      /* +2A: foreground of the third pair */
    unsigned char color2B;      /* +2B: background/extra of the third pair */
    unsigned char color2C;      /* +2C: foreground of the fourth pair */
    unsigned char color2D;      /* +2D: background/extra of the fourth pair */
    unsigned state;             /* +2E: the on bit, the previous-on bit, others */
    void (far *callback30)(struct Node far *node); /* +30: fired when a state op asks */
    unsigned tile34;            /* +34: the tile id DrawNode draws in the default kind */
    unsigned tile36;            /* +36: the tile id DrawNode draws in the switched modes */
};

extern int ReadKeySpecial(void);   /*@ 1AAD:0409: nonzero when a key is ready to read (E1AAD.ASM) */
extern int WaitGameKey2(void);   /*@ 1AAD:0588: read the ready key (E1AAD.ASM) */
extern int IsKeyPending(void);   /*@ 1AAD:0471: nonzero when a key is waiting; same as E14EF.C */
extern int WaitKeyEvent(void);   /*@ 1AAD:05D3: read a key; same as E14EF.C */
extern int GetKeyState(int key); /*@ 1AAD:05F1 */

void DispatchNodeEvent(struct Node far *head, int key);

extern void (far *gFC7C)(); /*@ ds:FC7C: the callback 16A5:0D78 installs and 16A5:00BD fires */

/* The node's +2E state bits DrawNode and the dispatcher read. Bit 0 is E1658.C's NODE_STATE_ON,
   bit 2 the "pressed" mark the dispatcher sets, bits 3 and 4 two smaller marks. */
#define NODE_STATE_ON   0x0001
#define NODE_STATE_4    0x0004
#define NODE_STATE_8    0x0008
#define NODE_STATE_10   0x0010

extern int gPointerHeight;   /*@ ds:8154: sprite height in pixels (E1BBE.ASM) */
extern int gPointerWidth8;   /*@ ds:8156: sprite width in 8-pixel columns (E1BBE.ASM) */
extern int g8115;            /*@ ds:8115: nonzero while a status line is up (src/E082C.C) */
extern int gFC80;            /*@ ds:FC80: a flag DrawNode clears while it draws a kind-1 node */
extern struct Region gRegions[]; /*@ ds:009F */
extern void (far *gDrawText)(void far *text, int x, int y, int color, int extra); /*@ ds:80CA */
extern void (far *gDraw805E)(int tile, int x, int y); /*@ ds:805E */
extern void (far *gDraw8066)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8066 */

extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */
extern void DrawBox(int x1, int y1, int x2, int y2, int colour); /*@ 1A1F:000C */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */

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
    if (ReadKeySpecial()) {
        key = WaitGameKey2();
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
    if (IsKeyPending()) {
        key = WaitKeyEvent();
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

/* 16A5:00BD: the event dispatcher (1798 bytes). Walks the list from `head` through the far next
   pointers and decides, per node, whether the event hits its rectangle (+1E..+24) and what to do
   to its state word (+2E). Working data in DGROUP: ds:5E10/5E12 keep a far node pointer saved
   across calls (the first call clears it), ds:5DE0/5DE2 the node visited last, ds:5DEA the draw
   flags, ds:5DE4/5DE6 the pressed node, ds:5DE8 a "the list is active" flag; ds:5E20 is a mouse
   moved flag. The node's type word +04 carries a 0x8000 bit the dispatcher reads; +06 and +08 are
   two key words; flags +0E has bits 1 and 2; the mouse position is gCursorX/gCursorY (ds:8138/
   813A) and the pointer size ds:8154/8156 (gPointerHeight/gPointerWidth8) is added to the hit
   rectangle. Key event codes 0x41/0x42 and 0xC6/0xC7 are the mouse buttons; key events go through
   GetKeyState. Two jump tables at 16A5:07B3 (g5E1E, four cases) and 16A5:07BB (g5E1C, four cases)
   select the state change. The pointer is hidden with HidePointerZone and shown again with
   ShowPointerZone around the drawing; DrawNode (16A5:07C3) redraws a node and the callback at
   ds:FC7C is fired for the node under the pointer. Long (about 150 lines of C); not yet written.
   Callers: DispatchKey (16A5:0009) and DispatchKeyWait (16A5:005A). [H] The exact per-type
   semantics of the type word's 0x8000 bit and of the key words are not established. */
/*@ 16A5:00BD stub */ void DispatchNodeEvent(struct Node far *head, int key) {}

/* 16A5:07C3: draw one node (1461 bytes). The node's box is at +1E (x), +20 (y), +22 (width) and
   +24 (height); x and y are made absolute through the region table record +1C (the region's
   origin in pixels, plus its width or height when x or y is negative). The drawing kind +0A
   (0..5) picks the path, but the state word +2E gates it: with bit 0 set the `modeB` +0B picks
   the path (its case 1 switches on the kind), with bit 2 set `modeC` +0C does, and only when both
   bits are clear does the kind switch run directly. A kind of 1 blits an art pointer through
   f1A29_000F (src10, src14 or src18), 2 draws text through gDrawText, 3 draws a tile through
   gDraw805E, 4 calls the art pointer as a far callback with the node, and 5 or 0 draws a box
   through DrawBox or gDraw8066. The pointer is hidden with HidePointerZone over the box grown by
   the mouse sprite size and shown again at the end; ds:8115 (the status-line flag) is cleared
   while a node is drawn and ds:FC80 while a kind-1 node is. Callers: ToggleNode (1658:0073),
   DisableNode (1658:0154), EnableNode (1658:017B), ResetNode (1658:01E6), SetNode (1658:02F5) and
   the dispatcher (16A5:00BD). [H] The exact meaning of the four colour pairs and of the two mode
   bytes is not established. */
/*@ 16A5:07C3 */ void DrawNode(struct Node far *node)
{
    int type;
    int st4;
    int st1;
    int modeC;
    int modeB;
    int savedFC80;
    int region;
    int st10;
    int st8;
    int savedAttr;
    int colour;
    int fpX8;
    int fpY;
    int x;
    int y;

    fpX8 = (gPointerWidth8 - 1) * 8;
    fpY = gPointerHeight;
    HidePointerZone(node->x - fpX8, node->y - fpY,
            node->x + fpX8 + node->width - 1, node->y + fpY + node->height - 1);
    type = node->kind;
    modeC = node->modeC;
    modeB = node->modeB;
    st4 = node->state & NODE_STATE_4;
    st1 = node->state & NODE_STATE_ON;
    st10 = node->state & NODE_STATE_10;
    st8 = node->state & NODE_STATE_8;
    region = node->region;
    x = gRegions[region].left * 8;
    if (node->x < 0)
        x += gRegions[region].width * 8;
    x += node->x;
    y = gRegions[region].top * 8;
    if (node->y < 0)
        y += gRegions[region].height * 8;
    y += node->y;

    if (st1) {
        if (modeB == 1) {
            if (type == 1) {
                savedFC80 = gFC80;
                gFC80 = 0;
                f1A29_000F(0, node->src14, x, y, region);
                gFC80 = savedFC80;
            } else if (type == 2) {
                if (!st4) {
                    savedAttr = g8115;
                    g8115 = 0;
                    gDrawText(node->src14, x, y, node->color28, node->color29);
                    g8115 = savedAttr;
                }
            } else if (type == 3) {
                savedAttr = g8115;
                g8115 = 0;
                gDraw805E(node->tile36, x >> 3, y);
                g8115 = savedAttr;
            } else if (type == 4) {
                if (node->src14)
                    ((NodeFn)node->src14)(node);
            }
        } else if (modeB == 2) {
            if (!st4) {
                savedAttr = g8115;
                g8115 = 0;
                DrawBox(x, y, x + node->width - 1, y + node->height - 1, node->color28);
                g8115 = savedAttr;
            }
        } else if (modeB == 3) {
            if (!st4) {
                savedAttr = g8115;
                g8115 = 0;
                gDraw8066(x, y, x + node->width - 1, y + node->height - 1, node->color28);
                g8115 = savedAttr;
            }
        } else if (modeB == 4) {
            if (node->src14)
                ((NodeFn)node->src14)(node);
        }
    }
    if (st4) {
        if (modeC == 1) {
            if (type == 1) {
                savedFC80 = gFC80;
                gFC80 = 0;
                f1A29_000F(0, node->src18, x, y, region);
                gFC80 = savedFC80;
            } else if (type == 2) {
                savedAttr = g8115;
                g8115 = 0;
                if (st1)
                    gDrawText(node->src18, x, y, node->color2C, node->color2D);
                else
                    gDrawText(node->src18, x, y, node->color2A, node->color2B);
                g8115 = savedAttr;
            } else if (type == 3) {
                savedAttr = g8115;
                g8115 = 0;
                gDraw805E(node->tile36, x >> 3, y);
                g8115 = savedAttr;
            } else if (type == 4) {
                if (node->src18)
                    ((NodeFn)node->src18)(node);
            }
        } else if (modeC == 2) {
            savedAttr = g8115;
            g8115 = 0;
            colour = st1 ? node->color2C : node->color2A;
            DrawBox(x, y, x + node->width - 1, y + node->height - 1, colour);
            g8115 = savedAttr;
        } else if (modeC == 3) {
            savedAttr = g8115;
            g8115 = 0;
            colour = st1 ? node->color2C : node->color2A;
            gDraw8066(x, y, x + node->width - 1, y + node->height - 1, colour);
            g8115 = savedAttr;
        } else if (modeC == 4) {
            if (node->src18)
                ((NodeFn)node->src18)(node);
        }
    }
    if (st4 == 0) {
        if (st1 == 0) {
            if (type == 1) {
                savedFC80 = gFC80;
                gFC80 = 0;
                f1A29_000F(0, node->src10, x, y, region);
                gFC80 = savedFC80;
            } else if (type == 2) {
                savedAttr = g8115;
                g8115 = 0;
                gDrawText(node->src10, x, y, node->color26, node->color27);
                g8115 = savedAttr;
            } else if (type == 3) {
                savedAttr = g8115;
                g8115 = 0;
                gDraw805E(node->tile34, x >> 3, y);
                g8115 = savedAttr;
            } else if (type == 4) {
                if (node->src10)
                    ((NodeFn)node->src10)(node);
            } else if (type == 5) {
                savedAttr = g8115;
                g8115 = 0;
                DrawBox(x, y, x + node->width - 1, y + node->height - 1, node->color26);
                g8115 = savedAttr;
            } else if (type == 0) {
                savedAttr = g8115;
                g8115 = 0;
                if (modeB == 2 || modeC == 2)
                    DrawBox(x, y, x + node->width - 1, y + node->height - 1, node->color26);
                else if (modeB == 3 && st8)
                    gDraw8066(x, y, x + node->width - 1, y + node->height - 1, node->color28);
                else if (modeC == 3 && st10)
                    gDraw8066(x, y, x + node->width - 1, y + node->height - 1, node->color2A);
                g8115 = savedAttr;
            }
        }
    }
    ShowPointerZone();
}

/* 16A5:0D78: store the 32-bit second argument as the far callback at ds:FC7C; the word first
   argument is ignored. main calls it once as (0, 0L) to clear the callback before the game starts
   (E07C8.C:151); the dispatcher 16A5:00BD fires gFC7C for the node under the pointer. */
/*@ 16A5:0D78 */ void SetNodeCallback(int arg, long value)
{
    gFC7C = (void (far *)())value;
}
