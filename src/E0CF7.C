/* Module at EOB.EXE 0CF7:0007 (591 bytes, the whole code segment): links static records into
   singly linked lists. Each BuildList* seeds a list with the first record of a table and links the
   rest with ListAdd. Every `gList = ListAdd(gList, x)` leaves gList unchanged, because ListAdd
   returns its first argument (see its note below), so after each BuildList gList is still the
   table's first record, the one ListInit set. Still unknown: the purpose of gFlag (and so of the a
   and b table sets), and what BuildList1 to BuildList5 number. */

/* A list node: offset 0 is a far pointer to the next node, or null at the tail. The tables point
   at records in DGROUP spaced on a 60-byte grid (every table entry is 6 mod 0x3C); the record's
   real size is not proven, between 56 and 60: E16A5.C lays one out through tile36 at +0x36/+0x37
   (56 bytes) and E1658.C through +0x33 (52 bytes). Only this link is touched here: ListInit zeroes
   it (the two word stores the bytes show, a null far pointer). The layout below is E16A5.C's, which
   names and extends the record E1658.C declares. */
struct Node {
    struct Node far *next;      /* +00: next node, or a null pointer at the tail */
    unsigned key;               /* +04: the sort key E1658.C compares */
    unsigned key06;             /* +06: the first key word the dispatcher matches */
    unsigned key08;             /* +08: the second key word the dispatcher matches */
    unsigned char kind;         /* +0A: the drawing kind 0..5 DrawNode switches on */
    unsigned char modeB;        /* +0B: the "on" drawing mode DrawNode switches on */
    unsigned char modeC;        /* +0C: the second drawing mode DrawNode switches on */
    unsigned char pad0D;        /* +0D: alignment pad */
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
    int tile34;                 /* +34: tile id DrawNode draws in the default kind */
    int tile36;                 /* +36: tile id DrawNode draws in the switched modes */
};

extern char gFlag; /*@ ds:F3B9 */ /* selects the "a" table set when nonzero (the longer lists for
                                     tables 1 and 2, the shorter for table 3). Three writers: 1 at
                                     0EF3:2281 in the function at 0EF3:21E0, just before it calls
                                     BuildList1 at 0EF3:2286; 0 at 0EF3:2544 in the function at
                                     0EF3:2541 that calls BuildList1 to BuildList3; and 0 at
                                     0ACD:0FA9, just before its BuildList1 call. Readers: BuildList1
                                     to BuildList3 test it here; at 0EF3:2257 the function at
                                     0EF3:21E0 tests it first and, when it is zero, calls through the
                                     far pointer at ds:80AE (0EF3:2269..227A) before the 1 store, so
                                     that path writes 1 too. Purpose unknown. */
extern struct Node far *gList; /*@ ds:FB48 */ /* head of the screen node list walked by the key
                                                 dispatcher DispatchKeyWait (E16A5.C:139-143,
                                                 16A5:005A); 0ACD:0E6C and overlays 0 and 1 pass it
                                                 at ds:FB48 too. See the ListAdd note below. */
/* Table base addresses and record counts. The first entry of tables 1a, 1b, 2a, 2b, 3a and 3b is
   the same record, ds:4836, so the a and b sets differ only after their first node; tables 4 and 5
   start with the records at ds:3E9A and ds:4B7E. One count constant per table ties each loop to
   its table. */
#define TABLE1A_COUNT 52
#define TABLE1B_COUNT 38
#define TABLE2A_COUNT 59
#define TABLE2B_COUNT 47
#define TABLE3A_COUNT 19
#define TABLE3B_COUNT 32
#define TABLE4_COUNT  7
#define TABLE5_COUNT  7
extern struct Node far *gTable1a[]; /*@ ds:5102 */ /* 52 records, TABLE1A_COUNT */
extern struct Node far *gTable1b[]; /*@ ds:506A */ /* 38 records, TABLE1B_COUNT */
extern struct Node far *gTable2a[]; /*@ ds:52DA */ /* 59 records, TABLE2A_COUNT */
extern struct Node far *gTable2b[]; /*@ ds:51D2 */ /* 47 records, TABLE2B_COUNT */
extern struct Node far *gTable3a[]; /*@ ds:528E */ /* 19 records, TABLE3A_COUNT */
extern struct Node far *gTable3b[]; /*@ ds:53C6 */ /* 32 records, TABLE3B_COUNT */
extern struct Node far *gTable4[]; /*@ ds:5446 */ /* 7 records, TABLE4_COUNT */
extern struct Node far *gTable5[]; /*@ ds:5462 */ /* 7 records, TABLE5_COUNT */

/* Walks head to its tail (the first node whose link is null), links item after that tail, clears
   item's own link, and returns head. The return is always its first argument, so a caller that
   assigns the result back to its head keeps the same head. */
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
        for (i = 1; i < TABLE1A_COUNT; i++)
            gList = ListAdd(gList, gTable1a[i]);
    } else {
        ListInit(gTable1b[0]);
        for (i = 1; i < TABLE1B_COUNT; i++)
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
        for (i = 1; i < TABLE2A_COUNT; i++)
            gList = ListAdd(gList, gTable2a[i]);
    } else {
        ListInit(gTable2b[0]);
        for (i = 1; i < TABLE2B_COUNT; i++)
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
        for (i = 1; i < TABLE3A_COUNT; i++)
            gList = ListAdd(gList, gTable3a[i]);
    } else {
        ListInit(gTable3b[0]);
        for (i = 1; i < TABLE3B_COUNT; i++)
            gList = ListAdd(gList, gTable3b[i]);
    }
}

/* 0CF7:01CC: links gTable4's 7 records into a list; no flag, so one fixed table. Same shape as
   BuildList1 without the branch. Its only caller is 0ACD:0264. */
/*@ 0CF7:01CC */ void BuildList4(void)
{
    int i;

    ListInit(gTable4[0]);
    for (i = 1; i < TABLE4_COUNT; i++)
        gList = ListAdd(gList, gTable4[i]);
}

/* 0CF7:0211: links gTable5's 7 records into a list; no flag, one fixed table. Same shape as
   BuildList4. Its only caller is 0EF3:059D. */
/*@ 0CF7:0211 */ void BuildList5(void)
{
    int i;

    ListInit(gTable5[0]);
    for (i = 1; i < TABLE5_COUNT; i++)
        gList = ListAdd(gList, gTable5[i]);
}
