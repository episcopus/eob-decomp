/* Module at EOB.EXE 0CF7:0007 (591 bytes, the whole code segment): links static records into
   singly linked lists. Go/no-go spike for the toolchain (SPEC 4.2): Turbo C++ 1.00, -ml -Z -O.
   Names are placeholders until the callers and the tables are understood. */

/* A list node: offset 0 is a far pointer to the next node, or null at the tail. The records the
   tables point at are 60 bytes each in DGROUP (stride 0x3C), but only this link is touched here.
   ListInit zeroes it, which is a null far pointer, the two word stores the bytes show. */
struct Node { struct Node far *next; };

extern char gFlag; /*@ ds:F3B9 */ /* picks the "a" table set when nonzero; written 1 at 0EF3:2281
                                     and 0 at 0EF3:2544 and 0ACD:0FA9. Meaning unknown. */
extern struct Node far *gList; /*@ ds:FB48 */ /* head of the list built last (see the ListAdd note) */
extern struct Node far *gTable1a[]; /*@ ds:5102 */
extern struct Node far *gTable1b[]; /*@ ds:506A */
extern struct Node far *gTable2a[]; /*@ ds:52DA */
extern struct Node far *gTable2b[]; /*@ ds:51D2 */
extern struct Node far *gTable3a[]; /*@ ds:528E */
extern struct Node far *gTable3b[]; /*@ ds:53C6 */
extern struct Node far *gTable4[]; /*@ ds:5446 */
extern struct Node far *gTable5[]; /*@ ds:5462 */

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
