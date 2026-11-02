/* Unit at EOB.EXE 16A5:0009 (3457 bytes, 5 functions): the event dispatch of the node lists
   E1658.C builds. A list item is a `struct Node` (its far `next` at +00, a flags word at +0E, a
   state word at +2E and the box it draws at +1E..+24). DispatchKey (16A5:0009) and
   DispatchKeyWait (16A5:005A) read one input event each, tag it, and hand it to the dispatcher
   16A5:00BD; the dispatcher walks the list under the mouse, updates the state word of the node it
   hits, redraws it through 16A5:07C3 and fires the callback SetNodeCallback (16A5:0D78) installs
   at ds:FC7C. The two key event codes 0x41/0x42 and 0xC6/0xC7 are the mouse buttons: E14EF.C
   names the same pair KEY_CLICK and KEY_CLICK2. Placeholder names. The unit was compiled through
   the assembler (the flags line records -B): only that keeps DrawNode's identical restart tails
   (the gStatusLine and gFC80 save/restore around each kind) as separate copies, where TCC's own
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
    unsigned width;             /* +22: box width */
    unsigned height;            /* +24: box height */
    unsigned char color26;      /* +26: text foreground, the default pair */
    unsigned char color27;      /* +27: text background/extra, the default pair */
    unsigned char color28;      /* +28: foreground of the "on" pair */
    unsigned char color29;      /* +29: background/extra of the "on" pair */
    unsigned char color2A;      /* +2A: foreground of the third pair */
    unsigned char color2B;      /* +2B: background/extra of the third pair */
    unsigned char color2C;      /* +2C: foreground of the fourth pair */
    unsigned char color2D;      /* +2D: background/extra of the fourth pair */
    unsigned state;             /* +2E: the on bit, the previous-on bit, others */
    int (far *callback30)(struct Node far *node); /* +30: fired when a state op asks, 0 stops the walk */
    int tile34;                 /* +34: the tile id DrawNode draws in the default kind */
    int tile36;                 /* +36: the tile id DrawNode draws in the switched modes */
};

extern int ReadKeySpecial(void);   /*@ 1AAD:0409: nonzero when a key is ready to read (E1AAD.ASM) */
extern int WaitGameKey2(void);   /*@ 1AAD:0588: read the ready key (E1AAD.ASM) */
extern int IsKeyPending(void);   /*@ 1AAD:0471: nonzero when a key is waiting; same as E14EF.C */
extern int WaitKeyEvent(void);   /*@ 1AAD:05D3: read a key; same as E14EF.C */
extern int GetKeyState(int key); /*@ 1AAD:05F1 */

int DispatchNodeEvent(struct Node far *head, int key);

extern void (far *gFC7C)(); /*@ ds:FC7C: the callback 16A5:0D78 installs and 16A5:00BD fires */

/* The node's +2E state bits DrawNode and the dispatcher read. Bit 0 is E1658.C's NODE_STATE_ON,
   bit 2 the "pressed" mark the dispatcher sets, bits 3 and 4 two smaller marks. */
#define NODE_STATE_ON   0x0001
#define NODE_STATE_4    0x0004
#define NODE_STATE_8    0x0008
#define NODE_STATE_10   0x0010

/* The +0E flags word's inactive bit, the same as E1658.C's NODE_INACTIVE. */
#define NODE_INACTIVE   0x0008

void DrawNode(struct Node far *node);

extern int gPointerHeight;   /*@ ds:8154: sprite height in pixels (E1BBE.ASM) */
extern int gPointerWidth8;   /*@ ds:8156: sprite width in 8-pixel columns (E1BBE.ASM) */
extern int gStatusLine;            /*@ ds:8115: nonzero while a status line is up (src/E082C.C) */
extern int gFC80;            /*@ ds:FC80: a flag DrawNode clears while it draws a kind-1 node */
extern struct Region gRegions[]; /*@ ds:009F */
extern void (far *gDrawText)(void far *text, int x, int y, int color, int extra); /*@ ds:80CA */
extern void (far *gDraw805E)(int tile, int x, int y); /*@ ds:805E */
extern void (far *gDraw8066)(int x1, int y1, int x2, int y2, int colour); /*@ ds:8066 */

