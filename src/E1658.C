/* Unit at EOB.EXE 1658:0003 (1238 bytes, the whole code segment, 12 functions): the operations of
   a singly linked list of screen items (nodes). A node is a struct with a far `next` pointer, an
   unsigned sort key, a flags word at +0E, a state word at +2E and a far callback at +30; the box
   coordinates the drawing routine uses sit at +1E..+24. 16A5:07C3 draws the node (it calls the
   text and box fronts 1A29:000F and ds:80CA) and dispatches the state operations; 0CF7:001E's
   ListAdd, shared with this unit, and InsertNode here build the list in key order. Names and the
   unknown bit meanings are placeholders until the callers name them. */

struct Node {
    struct Node far *next;                       /* +00: next node, or a null pointer at the tail */
    unsigned key;                                /* +04: sort key, compared unsigned */
    unsigned char pad06[8];                      /* +06: unknown fields */
    unsigned flags;                              /* +0E: the inactive bit and the two layout bits */
    unsigned char pad10[0x1E];                   /* +10: unknown fields */
    unsigned state;                              /* +2E: the on bit, the previous-on bit, others */
    void (far *callback)(struct Node far *node); /* +30: optional callback, fired when asked */
};

/* The +0E flags word. Bit 8 makes 0073, 01E6 and 02F5 do nothing but return the node, and 04A7
   returns its inverse, so it reads as an "inactive" mark: 0154 sets it and 017B clears it. Bits
   1, 0x100, 0x400, 0x1000 and 0x4000 only pick which of the two globals below is touched. */
#define NODE_INACTIVE   0x0008

/* The +2E state word. Bit 0 is the value 0073 toggles and 02F5 sets; 01E6 clears bits 0 and 1.
   Bit 8 is set to the OLD value of bit 0 just before bit 0 is changed, so a redraw can tell that
   the node changed. The remaining bits (4, 0x10) are used only by 01E6. */
#define NODE_STATE_ON   0x0001
#define NODE_STATE_PREV 0x0008

/* The two words at ds:5E1C and ds:5E1E, set here to 0x81 or 0x83 and compared with 1 or 3; bit 7
   of either makes the whole block a no-op. 16A5:00F6 writes them from its key checks (2 or 4, then
   1 or 3), so they carry a per-key state, but what drives them here is unknown. */
extern unsigned g5E1C; /*@ ds:5E1C */
extern unsigned g5E1E; /*@ ds:5E1E */

/* 16A5:07C3: draws the node (box at +1E..+24, text at +14/+16 and +28/+29) through the front
   ends 1A29:000F and ds:80CA, reading the mouse pointer size at ds:8154/8156. 1658:0073, 0154,
   017B, 01E6 and 02F5 all call it after changing the node. */
extern void DrawNode(struct Node far *node); /*@ 16A5:07C3 */

/* 1658:0003: stores the far pointer `next` in the node's own +00 field and returns the node. Only
   03D6 calls it, to link the node it keeps in front of the list it was given. */
/*@ 1658:0003 */ struct Node far *LinkNode(struct Node far *next, struct Node far *node)
{
    node->next = next;
    return node;
}

/* 1658:001E: walks `head` to its tail, links `item` after it, clears item's own link and returns
   the original head. Shared with E0CF7.C, which builds every list with it. */
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
   has a callback, fire it. Returns the node. The 5E1C/5E1E update is identical in 01E6 and
   02F5. */
/*@ 1658:0073 */ struct Node far *ToggleNode(struct Node far *node, int notify)
{
    if (!(node->flags & NODE_INACTIVE)) {
        if (node->state & NODE_STATE_ON)
            node->state |= NODE_STATE_PREV;
        else
            node->state &= ~NODE_STATE_PREV;
        node->state ^= NODE_STATE_ON;
        DrawNode(node);
        if (!(g5E1C & 0x80) && !(g5E1E & 0x80)) {
            if (node->flags & 1) {
                if (node->flags & 0x400) {
                    if (g5E1C != 3)
                        g5E1C = 0x83;
                } else if (node->flags & 0x4000) {
                    if (g5E1E != 3)
                        g5E1E = 0x83;
                }
            } else {
                if (node->flags & 0x100) {
                    if (g5E1C != 1)
                        g5E1C = 0x81;
                } else if (node->flags & 0x1000) {
                    if (g5E1E != 1)
                        g5E1E = 0x81;
                }
            }
        }
        if (notify && node->callback)
            node->callback(node);
    }
    return node;
}

