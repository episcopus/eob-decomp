/* Unit at EOB.EXE 1658:0003 (1238 bytes, the segment's code from 1658:0003; the three bytes
   1658:0000 to 0002 belong to unit e1547, which ends where this one starts): the operations of a
   singly linked list of screen items (nodes), 12 functions. A node is a struct with a far `next`
   pointer, an unsigned sort key, a flags word at +0E, a state word at +2E and a far callback at
   +30; the box coordinates the drawing routine uses sit at +1E..+24. DrawNode (16A5:07C3) first
   reads the state word +2E: with the on bit 0x0001 set, modeB +0B picks the path; with 0x0004
   set, modeC +0C does; only when both are clear does it draw by the kind +0A (art through
   1A29:000F for kind 1, text through gDrawText ds:80CA for kind 2, a tile through gDraw805E
   ds:805E for kind 3, a box through DrawBox 1A1F:000C or gDraw8066 ds:8066 for kinds 5 and 0).
   The five state operations here (0073, 0154, 017B, 01E6, 02F5) and the dispatcher 16A5:00BD
   call it. ListAdd (1658:001E), shared with this unit, appends to a list; InsertNode (1658:03D6)
   here inserts in key order. Names and the unknown bit meanings are placeholders until the
   callers name them. */

struct Node {
    struct Node far *next;                       /* +00: next node, or a null pointer at the tail */
    unsigned key;                                /* +04: sort key, compared unsigned */
    unsigned char pad06[8];                      /* +06..+0D: key06 (+06), key08 (+08), kind (+0A),
                                                    modeB (+0B) and modeC (+0C) as E16A5.C names
                                                    them; only +0E is touched here */
    unsigned flags;                              /* +0E: the inactive bit and the two layout bits */
    unsigned char pad10[0x1E];                   /* +10..+2D: the art pointers, region, box and
                                                    colour fields E16A5.C names (+10 src10, +14
                                                    src14, +18 src18, then the box and colours);
                                                    only +2E is read and written here */
    unsigned state;                              /* +2E: the on bit, the previous-on bit, others */
    int (far *callback30)(struct Node far *node); /* +30: optional callback, fired when asked; the
                                                    dispatcher is the only caller that uses its
                                                    result (E16A5.C:55, 178-179) */
};

/* The +0E flags word. Bit 0x0008 makes 0073, 01E6 and 02F5 do nothing but return the node, and
   04A7 returns its inverse, so it reads as an "inactive" mark: 0154 sets it and 017B clears it.
   Bit 0x0001 chooses both which of the two globals below is touched and the value written: set,
   0x0400 picks g5E1C and 0x4000 picks g5E1E, and the value written is 3 or 0x83; clear, 0x0100
   picks g5E1C and 0x1000 picks g5E1E, and the value is 1 or 0x81. The 0x80 of 0x81/0x83 then makes
   the whole block a no-op on the next call. */
#define NODE_INACTIVE   0x0008
#define NODE_FLAG_ON    0x0001  /* picks the on pair (value 3) over the off pair (value 1) */
#define NODE_FLAG_ON_5E1C  0x0400 /* with NODE_FLAG_ON set: g5E1C gets 3 or 0x83 */
#define NODE_FLAG_ON_5E1E  0x4000 /* with NODE_FLAG_ON set: g5E1E gets 3 or 0x83 */
#define NODE_FLAG_OFF_5E1C 0x0100 /* with NODE_FLAG_ON clear: g5E1C gets 1 or 0x81 */
#define NODE_FLAG_OFF_5E1E 0x1000 /* with NODE_FLAG_ON clear: g5E1E gets 1 or 0x81 */

/* The g5E1C/g5E1E button modes: 1 and 3 are the plain modes DispatchNodeEvent leaves, and this
   unit writes them with the 0x80 latch bit set so the block is a no-op on the next call. */
#define BUTTON_MODE_1       1
#define BUTTON_MODE_3       3
#define BUTTON_MODE_LATCH   0x80