extern void HidePointerZone(int left, int top, int right, int bottom); /*@ 1BBE:01FE */
extern void ShowPointerZone(void); /*@ 1BBE:02C2 */
extern void DrawBox(int x1, int y1, int x2, int y2, int colour); /*@ 1A1F:000C */
extern void f1A29_000F(int mode, void far *src, int x, int y, int region); /*@ 1A29:000F */

/* The dispatcher's working state in DGROUP. The hit point comes from the mouse (gCursorX/gCursorY)
   or, when a button is down, from the last click (ds:8140/8142). gLastNode and gPressedNode are
   far pointers; ds:5E1C and ds:5E1E carry the per-button mode (1,2,3 or 4) E1658.C also reads. */
extern int gMouseDisabled;   /*@ ds:814A */
extern int gCursorX;         /*@ ds:8138 */
extern int gCursorY;         /*@ ds:813A */
extern int g8140;            /*@ ds:8140: x of the last click (src/E14EF.C) */
extern int g8142;            /*@ ds:8142: y of the last click (src/E14EF.C) */
extern int g5E1C;            /*@ ds:5E1C: the left-button mode (src/E1658.C) */
extern int g5E1E;            /*@ ds:5E1E: the right-button mode (src/E1658.C) */
extern int gHitX;            /*@ ds:5E10: the x the hit test uses */
extern int gHitY;            /*@ ds:5E12: the y the hit test uses */
extern struct Node far *gLastNode;   /*@ ds:5DE0: the list head the last walk drew from */
extern int g5DEA[];          /*@ ds:5DEA: word 0 the drawn-node flags, words 1..4 the g5E1C redraw masks */
extern struct Node far *gPressedNode; /*@ ds:5DE4: the node a press selected, 0 when none */
extern int gListActive;      /*@ ds:5DE8: 0 stops the walk from reacting to the mouse */
extern char g5E20;           /*@ ds:5E20: nonzero when the mouse moved since the last call */
extern int gFC82;            /*@ ds:FC82: which key word matched, 1 or 2, 0 none */
extern int g5DF2[];          /*@ ds:5DF2: words 1..4 the g5E1E redraw masks */

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

/* 16A5:00BD: the event dispatcher (1798 bytes). `head` is the list to walk and `key` one key
   event (0 when only the mouse did something); it returns the event it consumed, or 0. First it
   maps the mouse buttons: the codes 0x41/0x42 and the negative pair 0xC6/0xC7 set ds:5E1C and
   ds:5E1E to a button mode (1 or 3 when bit 0x800 is set, else 2 or 4 by GetKeyState) and move the
   hit point ds:5E10/5E12 from gCursorX/gCursorY to the last click ds:8140/8142. Then it redraws
   the list from `head` with DrawNode unless `head` is still the node in ds:5DE0 and the mouse has
   not moved (ds:5E20), ORing the flags each node reports into ds:5DEA and firing the node callback
   ds:FC7C for a node whose state bit 0x20 is set, that carries a tile (+34/+36) and has the
   callback installed. Finally it walks the list from `head` (or from the pressed node in
   ds:5DE4/ds:5DE6 when there is one): a node with flags bit 8 (NODE_INACTIVE) is skipped; else
   when `key` equals the node's key word +06 or +08 it tags ds:FC82, sets ds:5E1C or ds:5E1E to 1
   and toggles the node's on bit; and when `key` is 0 it hit-tests the node box +1E..+24 against
   the hit point, and for the mode in ds:5E1C (ds:5E1E) selects one of four cases through the jump
   table at 16A5:07BB (16A5:07B3) that ORs the node's flags +0E mask into ds:5DEA and adjusts the
   state's 4 bit, pressed bit and result. It writes the state word +2E and flags +0E back, redraws
   the node when its bits 1/8 differ or its bits 4/0x10 differ, runs the node callback +30 whose
   0/nonzero decides whether the walk goes on, and clears the pressed node when it is released.
   The pointer is hidden and shown around the redraw. Callers: DispatchKey (16A5:0009) and
   DispatchKeyWait (16A5:005A). [H] The exact meaning of the flags bits and button modes, and of
   the array at ds:5DEA, is not established. */