/* 1658:0154: if the node is not inactive, make it inactive (set flags bit 8) and redraw it;
   returns the node. 04A7 returns the inverse of that bit. */
/*@ 1658:0154 */ struct Node far *DisableNode(struct Node far *node)
{
    if (!(node->flags & NODE_INACTIVE)) {
        node->flags |= NODE_INACTIVE;
        DrawNode(node);
    }
    return node;
}

/* 1658:017B: if the node is inactive, make it active again (clear flags bit 8) and redraw it;
   returns the node. The counterpart of DisableNode. */
/*@ 1658:017B */ struct Node far *EnableNode(struct Node far *node)
{
    if (node->flags & NODE_INACTIVE) {
        node->flags &= ~NODE_INACTIVE;
        DrawNode(node);
    }
    return node;
}

/* 1658:01A2: returns the first node whose key equals `key`, or a null pointer. The search walks
   from `head` and compares each node, including the tail, whose key is compared after the loop,
   which is why the comparison appears twice. Its callers are in overlay 0 and overlay 1. */
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

/* 1658:01E6: if the node is not inactive, save its on bit into the previous-on bit, then clear on
   bits 0 and 1, set bit 2, map bit 2 to bit 4 (the else branch is dead: bit 2 was just set, so
   the tested value is never zero) and clear bit 2 again; redraw and fire the callback as in
   0073. Returns the node. */
/*@ 1658:01E6 */ struct Node far *ResetNode(struct Node far *node, int notify)
{
    if (!(node->flags & NODE_INACTIVE)) {
        if (node->state & NODE_STATE_ON)
            node->state |= NODE_STATE_PREV;
        else
            node->state &= ~NODE_STATE_PREV;
        node->state &= ~NODE_STATE_ON;
        node->state &= ~2;
        if ((node->state |= 4) != 0)
            node->state |= 0x10;
        else
            node->state &= ~0x10;
        node->state &= ~4;
        DrawNode(node);
        if (!(g5E1C & 0x80) && !(g5E1E & 0x80)) {
            if (node->flags & 1) {
                if (node->flags & 0x400) {
                    if (g5E1C != 3)
                        g5E1C = 0x83;
                } else if (node->flags & 0x4000) {
                    if (g5E1E != 3)
                        g5E1E = 0x83;
                }
            } else {
                if (node->flags & 0x100) {
                    if (g5E1C != 1)
                        g5E1C = 0x81;
                } else if (node->flags & 0x1000) {
                    if (g5E1E != 1)
                        g5E1E = 0x81;
                }
            }
        }
        if (notify && node->callback)
            node->callback(node);
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
        if (!(g5E1C & 0x80) && !(g5E1E & 0x80)) {
            if (node->flags & 1) {
                if (node->flags & 0x400) {
                    if (g5E1C != 3)
                        g5E1C = 0x83;
                } else if (node->flags & 0x4000) {
                    if (g5E1E != 3)
                        g5E1E = 0x83;
                }
            } else {
                if (node->flags & 0x100) {
                    if (g5E1C != 1)
                        g5E1C = 0x81;
                } else if (node->flags & 0x1000) {
                    if (g5E1E != 1)
                        g5E1E = 0x81;
                }
            }
        }
        if (notify && node->callback)
            node->callback(node);
    }
    return node;
}

/* 1658:03D6: inserts `item` into the list `head` in key order, in front of the first node with an
   equal or greater key, or after the last node through ListAdd when its key is the largest.
   Returns the list, which is `item` itself when it became the new head. No direct caller was
   found; it is likely reached through the same dispatch as the state operations. */
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

/* 1658:04A7: returns 1 when the node is not inactive (flags bit 8 clear), else 0. */
/*@ 1658:04A7 */ int IsNodeEnabled(struct Node far *node)
{
    return !(node->flags & NODE_INACTIVE);
}

/* 1658:04BB: reads the node's state word and returns its on bit (bit 0), the bit that 0073
   toggles and 02F5 sets. No direct caller was found in the resident code; it is likely reached
   through the same dispatch as 0073. */
/*@ 1658:04BB */ int GetNodeState0(struct Node far *node)
{
    return node->state & NODE_STATE_ON;
}

/* 1658:04CA: reads the node's state word and returns bit 1, the second of the two state bits.
   Like 04BB it has no direct caller in the resident code. */
/*@ 1658:04CA */ int GetNodeState1(struct Node far *node)
{
    return node->state & 2;
}
