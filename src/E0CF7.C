/* Module at EOB.EXE 0CF7:0007 (591 bytes, the whole code segment): links static records into
   singly linked lists. Each BuildList* seeds a list with the first record of a table and appends
   the rest with ListAdd. Still unknown: the meaning of gFlag (and so of the a and b table sets),
   and what BuildList1 to BuildList5 number. */

/* A list node: offset 0 is a far pointer to the next node, or null at the tail. The tables point
   at records in DGROUP spaced on a 60-byte grid (every table entry is 6 mod 0x3C); the record's
   real size is not proven, E1658.C lays one out through +0x33 (52 bytes), so it is 52 to 60. Only
   this link is touched here: ListInit zeroes it, the two word stores the bytes show, a null far
   pointer. Same tag and layout as E1658.C, which shares these nodes, so the two agree once a
   header exists. */
struct Node {
    struct Node far *next;                       /* +00: next node, or a null pointer at the tail */
    unsigned key;                                /* +04: sort key, compared unsigned */
    unsigned char pad06[8];                      /* +06: unknown fields */
    unsigned flags;                              /* +0E: the inactive bit and the layout bits */
    unsigned char pad10[0x1E];                   /* +10: unknown fields */
    unsigned state;                              /* +2E: the on bit, the previous-on bit, others */
    void (far *callback)(struct Node far *node); /* +30: optional callback, fired when asked */
};

extern char gFlag; /*@ ds:F3B9 */ /* picks the "a" table set when nonzero (the longer lists for
                                     tables 1 and 2, the shorter for table 3). Written 1 at 0EF3:2281,
                                     just before that function calls BuildList1 at 0EF3:2286; written
                                     0 at 0EF3:2544 in the function at 0EF3:2541 that calls BuildList1
                                     to BuildList3, and at 0ACD:0FA9. Meaning unknown. */
extern struct Node far *gList; /*@ ds:FB48 */ /* head of the screen node list walked by the key
                                                 dispatcher DispatchKeyWait (E16A5.C:56-60, 16A5:005A);
                                                 0ACD:0E6C and overlays 0 and 1 pass it at ds:FB48
                                                 too. See the ListAdd note below. */
/* Table base addresses and record counts. The first entry of tables 1a, 1b, 2a, 2b, 3a and 3b is
   the same record, ds:4836, so the a and b sets differ only after their first node; tables 4 and 5
   start with the records at ds:3E9A and ds:4B7E. */
extern struct Node far *gTable1a[]; /*@ ds:5102 */ /* 52 records */
extern struct Node far *gTable1b[]; /*@ ds:506A */ /* 38 records */
extern struct Node far *gTable2a[]; /*@ ds:52DA */ /* 59 records */
extern struct Node far *gTable2b[]; /*@ ds:51D2 */ /* 47 records */
extern struct Node far *gTable3a[]; /*@ ds:528E */ /* 19 records */
extern struct Node far *gTable3b[]; /*@ ds:53C6 */ /* 32 records */
extern struct Node far *gTable4[]; /*@ ds:5446 */ /* 7 records */
extern struct Node far *gTable5[]; /*@ ds:5462 */ /* 7 records */

/* Walks head to its tail (the first node whose link is null), stores item there, clears item's
   own link, and returns head. The return is always its first argument. */
extern struct Node far *ListAdd(struct Node far *head, struct Node far *item); /*@ 1658:001E */

/* 0CF7:0007: points gList at head and clears head's link to a null far pointer, making head the
   only node of a new list. */
/*@ 0CF7:0007 */ void ListInit(struct Node far *head)
{
    gList = head;
    gList->next = 0;
}

/* 0CF7:0028: links one table's records into a list. gFlag nonzero picks gTable1a (52 records, the
   first seeds the list); zero picks gTable1b (38 records). */
/*@ 0CF7:0028 */ void BuildList1(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable1a[0]);
        for (i = 1; i < 52; i++)
            gList = ListAdd(gList, gTable1a[i]);
    } else {
        ListInit(gTable1b[0]);
        for (i = 1; i < 38; i++)
            gList = ListAdd(gList, gTable1b[i]);
    }
}

/* 0CF7:00B4: links one table's records into a list. gFlag nonzero picks gTable2a (59 records);
   zero picks gTable2b (47 records). Same shape as BuildList1. */
/*@ 0CF7:00B4 */ void BuildList2(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable2a[0]);
        for (i = 1; i < 59; i++)
            gList = ListAdd(gList, gTable2a[i]);
    } else {
        ListInit(gTable2b[0]);
        for (i = 1; i < 47; i++)
            gList = ListAdd(gList, gTable2b[i]);
    }
}

/* 0CF7:0140: links one table's records into a list. gFlag nonzero picks gTable3a (19 records);
   zero picks gTable3b (32 records). Same shape as BuildList1. */
/*@ 0CF7:0140 */ void BuildList3(void)
{
    int i;

    if (gFlag) {
        ListInit(gTable3a[0]);
        for (i = 1; i < 19; i++)
            gList = ListAdd(gList, gTable3a[i]);
    } else {
        ListInit(gTable3b[0]);
        for (i = 1; i < 32; i++)
            gList = ListAdd(gList, gTable3b[i]);
    }
}

/* 0CF7:01CC: links gTable4's 7 records into a list; no flag, so one fixed table. Same shape as
   BuildList1 without the branch. Its only caller is 0ACD:0264. */
/*@ 0CF7:01CC */ void BuildList4(void)
{
    int i;

    ListInit(gTable4[0]);
    for (i = 1; i < 7; i++)
        gList = ListAdd(gList, gTable4[i]);
}

/* 0CF7:0211: links gTable5's 7 records into a list; no flag, one fixed table. Same shape as
   BuildList4. Its only caller is 0EF3:059D. */
/*@ 0CF7:0211 */ void BuildList5(void)
{
    int i;

    ListInit(gTable5[0]);
    for (i = 1; i < 7; i++)
        gList = ListAdd(gList, gTable5[i]);
}