/*@ 16A5:00BD */ int DispatchNodeEvent(struct Node far *head, int key)
{
    int more;
    int result;
    int changed;
    int justPressed;
    int mouseEvent;
    int pressedFlag;
    int flagsLow;
    int flagsHigh;
    int buttonMode;
    int keyCode;
    int mask;
    unsigned char code;
    register int state;
    register int flags;

    mouseEvent = 0;
    gFC82 = 0;
    if (!gMouseDisabled) {
        gHitX = gCursorX;
        gHitY = gCursorY;
        if (GetKeyState(0x41))
            g5E1C = 2;
        else
            g5E1C = 4;
        if (GetKeyState(0x42))
            g5E1E = 2;
        else
            g5E1E = 4;
        code = key & 0xff;
        if (key < 0) {
            if (code >= 0xc6 && code <= 0xc7) {
                if (key & 0x800)
                    buttonMode = 3;
                else
                    buttonMode = 1;
                if (code == 0xc6)
                    g5E1C = buttonMode;
                else
                    g5E1E = buttonMode;
                gHitX = g8140;
                gHitY = g8142;
                mouseEvent = 1;
                key = 0;
            } else {
                key &= 0xff;
            }
        } else if (code >= 0x41 && code <= 0x42) {
            if (key & 0x800)
                buttonMode = 3;
            else
                buttonMode = 1;
            if (code == 0x41)
                g5E1C = buttonMode;
            else
                g5E1E = buttonMode;
            gHitX = g8140;
            gHitY = g8142;
            mouseEvent = 1;
            key = 0;
        }
    }
    result = 0;
    more = 1;
    if (head == 0)
        return key;
    if (head != gLastNode || g5E20 != 0) {
        gLastNode = head;
        g5DEA[0] = 0;
        do {
            DrawNode(head);
            g5DEA[0] |= head->flags & 0xaa04;
            if (head->state & 0x20)
                if (head->tile34 | head->tile36)
                    if (gFC7C)
                        gFC7C(head);
            if (head->next == 0)
                more = 0;
            if (more)
                head = head->next;
        } while (more);
        gPressedNode = 0;
        gListActive = 1;
        g5E20 = 0;
    }
    justPressed = 0;
    if (gPressedNode)
        head = gPressedNode;
    for (;;) {
        if (head->flags & NODE_INACTIVE) {
            more = (head = head->next) != 0;
            goto checkMore;
        }
        changed = 0;
        pressedFlag = 0;
        keyCode = head->key | 0x8000;
        state = head->state;
        flags = head->flags;
        if (state & NODE_STATE_ON)
            state |= NODE_STATE_8;
        else
            state &= ~NODE_STATE_8;
        if (state & NODE_STATE_4)
            state |= NODE_STATE_10;
        else
            state &= ~NODE_STATE_10;
        if (key != 0) {
            if (head->key06 == key) {
                gFC82 = 1;
                g5E1C = 1;
                state ^= NODE_STATE_ON;
                result = keyCode;
                changed = 1;
            } else if (head->key08 == key) {
                gFC82 = 2;
                g5E1E = 1;
                state ^= NODE_STATE_ON;
                result = keyCode;
                changed = 1;
            }
            goto update;
        }
        if (gMouseDisabled)
            goto update;
        if (g5DEA[0] == 0 && mouseEvent == 0)
            goto update;
        flagsLow = flags & 0xf00;
        flagsHigh = flags & 0xf000;
        if (gListActive == 0)
            goto update;
        if (head->x <= gHitX && head->x + head->width > gHitX &&
            head->y <= gHitY && head->y + head->height > gHitY) {
            state |= 2;
            if (flagsLow != 0) {
                switch (g5E1C) {
                case 4:
                    if (!(flags & 4) && head->modeC != 0) {
                        state |= 4;
                    } else {
                        state &= ~4;
                    }
                    if (flags & 0x800) {
                        changed = 1;
                    } else if (flags & 2) {
                        if (state & NODE_STATE_ON)
                            state &= ~NODE_STATE_ON;
                    }
                    break;
                case 2:
                    if ((flags & 4) && head->modeC != 0) {
                        state |= 4;
                        pressedFlag = 1;
                    } else {
                        state &= ~4;
                    }
                    if (flags & 0x200) {
                        changed = 1;
                        if (!(flags & 1)) {
                            state |= NODE_STATE_ON;
                            result = keyCode;
                        }
                    }
                    break;
                case 1:
                    if ((flags & 4) && head->modeC != 0) {
                        state |= 4;
                        pressedFlag = 1;
                    } else {
                        state &= ~4;
                    }
                    if (flags & 0x100) {
                        changed = 1;
                        if (!(flags & 1)) {
                            state ^= NODE_STATE_ON;
                            result = keyCode;
                        }
                        if (flags & 0x40) {
                            gPressedNode = head;
                            justPressed = 1;
                        }
                    }
                    break;
                case 3:
                    if (flags & 0x400) {
                        changed = 1;
                        if (flags & 1) {
                            state ^= NODE_STATE_ON;
                            result = keyCode;
                        }
                    }
                    if (flags & 2) {
                        if (state & NODE_STATE_ON)
                            state &= ~NODE_STATE_ON;
                    }
                    break;
                }
            }
            if (flagsHigh == 0)
                goto update;
            if (changed)
                goto update;
            if (pressedFlag)
                goto update;
            switch (g5E1E) {
            case 4:
                if (!(flags & 4) && head->modeC != 0)
                    state |= 4;
                else
                    state &= ~4;
                if (flags & 0x8000) {
                    changed = 1;
                } else if (flags & 2) {
                    if (state & NODE_STATE_ON)
                        state &= ~NODE_STATE_ON;
                }
                break;
            case 2:
                if ((flags & 4) && head->modeC != 0)
                    state |= 4;
                else
                    state &= ~4;
                if (flags & 0x2000) {
                    changed = 1;
                    if (!(flags & 1)) {
                        state |= NODE_STATE_ON;
                        result = keyCode;
                    }
                }
                break;
            case 1:
                if ((flags & 4) && head->modeC != 0)
                    state |= 4;
                else
                    state &= ~4;
                if (flags & 0x1000) {
                    changed = 1;
                    if (!(flags & 1)) {
                        state ^= NODE_STATE_ON;
                        result = keyCode;
                    }
                    if (flags & 0x40) {
                        gPressedNode = head;
                        justPressed = 1;
                    }
                }
                break;
            case 3:
                if (flags & 0x4000) {
                    changed = 1;
                    if (flags & 1) {
                        state ^= NODE_STATE_ON;
                        result = keyCode;
                    }
                }
                if (flags & 2) {
                    if (state & NODE_STATE_ON)
                        state &= ~NODE_STATE_ON;
                }
                break;
            }
            goto update;
        }
        state &= ~6;
        if ((flags & 0x40) && gPressedNode && justPressed == 0) {
            if (flags & 0x80) {
                gListActive = 0;
            } else {
                if (flagsLow != 0) {
                    mask = g5DEA[g5E1C];
                    if (flags & mask)
                        changed = 1;
                }
                if (flagsHigh != 0 && changed == 0) {
                    mask = g5DF2[g5E1E];
                    if (flags & mask)
                        changed = 1;
                }
                if (changed == 0) {
                    gPressedNode = 0;
                    gListActive = 1;
                }
            }
        }
        if ((flags & 2) && (state & NODE_STATE_ON))
            state &= ~NODE_STATE_ON;
    update:
        head->state = state;
        head->flags = flags;
        if (!(state & NODE_STATE_8) != !(state & NODE_STATE_ON) ||
            !(state & NODE_STATE_10) != !(state & NODE_STATE_4))
            DrawNode(head);
        if (changed && head->callback30)
            more = !head->callback30(head);
        if ((state & 2) && (flags & 0x20))
            more = 0;
        if (gPressedNode) {
            if ((flagsLow != 0 && g5E1C == 3) || (flagsHigh != 0 && g5E1E == 3)) {
                gPressedNode = 0;
                gListActive = 1;
            }
        }
        if (gPressedNode && justPressed == 0)
            goto done;
        if (more) {
            if ((head = head->next) == 0)
                goto done;
        }
    checkMore:
        if (more)
            continue;
        break;
    }
done:
    if (result == 0)
        result = key;
    return result;
}

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
                    savedAttr = gStatusLine;
                    gStatusLine = 0;
                    gDrawText(node->src14, x, y, node->color28, node->color29);
                    gStatusLine = savedAttr;
                }
            } else if (type == 3) {
                savedAttr = gStatusLine;
                gStatusLine = 0;
                gDraw805E(node->tile36, x >> 3, y);
                gStatusLine = savedAttr;
            } else if (type == 4) {
                if (node->src14)
                    ((NodeFn)node->src14)(node);
            }
        } else if (modeB == 2) {
            if (!st4) {
                savedAttr = gStatusLine;
                gStatusLine = 0;
                DrawBox(x, y, x + node->width - 1, y + node->height - 1, node->color28);
                gStatusLine = savedAttr;
            }
        } else if (modeB == 3) {
            if (!st4) {
                savedAttr = gStatusLine;
                gStatusLine = 0;
                gDraw8066(x, y, x + node->width - 1, y + node->height - 1, node->color28);
                gStatusLine = savedAttr;
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
                savedAttr = gStatusLine;
                gStatusLine = 0;
                if (st1)
                    gDrawText(node->src18, x, y, node->color2C, node->color2D);
                else
                    gDrawText(node->src18, x, y, node->color2A, node->color2B);
                gStatusLine = savedAttr;
            } else if (type == 3) {
                savedAttr = gStatusLine;
                gStatusLine = 0;
                gDraw805E(node->tile36, x >> 3, y);
                gStatusLine = savedAttr;
            } else if (type == 4) {
                if (node->src18)
                    ((NodeFn)node->src18)(node);
            }
        } else if (modeC == 2) {
            savedAttr = gStatusLine;
            gStatusLine = 0;
            colour = st1 ? node->color2C : node->color2A;
            DrawBox(x, y, x + node->width - 1, y + node->height - 1, colour);
            gStatusLine = savedAttr;
        } else if (modeC == 3) {
            savedAttr = gStatusLine;
            gStatusLine = 0;
            colour = st1 ? node->color2C : node->color2A;
            gDraw8066(x, y, x + node->width - 1, y + node->height - 1, colour);
            gStatusLine = savedAttr;
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
                savedAttr = gStatusLine;
                gStatusLine = 0;
                gDrawText(node->src10, x, y, node->color26, node->color27);
                gStatusLine = savedAttr;
            } else if (type == 3) {
                savedAttr = gStatusLine;
                gStatusLine = 0;
                gDraw805E(node->tile34, x >> 3, y);
                gStatusLine = savedAttr;
            } else if (type == 4) {
                if (node->src10)
                    ((NodeFn)node->src10)(node);
            } else if (type == 5) {
                savedAttr = gStatusLine;
                gStatusLine = 0;
                DrawBox(x, y, x + node->width - 1, y + node->height - 1, node->color26);
                gStatusLine = savedAttr;
            } else if (type == 0) {
                savedAttr = gStatusLine;
                gStatusLine = 0;
                if (modeB == 2 || modeC == 2)
                    DrawBox(x, y, x + node->width - 1, y + node->height - 1, node->color26);
                else if (modeB == 3 && st8)
                    gDraw8066(x, y, x + node->width - 1, y + node->height - 1, node->color28);
                else if (modeC == 3 && st10)
                    gDraw8066(x, y, x + node->width - 1, y + node->height - 1, node->color2A);
                gStatusLine = savedAttr;
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