/* The +2E state word. Bit 0x0001 is the value 0073 toggles and 02F5 sets; 01E6 clears bits 0x0001
   and 0x0002. Bit 0x0008 is set to the OLD value of bit 0x0001 just before bit 0x0001 is changed,
   so a redraw can tell that the node changed; E16A5.C calls this bit NODE_STATE_8 and redraws when
   bits 0x0001/0x0008 differ (E16A5.C:178), which is the "previous on" reading below. The remaining
   bits (0x0004, 0x0010) are used only by 01E6 in this unit; E16A5.C sets and tests them too. */
#define NODE_STATE_ON   0x0001
#define NODE_STATE_4    0x0004
#define NODE_STATE_PREV 0x0008
#define NODE_STATE_10   0x0010

/* The two words at ds:5E1C and ds:5E1E, set here to 0x81 or 0x83 and compared with 1 or 3; bit
   0x80 of either makes the whole block a no-op. DispatchNodeEvent (16A5:00BD) sets them from the
   button key codes (0x41/0x42 and 0xC6/0xC7) to the mouse button mode (1 to 4) (E16A5.C:165-166,
   206-212, 221-239), but what drives them here is unknown. Declared `unsigned` here and in
   E0ACD.C:104; E16A5.C:104-105 still calls them `int`. */
extern unsigned g5E1C; /*@ ds:5E1C */
extern unsigned g5E1E; /*@ ds:5E1E */

/* 16A5:07C3: draws the node by its kind +0A, box at +1E..+24. For the "on" looks it uses the art
   or callback pointer src14 at +14 and the colour pair at +28/+29, and it reads the mouse pointer
   size at ds:8154/8156. The rest of the record (+10 src10, +18 src18, the colour pair +26/+27 and
   the box) is named in E16A5.C. 1658:0073, 0154, 017B, 01E6 and 02F5 all call it after changing
   the node. */
extern void DrawNode(struct Node far *node); /*@ 16A5:07C3 */

/* 1658:0003: stores the far pointer `next` in the node's own +00 field and returns the node. Only
   03D6 calls it, to link the node it keeps in front of the list it was given. */
/*@ 1658:0003 */ struct Node far *LinkNode(struct Node far *next, struct Node far *node)
{
    node->next = next;
    return node;
}

/* 1658:001E: walks `head` to its tail, links `item` after it, clears item's own link and returns
   the original head. Shared with the BuildList functions (BuildList1 to BuildList5 in E0CF7.C,
   BuildList1 to BuildList4 in E0ACD.C), BuildRowNodeList (ovl0:042F, EOV0.C:284) and _Ovl1_1153
   (ovl1:1153, EOV1.ASM), and called by InsertNode below. */
/*@ 1658:001E */ struct Node far *ListAdd(struct Node far *head, struct Node far *item)
{
    struct Node far *first;

    first = head;
    while (head->next)
        head = head->next;
    head->next = item;
    item->next = 0;
    return first;
}

/* 1658:0073: if the node is not inactive, save its on bit into the previous-on bit, toggle the on
   bit, redraw the node, and run the ds:5E1C / ds:5E1E update. When `notify` is set and the node
   has a callback, fire it. Returns the node. The 5E1C/5E1E update is the same as ClearNode's and
   SetNode's. */
/*@ 1658:0073 */ struct Node far *ToggleNode(struct Node far *node, int notify)
{
    if (!(node->flags & NODE_INACTIVE)) {
        if (node->state & NODE_STATE_ON)
            node->state |= NODE_STATE_PREV;
        else
            node->state &= ~NODE_STATE_PREV;
        node->state ^= NODE_STATE_ON;
        DrawNode(node);
        if (!(g5E1C & BUTTON_MODE_LATCH) && !(g5E1E & BUTTON_MODE_LATCH)) {
            if (node->flags & NODE_FLAG_ON) {
                if (node->flags & NODE_FLAG_ON_5E1C) {
                    if (g5E1C != BUTTON_MODE_3)
                        g5E1C = BUTTON_MODE_3 | BUTTON_MODE_LATCH;
                } else if (node->flags & NODE_FLAG_ON_5E1E) {
                    if (g5E1E != BUTTON_MODE_3)
                        g5E1E = BUTTON_MODE_3 | BUTTON_MODE_LATCH;
                }
            } else {
                if (node->flags & NODE_FLAG_OFF_5E1C) {
                    if (g5E1C != BUTTON_MODE_1)
                        g5E1C = BUTTON_MODE_1 | BUTTON_MODE_LATCH;
                } else if (node->flags & NODE_FLAG_OFF_5E1E) {
                    if (g5E1E != BUTTON_MODE_1)
                        g5E1E = BUTTON_MODE_1 | BUTTON_MODE_LATCH;
                }
            }
        }
        if (notify && node->callback30)
            node->callback30(node);
    }
    return node;
}

/* 1658:0154: if the node is not inactive, make it inactive (set flags bit 0x0008) and redraw it;
   returns the node. 04A7 returns the inverse of that bit. DispatchNodeEvent (16A5:00BD) skips a
   node whose 0x0008 is set (E16A5.C:171-172). */
/*@ 1658:0154 */ struct Node far *DisableNode(struct Node far *node)
{
    if (!(node->flags & NODE_INACTIVE)) {
        node->flags |= NODE_INACTIVE;
        DrawNode(node);
    }
    return node;
}

/* 1658:017B: if the node is inactive, make it active again (clear flags bit 0x0008) and redraw
   it; returns the node. The counterpart of DisableNode, whose 0x0008 DispatchNodeEvent
   (16A5:00BD) tests (E16A5.C:171-172). */
/*@ 1658:017B */ struct Node far *EnableNode(struct Node far *node)
{
    if (node->flags & NODE_INACTIVE) {
        node->flags &= ~NODE_INACTIVE;
        DrawNode(node);
    }
    return node;
}

/* 1658:01A2: returns the first node whose key equals `key`, or a null pointer. The search walks
   from `head`, comparing each node before stepping, then compares the last node's key after the
   loop, which is why the comparison appears twice. Its callers are in overlay 0 and overlay 1.
   QUIRK 1658:01A2: when the list has one node whose key does not match, `head = head->next` makes
   `head` a null far pointer; the loop test `while (head->next)` then reads the word at 0000:0000,
   and the final `head->key` reads 0000:0004. If the word at 0000:0000 (an interrupt vector) is
   nonzero, the loop walks the vector table as if its words were nodes. A port must keep the
   null-pointer read. */
/*@ 1658:01A2 */ struct Node far *FindNode(struct Node far *head, unsigned key)
{
    do {
        if (head->key == key)
            return head;
        head = head->next;
    } while (head->next);
    if (head->key == key)
        return head;
    return 0;
}

/* 1658:01E6: the counterpart of SetNode: if the node is not inactive, save its on bit into the
   previous-on bit, clear the on bit (0x0001) and 0x0002, mark 0x0010 and clear 0x0004. The 0x0004
   set and the 0x0010 mark are the dispatcher's "0x0004 was set" mapping (E16A5.C:286-289); its
   else branch is dead here because 0x0004 was just set. Then redraw and fire the callback as in
   0073. Returns the node. */
/*@ 1658:01E6 */ struct Node far *ClearNode(struct Node far *node, int notify)
{
    if (!(node->flags & NODE_INACTIVE)) {
        if (node->state & NODE_STATE_ON)
            node->state |= NODE_STATE_PREV;
        else
            node->state &= ~NODE_STATE_PREV;
        node->state &= ~NODE_STATE_ON;
        node->state &= ~2;
        if ((node->state |= NODE_STATE_4) != 0)
            node->state |= NODE_STATE_10;
        else
            node->state &= ~NODE_STATE_10;
        node->state &= ~NODE_STATE_4;
        DrawNode(node);
        if (!(g5E1C & BUTTON_MODE_LATCH) && !(g5E1E & BUTTON_MODE_LATCH)) {
            if (node->flags & NODE_FLAG_ON) {
                if (node->flags & NODE_FLAG_ON_5E1C) {
                    if (g5E1C != BUTTON_MODE_3)
                        g5E1C = BUTTON_MODE_3 | BUTTON_MODE_LATCH;
                } else if (node->flags & NODE_FLAG_ON_5E1E) {
                    if (g5E1E != BUTTON_MODE_3)
                        g5E1E = BUTTON_MODE_3 | BUTTON_MODE_LATCH;
                }
            } else {
                if (node->flags & NODE_FLAG_OFF_5E1C) {
                    if (g5E1C != BUTTON_MODE_1)
                        g5E1C = BUTTON_MODE_1 | BUTTON_MODE_LATCH;
                } else if (node->flags & NODE_FLAG_OFF_5E1E) {
                    if (g5E1E != BUTTON_MODE_1)
                        g5E1E = BUTTON_MODE_1 | BUTTON_MODE_LATCH;
                }
            }
        }
        if (notify && node->callback30)
            node->callback30(node);
    }
    return node;
}

/* 1658:02F5: like 0073 but with the on bit set instead of toggled: if the node is not inactive,
   save its on bit into the previous-on bit, set the on bit, redraw and fire the callback as in
   0073. Returns the node. */
/*@ 1658:02F5 */ struct Node far *SetNode(struct Node far *node, int notify)
{
    if (!(node->flags & NODE_INACTIVE)) {
        if (node->state & NODE_STATE_ON)
            node->state |= NODE_STATE_PREV;
        else
            node->state &= ~NODE_STATE_PREV;
        node->state |= NODE_STATE_ON;
        DrawNode(node);
        if (!(g5E1C & BUTTON_MODE_LATCH) && !(g5E1E & BUTTON_MODE_LATCH)) {
            if (node->flags & NODE_FLAG_ON) {
                if (node->flags & NODE_FLAG_ON_5E1C) {
                    if (g5E1C != BUTTON_MODE_3)
                        g5E1C = BUTTON_MODE_3 | BUTTON_MODE_LATCH;
                } else if (node->flags & NODE_FLAG_ON_5E1E) {
                    if (g5E1E != BUTTON_MODE_3)
                        g5E1E = BUTTON_MODE_3 | BUTTON_MODE_LATCH;
                }
            } else {
                if (node->flags & NODE_FLAG_OFF_5E1C) {
                    if (g5E1C != BUTTON_MODE_1)
                        g5E1C = BUTTON_MODE_1 | BUTTON_MODE_LATCH;
                } else if (node->flags & NODE_FLAG_OFF_5E1E) {
                    if (g5E1E != BUTTON_MODE_1)
                        g5E1E = BUTTON_MODE_1 | BUTTON_MODE_LATCH;
                }
            }
        }
        if (notify && node->callback30)
            node->callback30(node);
    }
    return node;
}

/* 1658:03D6: inserts `item` into the list `head` in key order, in front of the first node with an
   equal or greater key, or after the last node through ListAdd when its key is the largest.
   Returns the list, which is `item` itself when it became the new head. No direct caller was
   found in the resident code or the overlays. */
/*@ 1658:03D6 */ struct Node far *InsertNode(struct Node far *head, struct Node far *item)
{
    struct Node far *first;
    struct Node far *prev;

    prev = first = head;
    if (item->key <= first->key) {
        head = LinkNode(head, item);
        return head;
    }
    while (item->key > head->key && head->next) {
        prev = head;
        head = head->next;
    }
    if (item->key > head->key) {
        head = ListAdd(first, item);
    } else {
        prev->next = item;
        item->next = head;
    }
    return first;
}

/* 1658:04A7: returns 1 when the node is not inactive (flags bit 0x0008 clear), else 0. */
/*@ 1658:04A7 */ int IsNodeEnabled(struct Node far *node)
{
    return !(node->flags & NODE_INACTIVE);
}

/* 1658:04BB: reads the node's state word and returns its on bit (0x0001), the bit 0073 toggles and
   02F5 sets. No direct caller was found in the resident code or the overlays. */
/*@ 1658:04BB */ int IsNodeOn(struct Node far *node)
{
    return node->state & NODE_STATE_ON;
}

/* 1658:04CA: Purpose unknown: reads the node's state word and returns bit 0x0002, the second of
   the two state bits; no name or caller for that bit was found in the tree. Like 04BB it has no
   direct caller in the resident code. */
/*@ 1658:04CA */ int GetNodeState1(struct Node far *node)
{
    return node->state & 2;
}
